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

#include "engine_mmath.h"

#include <units/Ornithopter.h>

#include <Map.h>
#include <House.h>
#include <Game.h>

namespace {
using namespace Dune::Engine;

constexpr int ORNITHOPTER_FRAMETIME = 3;

constexpr AirUnitConstants ornithopter_constants{Ornithopter::item_id, 1, Bullet_SmallRocket};
} // namespace

namespace Dune::Engine {

Ornithopter::Ornithopter(uint32_t objectID, const ObjectInitializer& initializer)
    : AirUnit(ornithopter_constants, objectID, initializer) {

    Ornithopter::init(initializer.game());

    Ornithopter::setHealth(initializer.game(), getMaxHealth(initializer.game()));

    timeLastShot = 0;
}

Ornithopter::Ornithopter(uint32_t objectID, const ObjectStreamInitializer& initializer)
    : AirUnit(ornithopter_constants, objectID, initializer) {
    Ornithopter::init(initializer.game());

    auto& stream = initializer.stream();

    timeLastShot = stream.readUint32();
}

void Ornithopter::init(const Game& game) {
    assert(itemID == Unit_Ornithopter);
    owner->incrementUnits(itemID);

    currentMaxSpeed = game.getObjectData(itemID, originalHouseID).maxspeed;
}

Ornithopter::~Ornithopter() = default;

void Ornithopter::save(const Game& game, OutputStream& stream) const {
    parent::save(game, stream);

    stream.writeUint32(timeLastShot);
}

void Ornithopter::checkPos(const GameContext& context) {
    parent::checkPos(context);

    if(!target) {
        if(destination.isValid()) {
            if(blockDistance(location, destination) <= 2) { destination.invalidate(); }
        } else {
            if(blockDistance(location, guardPoint) > 17) { setDestination(context, guardPoint); }
        }
    }
}

bool Ornithopter::canAttack(const GameContext& context, const ObjectBase* object) const {
    return (object != nullptr) && !object->isAFlyingUnit() &&
           ((object->getOwner()->getTeamID() != owner->getTeamID()) || object->getItemID() == Unit_Sandworm) &&
           object->isVisible(getOwner()->getTeamID());
}

void Ornithopter::destroy(const GameContext& context) {
    // place wreck
    if(auto* const pTile = context.map.tryGetTile(location.x, location.y)) {
        pTile->assignDeadUnit(DeadUnit_Ornithopter, owner->getHouseID(), Coord(lround(realX), lround(realY)));
    }

    parent::destroy(context);
}

bool Ornithopter::canPassTile(const GameContext& context, const Tile* pTile) const {
    return pTile && (!pTile->hasAnAirUnit());
}

FixPoint Ornithopter::getDestinationAngle(const Game& game) const {
    FixPoint angle;

    if(timeLastShot > 0 && (game.getGameCycleCount() - timeLastShot) < MILLI2CYCLES(1000)) {
        // we already shot at target and now want to fly in the opposite direction
        angle = destinationAngleRad(destination.x * TILESIZE + TILESIZE / 2, destination.y * TILESIZE + TILESIZE / 2,
                                    realX, realY);
    } else {
        angle = destinationAngleRad(realX, realY, destination.x * TILESIZE + TILESIZE / 2,
                                    destination.y * TILESIZE + TILESIZE / 2);
    }

    return angle * (8 / (FixPt_PI << 1));
}

bool Ornithopter::attack(const GameContext& context) {
    auto bAttacked = parent::attack(context);

    if(bAttacked) { timeLastShot = context.game.getGameCycleCount(); }
    return bAttacked;
}

} // namespace Dune::Engine
