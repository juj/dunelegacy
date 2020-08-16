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

#include <ObjectBase.h>

#include <Game.h>
#include <House.h>
#include <Map.h>
#include <players/HumanPlayer.h>

#include <engine_mmath.h>

//structures
#include <structures/Barracks.h>
#include <structures/ConstructionYard.h>
#include <structures/GunTurret.h>
#include <structures/HeavyFactory.h>
#include <structures/HighTechFactory.h>
#include <structures/IX.h>
#include <structures/LightFactory.h>
#include <structures/Palace.h>
#include <structures/Radar.h>
#include <structures/Refinery.h>
#include <structures/RepairYard.h>
#include <structures/RocketTurret.h>
#include <structures/Silo.h>
#include <structures/StarPort.h>
#include <structures/Wall.h>
#include <structures/WindTrap.h>
#include <structures/WOR.h>

//units
#include <units/Carryall.h>
#include <units/Devastator.h>
#include <units/Deviator.h>
#include <units/Frigate.h>
#include <units/Harvester.h>
#include <units/Launcher.h>
#include <units/MCV.h>
#include <units/Ornithopter.h>
#include <units/Quad.h>
#include <units/RaiderTrike.h>
#include <units/Saboteur.h>
#include <units/SandWorm.h>
#include <units/SiegeTank.h>
#include <units/Soldier.h>
#include <units/SonicTank.h>
#include <units/Tank.h>
#include <units/Trike.h>
#include <units/Trooper.h>

#include <array>

namespace Dune::Engine {

ObjectBase::ObjectBase(const ObjectBaseConstants& object_constants, uint32_t objectID,
                       const ObjectInitializer& initializer)
    : ObjectBase(object_constants, objectID) {
    originalHouseID = initializer.owner()->getHouseID();
    owner           = initializer.owner();
    byScenario      = initializer.byScenario();

    health       = 0;
    badlyDamaged = false;

    location    = Coord::Invalid();
    oldLocation = Coord::Invalid();
    destination = Coord::Invalid();
    realX       = 0;
    realY       = 0;

    drawnAngle = static_cast<ANGLETYPE>(0);
    angle      = static_cast<int>(drawnAngle);

    active                = false;
    respondable           = true;
    byScenario            = false;

    forced = false;
    targetFriendly = false;
    attackMode     = GUARD;

    setVisible(VIS_ALL, false);
}

ObjectBase::ObjectBase(const ObjectBaseConstants& object_constants, uint32_t objectID,
                       const ObjectStreamInitializer& initializer)
    : ObjectBase(object_constants, objectID) {
    auto&  game     = initializer.game();
    auto& stream    = initializer.stream();

    originalHouseID = static_cast<HOUSETYPE>(stream.readUint32());
    owner           = game.getHouse(static_cast<HOUSETYPE>(stream.readUint32()));

    health       = stream.readFixPoint();
    badlyDamaged = stream.readBool();

    location.x    = stream.readSint32();
    location.y    = stream.readSint32();
    oldLocation.x = stream.readSint32();
    oldLocation.y = stream.readSint32();
    destination.x = stream.readSint32();
    destination.y = stream.readSint32();
    realX         = stream.readFixPoint();
    realY         = stream.readFixPoint();

    angle      = stream.readFixPoint();
    drawnAngle = static_cast<ANGLETYPE>(stream.readSint8());

    active      = stream.readBool();
    respondable = stream.readBool();
    byScenario  = stream.readBool();

    forced = stream.readBool();
    target.load(stream);
    targetFriendly = stream.readBool();
    attackMode     = static_cast<ATTACKMODE>(stream.readUint32());

    std::array<bool, 7> b{false, false, false, false, false, false, false};

    stream.readBools(&b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6]);

    for(decltype(visible.size()) i = 0; i < visible.size(); ++i)
        visible[i] = b[i];
}

ObjectBase::ObjectBase(const ObjectBaseConstants& object_constants, uint32_t objectID)
    : constants_{object_constants}, itemID{object_constants.itemID}, objectID{objectID} { }

ObjectBase::~ObjectBase() = default;

void ObjectBase::destroy(const GameContext& context) { context.objectManager.removeObject(getObjectID()); }

void ObjectBase::cleanup(const GameContext& context, HumanPlayer* humanPlayer) { }

