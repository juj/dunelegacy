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

#ifndef ENGINE_ORNITHOPTER_H
#define ENGINE_ORNITHOPTER_H

#include <units/AirUnit.h>

namespace Dune::Engine {

class Ornithopter final : public AirUnit {
public:
    inline static constexpr ItemID_enum item_id = Unit_Ornithopter;
    using parent                                = AirUnit;

    Ornithopter(uint32_t objectID, const ObjectInitializer& initializer);
    Ornithopter(uint32_t objectID, const ObjectStreamInitializer& initializer);
    ~Ornithopter() override;

    void save(const Game& game, OutputStream& stream) const override;

    void checkPos(const GameContext& context) override;
    bool canAttack(const GameContext& context, const ObjectBase* object) const override;

    bool canPassTile(const GameContext& context, const Tile* pTile) const override;

    void destroy(const GameContext& context) override;

protected:
    FixPoint getDestinationAngle(const Game& game) const override;

    bool attack(const GameContext& context) override;

private:
    void init(const Game& game);

    uint32_t timeLastShot;
};

} // namespace Dune::Engine

#endif // ENGINE_ORNITHOPTER_H
