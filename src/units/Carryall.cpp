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

#include <units/Carryall.h>

#include <globals.h>

#include "mmath.h"
#include <FileClasses/GFXManager.h>
#include <FileClasses/SFXManager.h>
#include <Game.h>
#include <House.h>
#include <Map.h>
#include <SoundPlayer.h>

#include <structures/ConstructionYard.h>
#include <structures/Refinery.h>
#include <structures/RepairYard.h>
#include <units/Harvester.h>

namespace {
constexpr AirUnitConstants carryall_constants{Carryall::item_id};
} // namespace

Carryall::Carryall(uint32_t objectID, const ObjectInitializer& initializer)
    : AirUnit(carryall_constants, objectID, initializer) {
    Carryall::init();

    ObjectBase::setHealth(getMaxHealth());

    respondable_ = false;
}

Carryall::Carryall(uint32_t objectID, const ObjectStreamInitializer& initializer)
    : AirUnit(carryall_constants, objectID, initializer) {
    Carryall::init();

    auto& stream = initializer.stream();

    pickedUpUnitList = stream.readUint32Vector();
    if (!pickedUpUnitList.empty()) {
        drawnFrame = 1;
    }

    stream.readBools(&owned, &aDropOfferer, &droppedOffCargo);
}

void Carryall::init() {
    assert(itemID_ == Unit_Carryall);
    owner_->incrementUnits(itemID_);

    const auto* const gfx = dune::globals::pGFXManager.get();

    graphicID_    = ObjPic_Carryall;
    graphic_      = gfx->getObjPic(graphicID_, getOwner()->getHouseID());
    shadowGraphic = gfx->getObjPic(ObjPic_CarryallShadow, getOwner()->getHouseID());

    numImagesX_ = NUM_ANGLES;
    numImagesY_ = 2;
}

Carryall::~Carryall() = default;

void Carryall::save(OutputStream& stream) const {
    AirUnit::save(stream);

    stream.writeUint32Vector(pickedUpUnitList);

    stream.writeBools(owned, aDropOfferer, droppedOffCargo);
}

bool Carryall::update(const GameContext& context) {
    const auto& maxSpeed = context.game.objectData.data[itemID_][static_cast<int>(originalHouseID_)].maxspeed;

    FixPoint dist             = -1;
    const auto* const pTarget = target_.getObjPointer();
    if (pTarget != nullptr && pTarget->isAUnit()) {
        dist = distanceFrom(realX_, realY_, pTarget->getRealX(), pTarget->getRealY());
    } else if ((pTarget != nullptr) || hasCargo()) {
        dist = distanceFrom(realX_, realY_, destination_.x * TILESIZE + TILESIZE / 2,
                            destination_.y * TILESIZE + TILESIZE / 2);
    }

    if (dist >= 0) {
        static constexpr FixPoint minSpeed = FixPoint32(TILESIZE / 32);
        if (dist < TILESIZE / 2) {
            currentMaxSpeed = std::min(dist, minSpeed);
        } else if (dist >= 10 * TILESIZE) {
            currentMaxSpeed = maxSpeed;
        } else {
            const FixPoint m = (maxSpeed - minSpeed) / ((10 * TILESIZE) - (TILESIZE / 2));
            const FixPoint t = minSpeed - (TILESIZE / 2) * m;
            currentMaxSpeed  = dist * m + t;
        }
    } else {
        currentMaxSpeed = std::min(currentMaxSpeed + 0.2_fix, maxSpeed);
    }

    if (!AirUnit::update(context)) {
        return false;
    }

    // check if this carryall has to be removed because it has just brought something
    // to the map (e.g. new harvester)
    if (active_) {
        const auto& map = context.map;
        if (aDropOfferer && droppedOffCargo && (!hasCargo())
            && ((getRealX() < -TILESIZE) || (getRealX() > (map.getSizeX() + 1) * TILESIZE) || (getRealY() < -TILESIZE)
                || (getRealY() > (map.getSizeY() + 1) * TILESIZE))) {
            setVisible(VIS_ALL, false);
            destroy(context);
            return false;
        }
    }
    return true;
}