void ObjectBase::save(const Game& game, OutputStream& stream) const {
    stream.writeUint32(static_cast<uint32_t>(originalHouseID));
    stream.writeUint32(static_cast<uint32_t>(owner->getHouseID()));

    stream.writeFixPoint(health);
    stream.writeBool(badlyDamaged);

    stream.writeSint32(location.x);
    stream.writeSint32(location.y);
    stream.writeSint32(oldLocation.x);
    stream.writeSint32(oldLocation.y);
    stream.writeSint32(destination.x);
    stream.writeSint32(destination.y);
    stream.writeFixPoint(realX);
    stream.writeFixPoint(realY);

    stream.writeFixPoint(angle);
    stream.writeSint8(static_cast<int8_t>(drawnAngle));

    stream.writeBool(active);
    stream.writeBool(respondable);
    stream.writeBool(byScenario);

    stream.writeBool(forced);
    target.save(stream);
    stream.writeBool(targetFriendly);
    stream.writeUint32(attackMode);

    stream.writeBools(visible[0], visible[1], visible[2], visible[3], visible[4], visible[5], visible[6]);
}

/**
    Returns the center point of this object
    \return the center point in world coordinates
*/
Coord ObjectBase::getCenterPoint() const { return Coord(lround(realX), lround(realY)); }

Coord ObjectBase::getClosestCenterPoint(const Coord& objectLocation) const { return getCenterPoint(); }

int ObjectBase::getMaxHealth(const Game& game) const {
    return game.objectData.data[itemID][static_cast<int>(originalHouseID)].hitpoints;
}

void ObjectBase::handleDamage(const GameContext& context, int damage, uint32_t damagerID, House* damagerOwner) {
    if(damage >= 0) {
        FixPoint newHealth = getHealth();

        newHealth -= damage;

        if(newHealth <= 0) {
            setHealth(context.game, 0);

            if(damagerOwner != nullptr) { damagerOwner->informHasKilled(itemID); }
        } else {
            setHealth(context.game, newHealth);
        }
    }

    getOwner()->noteDamageLocation(this, damage, damagerID);
}

void ObjectBase::setDestination(const GameContext& context, int newX, int newY) {
    if(context.map.tileExists(newX, newY) || ((newX == INVALID_POS) && (newY == INVALID_POS))) {
        destination.x = newX;
        destination.y = newY;
    }
}

void ObjectBase::setHealth(const Game& game, FixPoint newHealth) {
    const auto max_health = getMaxHealth(game);

    if((newHealth >= 0) && (newHealth <= max_health)) {
        health       = newHealth;
        badlyDamaged = health < BADLYDAMAGEDRATIO * max_health;
    }
}

void ObjectBase::setLocation(const GameContext& context, int xPos, int yPos) {
    if((xPos == INVALID_POS) && (yPos == INVALID_POS)) {
        location.invalidate();
    } else if(context.map.tileExists(xPos, yPos)) {
        location.x = xPos;
        location.y = yPos;
        realX      = location.x * TILESIZE;
        realY      = location.y * TILESIZE;

        assignToMap(context, location);
    }
}

void ObjectBase::setVisible(int teamID, bool status) {
    if(teamID == VIS_ALL) {
        if(status) {
            visible.set();
        } else {
            visible.reset();
        }
    } else if((teamID >= 0) && (teamID < NUM_TEAMS)) {
        visible[teamID] = status;
    }
}

void ObjectBase::setTarget(const ObjectManager& objectManager, const ObjectBase* newTarget) {
    target.pointTo(const_cast<ObjectBase*>(newTarget));

    auto friendly = false;

    if(target) {
        if(auto* targetPtr = target.getObjPointer(objectManager)) {
            friendly = (targetPtr->getOwner()->getTeamID() == owner->getTeamID()) && (getItemID() != Unit_Sandworm) &&
                       (targetPtr->getItemID() != Unit_Sandworm);
        }
    }

    targetFriendly = friendly;
}

void ObjectBase::unassignFromMap(const GameContext& context, const Coord& location) const {
    auto* tile = context.map.tryGetTile(location.x, location.y);

    if(!tile) return;

    tile->unassignObject(getObjectID());
}

bool ObjectBase::canAttack(const GameContext& context, const ObjectBase* object) const {
    return canAttack() && (object != nullptr) && (object->isAStructure() || !object->isAFlyingUnit()) &&
           (((object->getOwner()->getTeamID() != owner->getTeamID()) && object->isVisible(getOwner()->getTeamID())) ||
            (object->getItemID() == Unit_Sandworm));
}

bool ObjectBase::isVisible(int teamID) const {
    if(visible.all()) return true;

    if((teamID >= 0) && (teamID < NUM_TEAMS)) { return visible[teamID]; }
    return false;
}

bool ObjectBase::isVisible() const { return visible.any(); }

Coord ObjectBase::getClosestPoint(const Coord& point) const { return location; }

const StructureBase* ObjectBase::findClosestTargetStructure(const GameContext& context) const {
    const StructureBase* pClosestStructure = nullptr;
    auto                 closestDistance   = FixPt_MAX;
    for(const StructureBase* pStructure : context.game.structureList) {
        if(canAttack(context, pStructure)) {
            const auto closestPoint      = pStructure->getClosestPoint(getLocation());
            auto       structureDistance = blockDistance(getLocation(), closestPoint);

            if(pStructure->getItemID() == Structure_Wall) {
                structureDistance += 20000000; // so that walls are targeted very last
            }

            if(structureDistance < closestDistance) {
                closestDistance   = structureDistance;
                pClosestStructure = pStructure;
            }
        }
    }

    return pClosestStructure;
}

