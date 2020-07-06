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

#ifndef AIRUNIT_H
#define AIRUNIT_H

#include <units/UnitBase.h>
#include <FileClasses/GFXManager.h>

class AirUnit : public UnitBase
{
protected:
    AirUnit(ItemID_enum itemID, Uint32 objectID, const ObjectInitializer& initializer);
    AirUnit(ItemID_enum itemID, Uint32 objectID, const ObjectStreamInitializer& initializer);

public:
    using parent = UnitBase;

    ~AirUnit() override = 0;

    AirUnit(const AirUnit &) = delete;
    AirUnit(AirUnit &&) = delete;
    AirUnit& operator=(const AirUnit &) = delete;
    AirUnit& operator=(AirUnit &&) = delete;

    void save(OutputStream& stream) const override;

    void blitToScreen() override;

    void playConfirmSound() override { }
    void playSelectSound() override { }

    void destroy(const GameContext& context) override;

    void assignToMap(const GameContext& context, const Coord& pos) override;
    void checkPos(const GameContext& context) override;
    bool canPassTile(const Tile* pTile) const override;

    FixPoint getMaxSpeed(const GameContext& context) const override {
        return currentMaxSpeed;
    }

protected:
    virtual FixPoint getDestinationAngle() const;

    void navigate(const GameContext& context) override;
    void move(const GameContext& context) override;
    void turn(const GameContext& context) override;

    FixPoint currentMaxSpeed;               ///< The current maximum allowed speed

    zoomable_texture shadowGraphic{};       ///< The graphic for the shadow of this air unit

private:
    void initAirUnit();
};

template<>
inline AirUnit* dune_cast(ObjectBase* base) {
    if(base && base->isAFlyingUnit()) return static_cast<AirUnit*>(base);

    return nullptr;
}

template<>
inline const AirUnit* dune_cast(const ObjectBase* base) {
    if(base && base->isAFlyingUnit()) return static_cast<const AirUnit*>(base);

    return nullptr;
}

#endif // AIRUNIT_H
