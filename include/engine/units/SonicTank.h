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

#ifndef ENGINE_SONICTANK_H
#define ENGINE_SONICTANK_H

#include <units/TrackedUnit.h>

namespace Dune::Engine {

class SonicTank final : public TrackedUnit {
public:
    inline static constexpr ItemID_enum item_id = Unit_SonicTank;
    using parent                                = TrackedUnit;

    SonicTank(uint32_t objectID, const ObjectInitializer& initializer);
    SonicTank(uint32_t objectID, const ObjectStreamInitializer& initializer);
    ~SonicTank() override;

    void destroy(const GameContext& context) override;

    void handleDamage(const GameContext& context, int damage, uint32_t damagerID, House* damagerOwner) override;

    bool canAttack(const GameContext& context, const ObjectBase* object) const override;

private:
    void init();
};

} // namespace Dune::Engine

#endif // ENGINE_SONICTANK_H