const UnitBase* ObjectBase::findClosestTargetUnit(const GameContext& context) const {
    const UnitBase* pClosestUnit    = nullptr;
    auto            closestDistance = FixPt_MAX;
    for(const UnitBase* pUnit : context.game.unitList) {
        if(canAttack(context, pUnit)) {
            const auto closestPoint = pUnit->getClosestPoint(getLocation());
            const auto unitDistance = blockDistance(getLocation(), closestPoint);

            if(unitDistance < closestDistance) {
                closestDistance = unitDistance;
                pClosestUnit    = pUnit;
            }
        }
    }

    return pClosestUnit;
}

const ObjectBase* ObjectBase::findClosestTarget(const GameContext& context) const {
    const ObjectBase* pClosestObject  = nullptr;
    FixPoint          closestDistance = FixPt_MAX;
    for(const StructureBase* pStructure : context.game.structureList) {
        if(canAttack(context, pStructure)) {
            const auto closestPoint      = pStructure->getClosestPoint(getLocation());
            auto       structureDistance = blockDistance(getLocation(), closestPoint);

            if(pStructure->getItemID() == Structure_Wall) {
                structureDistance += 20000000; // so that walls are targeted very last
            }

            if(structureDistance < closestDistance) {
                closestDistance = structureDistance;
                pClosestObject  = pStructure;
            }
        }
    }

    for(const UnitBase* pUnit : context.game.unitList) {
        if(canAttack(context, pUnit)) {
            const auto closestPoint = pUnit->getClosestPoint(getLocation());
            const auto unitDistance = blockDistance(getLocation(), closestPoint);

            if(unitDistance < closestDistance) {
                closestDistance = unitDistance;
                pClosestObject  = pUnit;
            }
        }
    }

    return pClosestObject;
}

const ObjectBase* ObjectBase::findTarget(const GameContext& context) const {
    // searches for a target in an area like as shown below
    //
    //                    *
    //                  *****
    //                  *****
    //                 ***T***
    //                  *****
    //                  *****
    //                    *

    auto checkRange = 0;
    switch(attackMode) {
        case GUARD: {
            checkRange = getWeaponRange(context.game);
        } break;

        case AREAGUARD: {
            checkRange = getAreaGuardRange(context.game);
        } break;

        case AMBUSH: {
            checkRange = getViewRange(context.game);
        } break;

        case HUNT: {
            // check whole map
            return findClosestTarget(context);
        } break;

        case STOP:
        default: {
            return nullptr;
        } break;
    }

    if(getItemID() == Unit_Sandworm) { checkRange = getViewRange(context.game); }

    ObjectBase* pClosestTarget        = nullptr;
    auto        closestTargetDistance = FixPt_MAX;

    Coord      coord;
    const auto startY = std::max(0, location.y - checkRange);
    const auto endY   = std::min(context.map.getSizeY() - 1, location.y + checkRange);
    for(coord.y = startY; coord.y <= endY; coord.y++) {
        const auto startX = std::max(0, location.x - checkRange);
        const auto endX   = std::min(context.map.getSizeX() - 1, location.x + checkRange);
        for(coord.x = startX; coord.x <= endX; coord.x++) {

            const auto targetDistance = blockDistance(location, coord);
            if(targetDistance <= checkRange) {
                Tile* pTile = context.map.getTile(coord);
                if(pTile->isExploredByTeam(&context.game, getOwner()->getTeamID()) &&
                   !pTile->isFoggedByTeam(&context.game, getOwner()->getTeamID()) && pTile->hasAnObject()) {

                    auto* const pNewTarget = pTile->getObject(context.objectManager);
                    if(!pNewTarget) continue;

                    if(((pNewTarget->getItemID() != Structure_Wall && pNewTarget->getItemID() != Unit_Carryall) ||
                        pClosestTarget == nullptr) &&
                        canAttack(context, pNewTarget)) {
                        if(targetDistance < closestTargetDistance) {
                            pClosestTarget        = pNewTarget;
                            closestTargetDistance = targetDistance;
                        }
                    }
                }
            }
        }
    }

    return pClosestTarget;
}

int ObjectBase::getViewRange(const Game& game) const {
    return game.getObjectData(itemID, originalHouseID).viewrange;
}

int ObjectBase::getAreaGuardRange(const Game& game) const { return 2 * getWeaponRange(game); }

int ObjectBase::getWeaponRange(const Game& game) const {
    return game.getObjectData(itemID, originalHouseID).weaponrange;
}

