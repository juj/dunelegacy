/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <units/GroundUnit.h>

#include <globals.h>

#include <Game.h>
#include <House.h>
#include <Map.h>
#include <SoundPlayer.h>

#include <players/HumanPlayer.h>

#include <structures/RepairYard.h>
#include <units/Carryall.h>

#include "mmath.h"

GroundUnit::GroundUnit(const GroundUnitConstants& constants, uint32_t objectID, const ObjectInitializer& initializer)
    : UnitBase(constants, objectID, initializer) { }

GroundUnit::GroundUnit(const GroundUnitConstants& constants, uint32_t objectID,
                       const ObjectStreamInitializer& initializer)
    : UnitBase(constants, objectID, initializer) {

    auto& stream = initializer.stream();

    awaitingPickup = stream.readBool();
    bookedCarrier  = stream.readUint32();
}

GroundUnit::~GroundUnit() = default;

void GroundUnit::save(OutputStream& stream) const {
    UnitBase::save(stream);

    stream.writeBool(awaitingPickup);
    stream.writeUint32(bookedCarrier);
}

void GroundUnit::assignToMap(const GameContext& context, const Coord& pos) {
    auto& map = context.map;

    if (auto* tile = map.tryGetTile(pos.x, pos.y)) {
        tile->assignNonInfantryGroundObject(getObjectID());
        map.viewMap(owner_->getHouseID(), pos, getViewRange());
    }
}

void GroundUnit::checkPos(const GameContext& context) {
    auto* pTile = context.map.getTile(location_);

    if (!moving && !justStoppedMoving && !isInfantry()) {
        pTile->setTrack(drawnAngle_, context.game.getGameCycleCount());
    }

    if (justStoppedMoving) {
        realX_ = location_.x * TILESIZE + TILESIZE / 2;
        realY_ = location_.y * TILESIZE + TILESIZE / 2;
        // findTargetTimer = 0;  //allow a scan for new targets now

        if (pTile->isSpiceBloom()) {
            setHealth(0);
            setVisible(VIS_ALL, false);
            pTile->triggerSpiceBloom(context, getOwner());
        } else if (pTile->isSpecialBloom()) {
            pTile->triggerSpecialBloom(context, getOwner());
        }
    }

    /*
        Go to repair yard if low on health
    */
    if (active_ && (getHealth() < getMaxHealth() / 2) && !goingToRepairYard && owner_->hasRepairYard() && !pickedUp
        && owner_->hasCarryalls() && owner_->getHouseID() == originalHouseID_ // stop deviated units from being repaired
        && !isInfantry() && !forced_) { // Stefan - Allow units with targets to be picked up for repairs

        doRepair(context);
    }

    if (goingToRepairYard) {
        if (target_.getObjPointer() == nullptr) {
            goingToRepairYard = false;
            awaitingPickup    = false;
            bookedCarrier     = NONE_ID;

            clearPath();
        } else {
            const auto* const pObject = pTile->getGroundObject(context.objectManager);

            if (justStoppedMoving && (pObject != nullptr) && (pObject->getObjectID() == target_.getObjectID())) {
                if (const auto* const pRepairYard = dune_cast<RepairYard>(target_.getObjPointer())) {
                    if (pRepairYard->isFree()) {
                        setGettingRepaired();
                    } else {
                        // the repair yard is already in use by some other unit => move out
                        const Coord newDestination =
                            context.map.findDeploySpot(this, target_.getObjPointer()->getLocation(), getLocation(),
                                                       pRepairYard->getStructureSize());
                        doMove2Pos(context, newDestination, true);
                    }
                }
            }
        }
    }

    // If we are awaiting a pickup try book a carryall if we have one
    if (!pickedUp && attackMode_ == CARRYALLREQUESTED && bookedCarrier == NONE_ID) {
        if (getOwner()->hasCarryalls() && (target_ || (destination_ != location_))) {
            requestCarryall(context);
        } else {
            if (getItemID() == Unit_Harvester) {
                doSetAttackMode(context, HARVEST);
            } else {
                doSetAttackMode(context, GUARD);
            }
        }
    }
}

void GroundUnit::playConfirmSound() {
    dune::globals::soundPlayer->playVoice(
        dune::globals::pGFXManager->random().getRandOf(Voice_enum::Acknowledged, Voice_enum::Affirmative),
        getOwner()->getHouseID());
}

void GroundUnit::playSelectSound() {
    dune::globals::soundPlayer->playVoice(Voice_enum::Reporting, getOwner()->getHouseID());
}

/**
    Request a Carryall to drop at target location
**/

void GroundUnit::doRequestCarryallDrop(const GameContext& context, int xPos, int yPos) {
    if (getOwner()->hasCarryalls() && !awaitingPickup && context.map.tileExists(xPos, yPos)) {
        doMove2Pos(context, xPos, yPos, true);
        requestCarryall(context);
    }
}

