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

#include <units/InfantryBase.h>

#include <globals.h>

#include "FileClasses/GFXManager.h"
#include <Game.h>
#include <House.h>
#include <Map.h>
#include <ScreenBorder.h>
#include <SoundPlayer.h>

#include <players/HumanPlayer.h>

#include <structures/Refinery.h>
#include <structures/RepairYard.h>
#include <structures/StructureBase.h>
#include <units/Harvester.h>

namespace {
// the position on the tile
constexpr Coord tilePositionOffset[5] = {Coord(0, 0), Coord(-TILESIZE / 4, -TILESIZE / 4),
                                         Coord(TILESIZE / 4, -TILESIZE / 4), Coord(-TILESIZE / 4, TILESIZE / 4),
                                         Coord(TILESIZE / 4, TILESIZE / 4)};
} // namespace

InfantryBase::InfantryBase(const InfantryBaseConstants& constants, uint32_t objectID,
                           const ObjectInitializer& initializer)
    : GroundUnit(constants, objectID, initializer), tilePosition(INVALID_POS), oldTilePosition(INVALID_POS) {

    InfantryBase::setHealth(getMaxHealth());
}

InfantryBase::InfantryBase(const InfantryBaseConstants& constants, uint32_t objectID,
                           const ObjectStreamInitializer& initializer)
    : GroundUnit(constants, objectID, initializer) {

    auto& stream = initializer.stream();

    tilePosition    = stream.readSint8();
    oldTilePosition = stream.readSint8();
}

InfantryBase::~InfantryBase() = default;

void InfantryBase::save(OutputStream& stream) const {

    GroundUnit::save(stream);

    stream.writeSint8(tilePosition);
    stream.writeSint8(oldTilePosition);
}

void InfantryBase::handleCaptureClick(const GameContext& context, int xPos, int yPos) {
    if (respondable_ && ((getItemID() == Unit_Soldier) || (getItemID() == Unit_Trooper))) {
        const auto* const tempTarget = context.map.tryGetObject(context, xPos, yPos);

        if (!tempTarget)
            return;

        // capture structure
        context.game.getCommandManager().addCommand(Command(dune::globals::pLocalPlayer->getPlayerID(),
                                                            CMDTYPE::CMD_INFANTRY_CAPTURE, objectID_,
                                                            tempTarget->getObjectID()));
    }
}

void InfantryBase::doCaptureStructure(const GameContext& context, uint32_t targetStructureID) {
    const auto* pStructure = context.objectManager.getObject<StructureBase>(targetStructureID);
    doCaptureStructure(context, pStructure);
}

void InfantryBase::doCaptureStructure(const GameContext& context, const StructureBase* pStructure) {

    if ((pStructure == nullptr) || (!pStructure->canBeCaptured())
        || (pStructure->getOwner()->getTeamID() == getOwner()->getTeamID())) {
        // does not exist anymore, cannot be captured or is a friendly building
        return;
    }

    doAttackObject(context, pStructure, true);
    doSetAttackMode(context, CAPTURE);
}

void InfantryBase::assignToMap(const GameContext& context, const Coord& pos) {
    auto& [game, map, objectManager] = context;

    if (auto* tile = map.tryGetTile(pos.x, pos.y)) {
        oldTilePosition = tilePosition;
        tilePosition    = tile->assignInfantry(objectManager, getObjectID());
        map.viewMap(owner_->getHouseID(), pos, getViewRange());
    }
}

void InfantryBase::blitToScreen() {
    const auto* const screenborder = dune::globals::screenborder.get();
    const auto zoom                = dune::globals::currentZoomlevel;

    const auto dest =
        calcSpriteDrawingRect(graphic_[zoom], screenborder->world2screenX(realX_), screenborder->world2screenY(realY_),
                              numImagesX_, numImagesY_, HAlign::Center, VAlign::Center);

    auto temp = drawnAngle_;
    if (temp == ANGLETYPE::UP) {
        temp = ANGLETYPE::RIGHTUP;
    } else if (temp == ANGLETYPE::DOWN) {
        temp = ANGLETYPE::LEFTUP;
    } else if (temp == ANGLETYPE::LEFTUP || temp == ANGLETYPE::LEFTDOWN || temp == ANGLETYPE::LEFT) {
        temp = ANGLETYPE::UP;
    } else {
        temp = ANGLETYPE::RIGHT;
    }

    const SDL_Rect source = calcSpriteSourceRect(graphic_[zoom], static_cast<int>(temp), numImagesX_,
                                                 (walkFrame / 10 == 3) ? 1 : walkFrame / 10, numImagesY_);

    Dune_RenderCopyF(dune::globals::renderer.get(), graphic_[zoom], &source, &dest);
}

