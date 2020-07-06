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

#include <units/SonicTank.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Game.h>
#include <Map.h>
#include <Explosion.h>
#include <ScreenBorder.h>
#include <SoundPlayer.h>


SonicTank::SonicTank(ItemID_enum itemID, Uint32 objectID, const ObjectInitializer& initializer) : TrackedUnit(itemID, objectID, initializer) {
    SonicTank::init();

    setHealth(getMaxHealth());
}

SonicTank::SonicTank(ItemID_enum itemID, Uint32 objectID, const ObjectStreamInitializer& initializer) : TrackedUnit(itemID, objectID, initializer) {
    SonicTank::init();
}

void SonicTank::init() {
    assert(itemID == Unit_SonicTank);
    owner->incrementUnits(itemID);

    numWeapons = 1;
    bulletType = Bullet_Sonic;

    graphicID = ObjPic_Tank_Base;
    gunGraphicID = ObjPic_Sonictank_Gun;
    graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
    turretGraphic = pGFXManager->getObjPic(gunGraphicID,getOwner()->getHouseID());

    numImagesX = static_cast<int>(ANGLETYPE::NUM_ANGLES);
    numImagesY = 1;
}

SonicTank::~SonicTank() = default;

void SonicTank::blitToScreen() {
    int x1 = screenborder->world2screenX(realX);
    int y1 = screenborder->world2screenY(realY);

    SDL_Texture* pUnitGraphic = graphic[currentZoomlevel];
    SDL_Rect source1 = calcSpriteSourceRect(pUnitGraphic, static_cast<int>(drawnAngle), numImagesX);
    SDL_Rect dest1 = calcSpriteDrawingRect( pUnitGraphic, x1, y1, numImagesX, 1, HAlign::Center, VAlign::Center);

    SDL_RenderCopy(renderer, pUnitGraphic, &source1, &dest1);

    const Coord sonicTankTurretOffset[] =   {   Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8)
                                            };

    SDL_Texture* pTurretGraphic = turretGraphic[currentZoomlevel];
    SDL_Rect source2 = calcSpriteSourceRect(pTurretGraphic, static_cast<int>(drawnAngle), numImagesX);
    SDL_Rect dest2 = calcSpriteDrawingRect( pTurretGraphic,
                                            screenborder->world2screenX(realX + sonicTankTurretOffset[static_cast<int>(drawnAngle)].x),
                                            screenborder->world2screenY(realY + sonicTankTurretOffset[static_cast<int>(drawnAngle)].y),
                                            numImagesX, 1, HAlign::Center, VAlign::Center);

    SDL_RenderCopy(renderer, pTurretGraphic, &source2, &dest2);

    if(isBadlyDamaged()) {
        drawSmoke(x1, y1);
    }
}

void SonicTank::destroy(const GameContext& context) {
    if(currentGameMap->tileExists(location) && isVisible()) {
        Coord realPos(lround(realX), lround(realY));
        currentGame->addExplosion(Explosion_SmallUnit, realPos, owner->getHouseID());

        if(isVisible(getOwner()->getTeamID()))
            soundPlayer->playSoundAt(Sound_ExplosionSmall,location);
    }

    parent::destroy(context);
}

void SonicTank::handleDamage(const GameContext& context, int damage, Uint32 damagerID, House* damagerOwner) {
    ObjectBase* damager = currentGame->getObjectManager().getObject(damagerID);

    if (!damager || (damager->getItemID() != Unit_SonicTank))
        parent::handleDamage(context, damage, damagerID, damagerOwner);
}

bool SonicTank::canAttack(const ObjectBase *object) const {
    return ((object != nullptr) && ObjectBase::canAttack(object) && (object->getItemID() != Unit_SonicTank));
}

void SonicTank::playAttackSound() {
    soundPlayer->playSoundAt(Sound_Sonic,location);
}
