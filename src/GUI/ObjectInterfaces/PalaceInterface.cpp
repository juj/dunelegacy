#include <GUI/ObjectInterfaces/PalaceInterface.h>

#include <FileClasses/Font.h>
#include <FileClasses/FontManager.h>
#include <FileClasses/TextManager.h>

#include <structures/Palace.h>

std::unique_ptr<PalaceInterface> PalaceInterface::create(const GameContext& context, int objectID) {
    std::unique_ptr<PalaceInterface> tmp{new PalaceInterface{context, objectID}};
    tmp->pAllocated = true;
    return tmp;
}

sdl2::surface_ptr PalaceInterface::createSurface(SurfaceLoader* surfaceLoader, GeneratedPicture id) {
    auto* const deathHandSurface = surfaceLoader->getSmallDetailSurface(Picture_DeathHand);

    const auto pText{dune::globals::pFontManager->getFont(12)->createTextSurface(_("READY"), COLOR_WHITE)};

    sdl2::surface_ptr pReady{SDL_CreateRGBSurface(0, getWidth(deathHandSurface), getHeight(deathHandSurface),
                                                  SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    SDL_FillRect(pReady.get(), nullptr, COLOR_TRANSPARENT);

    auto dest = calcAlignedDrawingRect(pText.get(), pReady.get());
    SDL_BlitSurface(pText.get(), nullptr, pReady.get(), &dest);

    return std::move(pReady);
}

PalaceInterface::PalaceInterface(const GameContext& context, int objectID)
    : DefaultStructureInterface(context, objectID) {
    mainHBox.addWidget(&weaponBox);

    auto* const gfx = dune::globals::pGFXManager.get();

    const auto* const pTexture = gfx->getSmallDetailPic(Picture_DeathHand);
    weaponBox.addWidget(&weaponProgressBar, Point((SIDEBARWIDTH - 25 - getWidth(pTexture)) / 2, 5),
                        getTextureSize(pTexture));

    weaponBox.addWidget(&weaponSelectButton, Point((SIDEBARWIDTH - 25 - getWidth(pTexture)) / 2, 5),
                        getTextureSize(pTexture));

    const auto* const ready = gfx->getGeneratedPicture(GeneratedPicture::PalaceReadyText);
    weaponSelectButton.setTextures(ready);
    weaponSelectButton.setVisible(false);

    weaponSelectButton.setOnClick([&] { onSpecial(context_); });
}

bool PalaceInterface::update() {
    auto* const pObject = context_.objectManager.getObject(objectID);
    if (pObject == nullptr)
        return false;

    if (const auto* pPalace = dune_cast<Palace>(pObject)) {
        SmallDetailPics_Enum picID{};

        switch (pPalace->getOwner()->getHouseID()) {
            case HOUSETYPE::HOUSE_HARKONNEN:
            case HOUSETYPE::HOUSE_SARDAUKAR: {
                picID = Picture_DeathHand;
            } break;

            case HOUSETYPE::HOUSE_ATREIDES:
            case HOUSETYPE::HOUSE_FREMEN: {
                picID = Picture_Fremen;
            } break;

            case HOUSETYPE::HOUSE_ORDOS:
            case HOUSETYPE::HOUSE_MERCENARY: {
                picID = Picture_Saboteur;
            } break;

            default: {
                picID = Picture_Fremen;
            } break;
        }

        weaponProgressBar.setTexture(dune::globals::pGFXManager->getSmallDetailPic(picID));
        weaponProgressBar.setProgress(pPalace->getPercentComplete());

        weaponSelectButton.setVisible(pPalace->isSpecialWeaponReady());
    }

    return DefaultStructureInterface::update();
}

void PalaceInterface::onSpecial(const GameContext& context) const {
    auto* pPalace = context.objectManager.getObject<Palace>(objectID);
    if (pPalace == nullptr)
        return;

    if ((pPalace->getOriginalHouseID() == HOUSETYPE::HOUSE_HARKONNEN)
        || (pPalace->getOriginalHouseID() == HOUSETYPE::HOUSE_SARDAUKAR)) {
        context_.game.currentCursorMode = Game::CursorMode_Attack;
    } else {
        pPalace->handleSpecialClick(context);
    }
}