bool InfantryBase::canPassTile(const Tile* pTile) const {
    bool passable = false;

    if (!pTile->hasAGroundObject()) {
        if (pTile->getType() != Terrain_Mountain) {
            passable = true;
        } else {
            /* if this unit is infantry so can climb, and tile can take more infantry */
            if (pTile->infantryNotFull()) {
                passable = true;
            }
        }
    } else {
        const auto* const object = pTile->getGroundObject(dune::globals::currentGame->getObjectManager());

        if ((object != nullptr) && (object->getObjectID() == target_.getObjectID()) && object->isAStructure()
            && (object->getOwner()->getTeamID() != owner_->getTeamID()) && object->isVisible(getOwner()->getTeamID())) {
            passable = true;
        } else {
            passable = (!pTile->hasANonInfantryGroundObject()
                        && (pTile->infantryNotFull()
                            && (pTile->getInfantryTeam(dune::globals::currentGame->getObjectManager())
                                == getOwner()->getTeamID())));
        }
    }

    return passable;
}

void InfantryBase::checkPos(const GameContext& context) {
    if (moving && !justStoppedMoving) {
        if (++walkFrame > 39) {
            walkFrame = 0;
        }
    }

    if (!justStoppedMoving)
        return;

    auto& [game, map, objectManager] = context;

    walkFrame = 0;

    if (auto* tile = map.tryGetTile(location_.x, location_.y)) {
        if (tile->isSpiceBloom()) {
            setHealth(0);
            tile->triggerSpiceBloom(context, getOwner());
        } else if (tile->isSpecialBloom()) {
            tile->triggerSpecialBloom(context, getOwner());
        }
    }

    const auto* const object = target_.getObjPointer();

    if (!object || !object->isAStructure())
        return;

    // check to see if close enough to blow up target
    if (getOwner()->getTeamID() == object->getOwner()->getTeamID()) {
        const auto closestPoint = object->getClosestPoint(location_);

        if (blockDistance(location_, closestPoint) <= 0.5_fix) {
            StructureBase* pCapturedStructure = target_.getStructurePointer();
            if (pCapturedStructure->getHealthColor() == COLOR_RED) {
                House* pOwner                      = pCapturedStructure->getOwner();
                const auto targetID                = pCapturedStructure->getItemID();
                const int posX                     = pCapturedStructure->getX();
                const int posY                     = pCapturedStructure->getY();
                const auto origHouse               = pCapturedStructure->getOriginalHouseID();
                const int oldHealth                = lround(pCapturedStructure->getHealth());
                const bool isSelected              = pCapturedStructure->isSelected();
                const bool isSelectedByOtherPlayer = pCapturedStructure->isSelectedByOtherPlayer();

                FixPoint capturedSpice = 0;

                UnitBase* pContainedUnit = nullptr;

                if (pCapturedStructure->getItemID() == Structure_Silo) {
                    capturedSpice = game.objectData.data[Structure_Silo][static_cast<int>(originalHouseID_)].capacity
                                  * (pOwner->getStoredCredits() / pOwner->getCapacity());
                } else if (pCapturedStructure->getItemID() == Structure_Refinery) {
                    capturedSpice = game.objectData.data[Structure_Silo][static_cast<int>(originalHouseID_)].capacity
                                  * (pOwner->getStoredCredits() / pOwner->getCapacity());
                    auto* pRefinery = static_cast<Refinery*>(pCapturedStructure);
                    if (!pRefinery->isFree()) {
                        pContainedUnit = pRefinery->getHarvester();
                    }
                } else if (pCapturedStructure->getItemID() == Structure_RepairYard) {
                    auto* pRepairYard = static_cast<RepairYard*>(pCapturedStructure);
                    if (!pRepairYard->isFree()) {
                        pContainedUnit = pRepairYard->getRepairUnit();
                    }
                }

                auto containedUnitID             = ItemID_enum::ItemID_Invalid;
                FixPoint containedUnitHealth     = 0;
                FixPoint containedHarvesterSpice = 0;
                if (pContainedUnit != nullptr) {
                    containedUnitID     = pContainedUnit->getItemID();
                    containedUnitHealth = pContainedUnit->getHealth();
                    if (containedUnitID == Unit_Harvester) {
                        containedHarvesterSpice = static_cast<Harvester*>(pContainedUnit)->getAmountOfSpice();
                    }

                    // will be destroyed by the captured structure
                    pContainedUnit = nullptr;
                }

                // remove all other infantry units capturing this building
                const auto capturedStructureLocation = pCapturedStructure->getLocation();

                { // Scope
                    std::vector<ObjectBase*> killObjects;

                    map.for_each(capturedStructureLocation.x,
                                 capturedStructureLocation.x + pCapturedStructure->getStructureSizeX(),
                                 capturedStructureLocation.y,
                                 capturedStructureLocation.y + pCapturedStructure->getStructureSizeY(),
                                 [&](auto& tile) {
                                     // make a copy of infantry list to avoid problems of modifying the list during
                                     // iteration (!)

                                     for (auto infantryID : tile.getInfantryList()) {
                                         if (infantryID == getObjectID())
                                             continue;

                                         auto* const pObject = context.objectManager.getObject(infantryID);
                                         if (pObject->getLocation() == tile.getLocation()) {
                                             killObjects.push_back(pObject);
                                         }
                                     }
                                 });

                    for (auto* pObject : killObjects)
                        pObject->destroy(context);
                }

                // destroy captured structure ... (TODO: without calling destroy()?)
                pCapturedStructure->setHealth(0);
                objectManager.removeObject(pCapturedStructure->getObjectID());

                // ... and create a new one
                auto* pNewStructure = owner_->placeStructure(NONE_ID, targetID, posX, posY, false, true);

                pNewStructure->setOriginalHouseID(origHouse);
                pNewStructure->setHealth(oldHealth);
                if (isSelected) {
                    pNewStructure->setSelected(true);
                    game.getSelectedList().insert(pNewStructure->getObjectID());
                    game.selectionChanged();
                }

                if (isSelectedByOtherPlayer) {
                    pNewStructure->setSelectedByOtherPlayer(true);
                    game.getSelectedByOtherPlayerList().insert(pNewStructure->getObjectID());
                }

                if (containedUnitID != ItemID_enum::ItemID_Invalid) {
                    auto* pNewUnit = owner_->createUnit(containedUnitID);

                    pNewUnit->setRespondable(false);
                    pNewUnit->setActive(false);
                    pNewUnit->setVisible(VIS_ALL, false);
                    pNewUnit->setHealth(containedUnitHealth);

                    auto* harvester = dune_cast<Harvester>(pNewUnit);

                    if (harvester) {
                        harvester->setAmountOfSpice(containedHarvesterSpice);
                    }

                    if (auto* pRefinery = dune_cast<Refinery>(pNewStructure)) {
                        pRefinery->book();
                        if (harvester)
                            pRefinery->assignHarvester(harvester);
                    } else if (auto* pRepairYard = dune_cast<RepairYard>(pNewUnit)) {
                        pRepairYard->book();
                        pRepairYard->assignUnit(pNewUnit);
                    }
                }

                // steal credits
                pOwner->takeCredits(capturedSpice);
                owner_->addCredits(capturedSpice, false);
                owner_->updateBuildLists();

            } else {
                const int damage = lround(std::min(pCapturedStructure->getHealth() / 2, getHealth() * 2));
                pCapturedStructure->handleDamage(context, damage, NONE_ID, getOwner());
            }
            // destroy unit indirectly
            setTarget(nullptr);
            setHealth(0);
        }
    } else if (target_.getObjPointer() != nullptr && target_.getObjPointer()->isAStructure()) {
        Coord closestPoint;
        closestPoint = target_.getObjPointer()->getClosestPoint(location_);

        if (blockDistance(location_, closestPoint) <= 0.5_fix) {
            // destroy unit indirectly
            setTarget(nullptr);
            setHealth(0);
        }
    }
}