void Carryall::deploy(const GameContext& context, const Coord& newLocation) {
    parent::deploy(context, newLocation);

    respondable_ = false;
}

void Carryall::checkPos(const GameContext& context) {
    parent::checkPos(context);

    if (!active_)
        return;

    const auto& [game, map, objectManager] = context;

    if (hasCargo()) {
        if ((location_ == destination_) && (currentMaxSpeed <= 0.5_fix)) {
            // drop up to 3 infantry units at once or one other unit
            auto droppedUnits = 0;
            do {
                const auto unitID       = pickedUpUnitList.front();
                const auto* const pUnit = objectManager.getObject<UnitBase>(unitID);

                if (pUnit == nullptr) {
                    return;
                }

                if ((!pUnit->isInfantry()) && (droppedUnits > 0)) {
                    // we already dropped infantry and this is no infantry
                    // => do not drop this here
                    break;
                }

                deployUnit(context, unitID);
                droppedUnits++;

                if (!pUnit->isInfantry()) {
                    // we dropped a non infantry unit
                    // => do not drop another unit
                    break;
                }
            } while (hasCargo() && (droppedUnits < 3));

            if (!pickedUpUnitList.empty()) {
                // find next place to drop
                for (auto i = 8; i < 18; i++) {
                    const auto r     = game.randomGen.rand(3, i / 2);
                    const auto angle = 2 * FixPt_PI * game.randomGen.randFixPoint();

                    auto dropCoord =
                        location_ + Coord(lround(r * FixPoint::sin(angle)), lround(-r * FixPoint::cos(angle)));
                    if (map.tileExists(dropCoord) && !map.getTile(dropCoord)->hasAGroundObject()) {
                        setDestination(dropCoord);
                        break;
                    }
                }
            } else {
                setTarget(nullptr);
                setDestination(guardPoint);
            }
        }
    } else if (!isBooked()) {
        if (destination_.isValid()) {
            if (blockDistance(location_, destination_) <= 2) {
                destination_.invalidate();
            }
        } else {
            if (blockDistance(location_, guardPoint) > 17) {
                setDestination(guardPoint);
            }
        }
    }
}

void Carryall::pre_deployUnits(const GameContext& context) {
    dune::globals::soundPlayer->playSoundAt(Sound_enum::Sound_Drop, location_);

    currentMaxSpeed = 0;
    setSpeeds(context);
}

void Carryall::deployUnit(const GameContext& context, uint32_t unitID) {
    const auto iter = std::ranges::find(std::as_const(pickedUpUnitList), unitID);

    if (pickedUpUnitList.cend() == iter)
        return;

    pickedUpUnitList.erase(iter);

    auto* const pUnit = context.objectManager.getObject<UnitBase>(unitID);

    if (pUnit == nullptr)
        return;

    pre_deployUnits(context);

    if (auto* const tile = context.map.tryGetTile(location_.x, location_.y))
        deployUnit(context, tile, pUnit);
    else
        sdl2::log_error(SDL_LOG_CATEGORY_APPLICATION, "Carryall deploy failed for location %d, %d", location_.x,
                        location_.y);

    post_deployUnits();
}