bool GroundUnit::requestCarryall(const GameContext& context) {
    if (getOwner()->hasCarryalls() && !awaitingPickup) {

        // This allows a unit to keep requesting a carryall even if one isn't available right now
        doSetAttackMode(context, CARRYALLREQUESTED);

        for (auto* pUnit : dune::globals::unitList) {
            if (pUnit->getOwner() != owner_)
                continue;

            auto* carryall = dune_cast<Carryall>(pUnit);
            if (!carryall)
                continue;

            if (!carryall->isBooked()) {
                carryall->setTarget(this);

                if (!isAwaitingPickup()) // For some reason, the carryall didn't book us.
                    return false;

                carryall->clearPath();
                bookCarrier(carryall);

                // setDestination(&location);    //stop moving, and wait for carryall to arrive

                return true;
            }
        }
    }

    return false;
}

void GroundUnit::setPickedUp(const GameContext& context, UnitBase* newCarrier) {
    UnitBase::setPickedUp(context, newCarrier);
    awaitingPickup = false;
    bookedCarrier  = NONE_ID;

    clearPath(); // Stefan: I don't think this is right
                 // but there is definitely something to it
                 // <try removing this to keep tanks moving even when a carryall is coming>
}

void GroundUnit::bookCarrier(UnitBase* newCarrier) {
    if (newCarrier == nullptr) {
        bookedCarrier  = NONE_ID;
        awaitingPickup = false;
    } else {
        bookedCarrier  = newCarrier->getObjectID();
        awaitingPickup = true;
    }
}

bool GroundUnit::hasBookedCarrier() {
    if (bookedCarrier == NONE_ID)
        return false;

    if (const auto* const carrier = getCarrier()) {
        if (carrier->getTarget() == this)
            return true;

        sdl2::log_error("A ground unit's carrier has another target!");
    } else
        sdl2::log_error("A ground unit has lost its booked carrier!");

    bookCarrier(nullptr);

    return false;
}

const UnitBase* GroundUnit::getCarrier() const {
    return dune::globals::currentGame->getObjectManager().getObject<UnitBase>(bookedCarrier);
}

FixPoint GroundUnit::getTerrainDifficulty(TERRAINTYPE terrainType) const {
    switch (terrainType) {
        case Terrain_Slab: return 1.0_fix;
        case Terrain_Sand: return 1.375_fix;
        case Terrain_Rock: return 1.5625_fix;
        case Terrain_Dunes: return 1.375_fix;
        case Terrain_Mountain: return 1.0_fix;
        case Terrain_Spice: return 1.375_fix;
        case Terrain_ThickSpice: return 1.375_fix;
        case Terrain_SpiceBloom: return 1.375_fix;
        case Terrain_SpecialBloom: return 1.375_fix;
        default: return 1.0_fix;
    }
}

void GroundUnit::move(const GameContext& context) {
    if (!moving && !justStoppedMoving && (((context.game.getGameCycleCount() + getObjectID()) % 512) == 0)) {
        context.map.viewMap(owner_->getHouseID(), location_, getViewRange());
    }

    parent::move(context);
}

void GroundUnit::navigate(const GameContext& context) {
    // Lets keep units moving even if they are awaiting a pickup
    // Could potentially make this distance based depending on how
    // far away the booked carrier is
    if (!awaitingPickup) {
        parent::navigate(context);
    }
}

void GroundUnit::handleSendToRepairClick() {
    dune::globals::currentGame->getCommandManager().addCommand(
        Command(dune::globals::pLocalPlayer->getPlayerID(), CMDTYPE::CMD_UNIT_SENDTOREPAIR, objectID_));
}

void GroundUnit::doRepair(const GameContext& context) noexcept {
    if (getHealth() >= getMaxHealth())
        return;

    // find a repair yard to return to

    FixPoint closestLeastBookedRepairYardDistance = 1000000;
    const RepairYard* pBestRepairYard             = nullptr;

    for (auto* pStructure : dune::globals::structureList) {
        const auto* const pRepairYard = dune_cast<RepairYard>(pStructure);
        if (pRepairYard && (pStructure->getOwner() == owner_)) {

            if (pRepairYard->getNumBookings() == 0) {
                const auto tempDistance = blockDistance(location_, pRepairYard->getClosestPoint(location_));
                if (tempDistance < closestLeastBookedRepairYardDistance) {
                    closestLeastBookedRepairYardDistance = tempDistance;
                    pBestRepairYard                      = pRepairYard;
                }
            }
        }
    }

    if (pBestRepairYard) {
        requestCarryall(context);
        doMove2Object(context, pBestRepairYard);
    }
}