void InfantryBase::destroy(const GameContext& context) {
    auto& [game, map, objectManager] = context;

    auto* pTile = map.tryGetTile(location_.x, location_.y);
    if (pTile && isVisible()) {

        if (pTile->hasANonInfantryGroundObject()) {
            if (const auto* object = pTile->getNonInfantryGroundObject(objectManager); object && object->isAUnit()) {
                // squashed
                pTile->assignDeadUnit(game.randomGen.randBool() ? DeadUnit_Infantry_Squashed1
                                                                : DeadUnit_Infantry_Squashed2,
                                      owner_->getHouseID(), {realX_.toFloat(), realY_.toFloat()});

                if (isVisible(getOwner()->getTeamID())) {
                    dune::globals::soundPlayer->playSoundAt(Sound_enum::Sound_Squashed, location_);
                }
            } else {
                // this unit has captured a building
            }

        } else if (getItemID() != Unit_Saboteur) {
            // "normal" dead
            pTile->assignDeadUnit(DeadUnit_Infantry, owner_->getHouseID(), {realX_.toFloat(), realY_.toFloat()});

            if (isVisible(getOwner()->getTeamID())) {
                const auto sound_id = dune::globals::pGFXManager->random().getRandOf(
                    Sound_enum::Sound_Scream1, Sound_enum::Sound_Scream2, Sound_enum::Sound_Scream3,
                    Sound_enum::Sound_Scream4, Sound_enum::Sound_Scream5, Sound_enum::Sound_Trumpet);
                dune::globals::soundPlayer->playSoundAt(sound_id, location_);
            }
        }
    }

    parent::destroy(context);
}