void Carryall::deployUnit(const GameContext& context, Tile* tile, UnitBase* pUnit) {
    if (tile->hasANonInfantryGroundObject()) {
        auto* const object = tile->getNonInfantryGroundObject(context.objectManager);
        if (object && object->getOwner() == getOwner()) {
            if (auto* const repair_yard = dune_cast<RepairYard>(object)) {

                if (repair_yard->isFree()) {
                    pUnit->setTarget(object); // unit books repair yard again
                    pUnit->setGettingRepaired();

                    return;
                }
                // unit is still going to repair yard but was unbooked from repair yard at pickup => book now

                repair_yard->book();
            } else if (const auto* const refinery = dune_cast<Refinery>(object)) {
                if (refinery->isFree()) {
                    if (auto* const harvester = dune_cast<Harvester>(pUnit)) {
                        harvester->setTarget(object);
                        harvester->setReturned(context);
                        goingToRepairYard = false;

                        return;
                    }
                }
            }
        }
    }

    pUnit->setAngle(drawnAngle_);
    const auto deployPos = context.map.findDeploySpot(pUnit, location_);
    pUnit->setForced(false); // Stop units being forced if they are deployed
    pUnit->deploy(context, deployPos);
    if (pUnit->getItemID() == Unit_Saboteur) {
        pUnit->doSetAttackMode(context, HUNT);
    } else if (pUnit->getItemID() != Unit_Harvester) {
        pUnit->doSetAttackMode(context, AREAGUARD);
    } else {
        pUnit->doSetAttackMode(context, HARVEST);
    }
}

void Carryall::post_deployUnits() {
    if (!pickedUpUnitList.empty())
        return;

    if (!aDropOfferer) {
        setTarget(nullptr);
        setDestination(guardPoint);
    }
    droppedOffCargo = true;
    drawnFrame      = 0;

    clearPath();
}

void Carryall::destroy(const GameContext& context) {
    // destroy cargo
    for (const auto pickedUpUnitID : pickedUpUnitList) {
        if (auto* const pPickedUpUnit = context.objectManager.getObject<UnitBase>(pickedUpUnitID)) {
            pPickedUpUnit->destroy(context);
        }
    }
    pickedUpUnitList.clear();

    // place wreck
    if (isVisible()) {
        if (auto* const pTile = context.map.tryGetTile(location_.x, location_.y)) {
            pTile->assignDeadUnit(DeadUnit_Carryall, owner_->getHouseID(), {realX_.toFloat(), realY_.toFloat()});
        }
    }

    parent::destroy(context);
}

void Carryall::releaseTarget() {
    setTarget(nullptr);

    if (!hasCargo())
        setDestination(guardPoint);
}

void Carryall::engageTarget(const GameContext& context) {
    if (!target_) {
        assert(!hasCargo());
        return;
    }

    auto* object = target_.getObjPointer();
    if (object == nullptr) {
        // the target does not exist anymore
        releaseTarget();
        return;
    }

    if (!object->isActive()) {
        // the target changed its state to inactive
        releaseTarget();
        return;
    }

    if (const auto* groundUnit = dune_cast<GroundUnit>(object); groundUnit && !groundUnit->isAwaitingPickup()) {
        // the target changed its state to not awaiting pickup anymore
        releaseTarget();
        return;
    }

    if (object->getOwner()->getTeamID() != owner_->getTeamID()) {
        // the target changed its owner_ (e.g. was deviated)
        releaseTarget();
        return;
    }

    Coord targetLocation;
    if (object->getItemID() == Structure_Refinery) {
        targetLocation = object->getLocation() + Coord(2, 0);
    } else {
        targetLocation = object->getClosestPoint(location_);
    }

    const Coord realLocation    = Coord(lround(realX_), lround(realY_));
    const Coord realDestination = targetLocation * TILESIZE + Coord(TILESIZE / 2, TILESIZE / 2);

    targetDistance = distanceFrom(realLocation, realDestination);

    if (targetDistance <= TILESIZE / 32) {
        if (hasCargo()) {
            if (object->isAStructure()) {
                while (!pickedUpUnitList.empty()) {
                    deployUnit(context, pickedUpUnitList.back());
                }

                setTarget(nullptr);
                setDestination(guardPoint);
            }
        } else {
            pickupTarget(context);
        }
    } else {
        setDestination(targetLocation);
    }
}

void Carryall::giveCargo(const GameContext& context, UnitBase* newUnit) {
    if (newUnit == nullptr) {
        return;
    }

    pickedUpUnitList.push_back(newUnit->getObjectID());

    newUnit->setPickedUp(context, this);

    drawnFrame = 1;

    droppedOffCargo = false;
}