int ObjectBase::getWeaponReloadTime(const Game& game) const {
    return game.getObjectData(itemID, originalHouseID).weaponreloadtime;
}

int ObjectBase::getInfSpawnProp(const Game& game) const {
    return game.getObjectData(itemID, originalHouseID).infspawnprop;
}

namespace {
template<typename ObjectType, typename... Args>
std::unique_ptr<ObjectBase> makeObject(Args&&... args) {
    static_assert(std::is_constructible<ObjectType, Args...>::value, "ObjectType is not constructible");
    static_assert(std::is_base_of<ObjectBase, ObjectType>::value, "ObjectType not derived from ObjectBase");
    static_assert(std::is_base_of<ObjectBase, typename ObjectType::parent>::value,
                  "ObjectType's parent is not derived from ObjectBase");
    static_assert(std::is_base_of<typename ObjectType::parent, ObjectType>::value,
                  "ObjectType's parent is not a base class");

    return std::make_unique<ObjectType>(std::forward<Args>(args)...);
}

template<typename... Args>
auto objectFactory(ItemID_enum itemID, Args&&... args) {
    // clang-format off
    switch(itemID) {
        case Barracks::item_id:            return makeObject<Barracks>(args...);
        case ConstructionYard::item_id:    return makeObject<ConstructionYard>(args...);
        case GunTurret::item_id:           return makeObject<GunTurret>(args...);
        case HeavyFactory::item_id:        return makeObject<HeavyFactory>(args...);
        case HighTechFactory::item_id:     return makeObject<HighTechFactory>(args...);
        case IX::item_id:                  return makeObject<IX>(args...);
        case LightFactory::item_id:        return makeObject<LightFactory>(args...);
        case Palace::item_id:              return makeObject<Palace>(args...);
        case Radar::item_id:               return makeObject<Radar>(args...);
        case Refinery::item_id:            return makeObject<Refinery>(args...);
        case RepairYard::item_id:          return makeObject<RepairYard>(args...);
        case RocketTurret::item_id:        return makeObject<RocketTurret>(args...);
        case Silo::item_id:                return makeObject<Silo>(args...);
        case StarPort::item_id:            return makeObject<StarPort>(args...);
        case Wall::item_id:                return makeObject<Wall>(args...);
        case WindTrap::item_id:            return makeObject<WindTrap>(args...);
        case WOR::item_id:                 return makeObject<WOR>(args...);

        case Carryall::item_id:            return makeObject<Carryall>(args...);
        case Devastator::item_id:          return makeObject<Devastator>(args...);
        case Deviator::item_id:            return makeObject<Deviator>(args...);
        case Frigate::item_id:             return makeObject<Frigate>(args...);
        case Harvester::item_id:           return makeObject<Harvester>(args...);
        case Soldier::item_id:             return makeObject<Soldier>(args...);
        case Launcher::item_id:            return makeObject<Launcher>(args...);
        case MCV::item_id:                 return makeObject<MCV>(args...);
        case Ornithopter::item_id:         return makeObject<Ornithopter>(args...);
        case Quad::item_id:                return makeObject<Quad>(args...);
        case Saboteur::item_id:            return makeObject<Saboteur>(args...);
        case Sandworm::item_id:            return makeObject<Sandworm>(args...);
        case SiegeTank::item_id:           return makeObject<SiegeTank>(args...);
        case SonicTank::item_id:           return makeObject<SonicTank>(args...);
        case Tank::item_id:                return makeObject<Tank>(args...);
        case Trike::item_id:               return makeObject<Trike>(args...);
        case RaiderTrike::item_id:         return makeObject<RaiderTrike>(args...);
        case Trooper::item_id:             return makeObject<Trooper>(args...);

        default:                            Dune::Logger.log("ObjectBase::makeObject(): %d is no valid ItemID!",itemID);
                                            return std::unique_ptr<ObjectBase>{};
    }
    // clang-format on
}

} // anonymous namespace

std::unique_ptr<ObjectBase> ObjectBase::createObject(ItemID_enum itemID, uint32_t objectID,
                                                     const ObjectInitializer& initializer) {
    return objectFactory(itemID, objectID, initializer);
}

std::unique_ptr<ObjectBase> ObjectBase::loadObject(ItemID_enum itemID, uint32_t objectID,
                                                   const ObjectStreamInitializer& initializer) {
    return objectFactory(itemID, objectID, initializer);
}

bool ObjectBase::targetInWeaponRange(const GameContext& context) const {
    const auto coord = target.getObjPointer(context.objectManager)->getClosestPoint(location);
    const auto dist  = blockDistance(location, coord);

    return dist <= context.game.getObjectData(itemID, originalHouseID).weaponrange;
}

} // namespace Dune::Engine