void InfantryBase::move(const GameContext& context) {
    if (!moving && !justStoppedMoving && (((context.game.getGameCycleCount() + getObjectID()) % 512) == 0)) {
        context.map.viewMap(owner_->getHouseID(), location_, getViewRange());
    }

    if (moving && !justStoppedMoving) {
        realX_ += xSpeed;
        realY_ += ySpeed;

        // check if unit is on the first half of the way
        FixPoint fromDistanceX;
        FixPoint fromDistanceY;
        FixPoint toDistanceX;
        FixPoint toDistanceY;

        const FixPoint epsilon = 3.75_fix;

        if (location_ != nextSpot) {
            const auto abstractDistanceX =
                FixPoint::abs(location_.x * TILESIZE + TILESIZE / 2 - (realX_ - bumpyOffsetX));
            const auto abstractDistanceY =
                FixPoint::abs(location_.y * TILESIZE + TILESIZE / 2 - (realY_ - bumpyOffsetY));

            fromDistanceX = FixPoint::abs(location_.x * TILESIZE + TILESIZE / 2 + tilePositionOffset[oldTilePosition].x
                                          - (realX_ - bumpyOffsetX));
            fromDistanceY = FixPoint::abs(location_.y * TILESIZE + TILESIZE / 2 + tilePositionOffset[oldTilePosition].y
                                          - (realY_ - bumpyOffsetY));
            toDistanceX   = FixPoint::abs(nextSpot.x * TILESIZE + TILESIZE / 2 + tilePositionOffset[tilePosition].x
                                          - (realX_ - bumpyOffsetX));
            toDistanceY   = FixPoint::abs(nextSpot.y * TILESIZE + TILESIZE / 2 + tilePositionOffset[tilePosition].y
                                          - (realY_ - bumpyOffsetY));

            // check if unit is half way out of old tile
            if ((abstractDistanceX >= TILESIZE / 2 + epsilon) || (abstractDistanceY >= TILESIZE / 2 + epsilon)) {
                // let something else go in
                unassignFromMap(location_);
                oldLocation_ = location_;
                location_    = nextSpot;

                context.map.viewMap(owner_->getHouseID(), location_, getViewRange());
            }

        } else {
            fromDistanceX = FixPoint::abs(oldLocation_.x * TILESIZE + TILESIZE / 2
                                          + tilePositionOffset[oldTilePosition].x - (realX_ - bumpyOffsetX));
            fromDistanceY = FixPoint::abs(oldLocation_.y * TILESIZE + TILESIZE / 2
                                          + tilePositionOffset[oldTilePosition].y - (realY_ - bumpyOffsetY));
            toDistanceX   = FixPoint::abs(location_.x * TILESIZE + TILESIZE / 2 + tilePositionOffset[tilePosition].x
                                          - (realX_ - bumpyOffsetX));
            toDistanceY   = FixPoint::abs(location_.y * TILESIZE + TILESIZE / 2 + tilePositionOffset[tilePosition].y
                                          - (realY_ - bumpyOffsetY));

            Coord wantedReal;
            wantedReal.x = nextSpot.x * TILESIZE + TILESIZE / 2 + tilePositionOffset[tilePosition].x;
            wantedReal.y = nextSpot.y * TILESIZE + TILESIZE / 2 + tilePositionOffset[tilePosition].y;

            if ((FixPoint::abs(wantedReal.x - (realX_ - bumpyOffsetX)) <= FixPoint::abs(xSpeed) / 2 + epsilon)
                && (FixPoint::abs(wantedReal.y - (realY_ - bumpyOffsetY)) <= FixPoint::abs(ySpeed) / 2 + epsilon)) {
                realX_       = wantedReal.x;
                realY_       = wantedReal.y;
                bumpyOffsetX = 0;
                bumpyOffsetY = 0;

                if (forced_ && (location_ == destination_) && !target_) {
                    setForced(false);
                }

                moving            = false;
                justStoppedMoving = true;

                oldLocation_.invalidate();
            }
        }

        bumpyMovementOnRock(fromDistanceX, fromDistanceY, toDistanceX, toDistanceY);

    } else {
        justStoppedMoving = false;
    }

    checkPos(context);
}