void Carryall::pickupTarget(const GameContext& context) {
    currentMaxSpeed = 0;
    setSpeeds(context);

    auto* const pTarget = target_.getObjPointer();
    if (!pTarget)
        return;

    if (auto* pGroundUnitTarget = dune_cast<GroundUnit>(pTarget)) {

        if (pTarget->getHealth() <= 0) {
            // unit died just in the moment we tried to pick it up => carryall also crushes
            setHealth(0);
            return;
        }

        if (pGroundUnitTarget->hasATarget() || (pGroundUnitTarget->getDestination() != pGroundUnitTarget->getLocation())
            || pGroundUnitTarget->isBadlyDamaged()) {

            if (pGroundUnitTarget->isBadlyDamaged()
                || (!pGroundUnitTarget->hasATarget() && pGroundUnitTarget->getItemID() != Unit_Harvester)) {
                pGroundUnitTarget->doRepair(context);
            }

            const auto* newTarget = pGroundUnitTarget->hasATarget() ? pGroundUnitTarget->getTarget() : nullptr;

            pickedUpUnitList.push_back(target_.getObjectID());
            pGroundUnitTarget->setPickedUp(context, this);

            drawnFrame = 1;

            if (newTarget && (newTarget->getItemID() == Structure_Refinery)) {
                pGroundUnitTarget->setGuardPoint(pGroundUnitTarget->getLocation());
                setTarget(newTarget);
                setDestination(target_.getObjPointer()->getLocation() + Coord(2, 0));
            } else if (newTarget && (newTarget->getItemID() == Structure_RepairYard)) {
                pGroundUnitTarget->setGuardPoint(pGroundUnitTarget->getLocation());
                setTarget(newTarget);
                setDestination(target_.getObjPointer()->getClosestPoint(location_));
            } else if (pGroundUnitTarget->getDestination().isValid()) {
                setDestination(pGroundUnitTarget->getDestination());
            }

            clearPath();

        } else {
            pGroundUnitTarget->setAwaitingPickup(false);
            if (pGroundUnitTarget->getAttackMode() == CARRYALLREQUESTED) {
                pGroundUnitTarget->doSetAttackMode(context, STOP);
            }
            releaseTarget();
        }
    } else {
        // get unit from structure
        if (auto* refinery = dune_cast<Refinery>(pTarget)) {
            // get harvester
            refinery->deployHarvester(context, this);
        } else if (auto* repairYard = dune_cast<RepairYard>(pTarget)) {
            // get repaired unit
            repairYard->deployRepairUnit(context, this);
        }
    }
}

void Carryall::setTarget(const ObjectBase* newTarget) {
    if (auto* const pTarget = target_.getObjPointer()) {
        if (targetFriendly_) {
            if (auto* groundUnit = dune_cast<GroundUnit>(pTarget)) {
                if (groundUnit->getCarrier() == this)
                    groundUnit->bookCarrier(nullptr);
            }
        }

        if (auto* refinery = dune_cast<Refinery>(pTarget))
            refinery->unBook();
    }

    parent::setTarget(newTarget);

    auto* const pTarget = target_.getObjPointer();
    if (!pTarget)
        return;

    if (auto* refinery = dune_cast<Refinery>(pTarget))
        refinery->book();

    if (targetFriendly_) {
        if (auto* groundUnit = dune_cast<GroundUnit>(pTarget))
            groundUnit->setAwaitingPickup(true);
    }
}

void Carryall::targeting(const GameContext& context) {
    if (target_)
        engageTarget(context);
}

void Carryall::turn(const GameContext& context) {
    const auto& map = context.map;

    if (active_ && aDropOfferer && droppedOffCargo && (!hasCargo())
        && ((getRealX() < TILESIZE / 2) || (getRealX() > map.getSizeX() * TILESIZE - TILESIZE / 2)
            || (getRealY() < TILESIZE / 2) || (getRealY() > map.getSizeY() * TILESIZE - TILESIZE / 2))) {
        // already partially outside the map => do not turn
        return;
    }

    parent::turn(context);
}