void InfantryBase::setLocation(const GameContext& context, int xPos, int yPos) {
    if (context.map.tileExists(xPos, yPos) || ((xPos == INVALID_POS) && (yPos == INVALID_POS))) {
        oldTilePosition = tilePosition = INVALID_POS;
        parent::setLocation(context, xPos, yPos);

        if (tilePosition != INVALID_POS) {
            realX_ += tilePositionOffset[tilePosition].x;
            realY_ += tilePositionOffset[tilePosition].y;
        }
    }
}

void InfantryBase::setSpeeds(const GameContext& context) {
    if (oldTilePosition == INVALID_POS) {
        sdl2::log_info("Warning: InfantryBase::setSpeeds(context): Infantry tile position == INVALID_POS.");
    } else if (tilePosition == oldTilePosition) {
        // haven't changed infantry position
        parent::setSpeeds(context);
    } else {

        const int sx = tilePositionOffset[oldTilePosition].x;
        const int sy = tilePositionOffset[oldTilePosition].y;

        int dx = 0;
        int dy = 0;
        // clang-format off
        switch(drawnAngle_) {
            case ANGLETYPE::RIGHT:     dx += TILESIZE;                 break;
            case ANGLETYPE::RIGHTUP:   dx += TILESIZE; dy -= TILESIZE; break;
            case ANGLETYPE::UP:                        dy -= TILESIZE; break;
            case ANGLETYPE::LEFTUP:    dx -= TILESIZE; dy -= TILESIZE; break;
            case ANGLETYPE::LEFT:      dx -= TILESIZE;                 break;
            case ANGLETYPE::LEFTDOWN:  dx -= TILESIZE; dy += TILESIZE; break;
            case ANGLETYPE::DOWN:                      dy += TILESIZE; break;
            case ANGLETYPE::RIGHTDOWN: dx += TILESIZE; dy += TILESIZE; break;
        }
        // clang-format on

        if (tilePosition != INVALID_POS) {
            dx += tilePositionOffset[tilePosition].x;
            dy += tilePositionOffset[tilePosition].y;
        }

        dx -= sx;
        dy -= sy;

        const FixPoint scale = context.game.objectData.data[itemID_][static_cast<int>(originalHouseID_)].maxspeed
                             / FixPoint::sqrt((dx * dx + dy * dy));
        xSpeed = dx * scale;
        ySpeed = dy * scale;
    }
}

void InfantryBase::squash(const GameContext& context) {
    destroy(context);
}

void InfantryBase::playConfirmSound() {
    dune::globals::soundPlayer->playVoice(
        dune::globals::pGFXManager->random().getRandOf(Voice_enum::MovingOut, Voice_enum::InfantryOut),
        getOwner()->getHouseID());
}

void InfantryBase::playSelectSound() {
    dune::globals::soundPlayer->playVoice(Voice_enum::YesSir, getOwner()->getHouseID());
}
