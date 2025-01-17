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

#include <Menu/SinglePlayerMenu.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

#include <misc/exceptions.h>
#include <misc/fnkdat.h>

#include <Menu/CustomGameMenu.h>
#include <Menu/HouseChoiceMenu.h>
#include <Menu/SinglePlayerSkirmishMenu.h>

#include <GUI/MsgBox.h>
#include <GUI/dune/LoadSaveWindow.h>

#include <Game.h>
#include <GameInitSettings.h>
#include <sand.h>

SinglePlayerMenu::SinglePlayerMenu() {
    // set up window

    SinglePlayerMenu::setWindowWidget(&windowWidget);

    const auto* const gfx = dune::globals::pGFXManager.get();

    // set up pictures in the background
    const auto* const pPlanetBackground = gfx->getUIGraphic(UI_PlanetBackground);
    planetPicture.setTexture(pPlanetBackground);
    auto dest1 = calcAlignedDrawingRect(pPlanetBackground);
    dest1.y    = dest1.y - getHeight(pPlanetBackground) / 2 + 10;
    windowWidget.addWidget(&planetPicture, dest1);

    const auto* const pDuneLegacy = gfx->getUIGraphic(UI_DuneLegacy);
    duneLegacy.setTexture(pDuneLegacy);
    auto dest2 = calcAlignedDrawingRect(pDuneLegacy);
    dest2.y    = dest2.y + getHeight(pDuneLegacy) / 2 + 28;
    windowWidget.addWidget(&duneLegacy, dest2);

    const auto* pMenuButtonBorder = gfx->getUIGraphic(UI_MenuButtonBorder);
    buttonBorder.setTexture(pMenuButtonBorder);
    auto dest3 = calcAlignedDrawingRect(pMenuButtonBorder);
    dest3.y    = dest3.y + getHeight(pMenuButtonBorder) / 2 + 59;
    windowWidget.addWidget(&buttonBorder, dest3);

    // set up menu buttons
    windowWidget.addWidget(&menuButtonsVBox, Point((getRendererWidth() - 160) / 2, getRendererHeight() / 2 + 64),
                           Point(160, 111));

    campaignButton.setText(_("CAMPAIGN"));
    campaignButton.setOnClick([this] { onCampaign(); });
    menuButtonsVBox.addWidget(&campaignButton);
    campaignButton.setActive();

    menuButtonsVBox.addWidget(Widget::create<VSpacer>(3).release());

    customButton.setText(_("CUSTOM GAME"));
    customButton.setOnClick([this] { onCustom(); });
    menuButtonsVBox.addWidget(&customButton);

    menuButtonsVBox.addWidget(Widget::create<VSpacer>(3).release());

    skirmishButton.setText(_("SKIRMISH"));
    skirmishButton.setOnClick([this] { onSkirmish(); });
    menuButtonsVBox.addWidget(&skirmishButton);

    menuButtonsVBox.addWidget(Widget::create<VSpacer>(3).release());

    loadSavegameButton.setText(_("LOAD GAME"));
    loadSavegameButton.setOnClick([this] { onLoadSavegame(); });
    menuButtonsVBox.addWidget(&loadSavegameButton);

    menuButtonsVBox.addWidget(Widget::create<VSpacer>(3).release());

    loadReplayButton.setText(_("LOAD REPLAY"));
    loadReplayButton.setOnClick([this] { onLoadReplay(); });
    menuButtonsVBox.addWidget(&loadReplayButton);

    menuButtonsVBox.addWidget(Widget::create<VSpacer>(3).release());

    cancelButton.setText(_("BACK"));
    cancelButton.setOnClick([this] { onCancel(); });
    menuButtonsVBox.addWidget(&cancelButton);
}

SinglePlayerMenu::~SinglePlayerMenu() = default;

void SinglePlayerMenu::onCampaign() {
    int player = HouseChoiceMenu().showMenu([&](const auto& e) { doInput(e); });

    if (player < 0) {
        return;
    }

    GameInitSettings init(static_cast<HOUSETYPE>(player), dune::globals::settings.gameOptions);

    for_each_housetype([&](const auto houseID) {
        if (houseID == static_cast<HOUSETYPE>(player)) {
            GameInitSettings::HouseInfo humanHouseInfo(static_cast<HOUSETYPE>(player), 1);
            humanHouseInfo.addPlayerInfo({dune::globals::settings.general.playerName, HUMANPLAYERCLASS});
            init.addHouseInfo(humanHouseInfo);
        } else {
            GameInitSettings::HouseInfo aiHouseInfo(houseID, 2);
            aiHouseInfo.addPlayerInfo({getHouseNameByNumber(houseID), dune::globals::settings.ai.campaignAI});
            init.addHouseInfo(aiHouseInfo);
        }
    });

    startSinglePlayerGame(init, [&](const auto& e) { doInput(e); });

    quit();
}

void SinglePlayerMenu::onCustom() {
    CustomGameMenu(false).showMenu(sdl_handler_);
}

void SinglePlayerMenu::onSkirmish() {
    SinglePlayerSkirmishMenu().showMenu(sdl_handler_);
}

void SinglePlayerMenu::onLoadSavegame() {
    auto [ok, savepath] = fnkdat("save/", FNKDAT_USER | FNKDAT_CREAT);
    openWindow(LoadSaveWindow::create(false, _("Load Game"), savepath, "dls").release());
}

void SinglePlayerMenu::onLoadReplay() {
    auto [ok, replaypath] = fnkdat("replay/", FNKDAT_USER | FNKDAT_CREAT);
    openWindow(LoadSaveWindow::create(false, _("Load Replay"), replaypath, "rpl").release());
}

void SinglePlayerMenu::onCancel() {
    quit();
}

void SinglePlayerMenu::onChildWindowClose(Window* pChildWindow) {
    std::filesystem::path filename;
    std::string extension;
    const auto* pLoadSaveWindow = dynamic_cast<LoadSaveWindow*>(pChildWindow);
    if (pLoadSaveWindow != nullptr) {
        filename  = pLoadSaveWindow->getFilename();
        extension = pLoadSaveWindow->getExtension();
    }

    if (!filename.empty()) {
        if (extension == "dls") {

            try {
                startSinglePlayerGame(GameInitSettings(std::move(filename)), [&](const auto& e) { doInput(e); });
            } catch (std::exception& e) {
                // most probably the savegame file is not valid or from a different dune legacy version
                openWindow(MsgBox::create(e.what()));
            }
        } else if (extension == "rpl") {
            startReplay(filename, [&](const auto& e) { doInput(e); });
        }
    }
}

void SinglePlayerMenu::resize(uint32_t width, uint32_t height) {
    parent::resize(width, height);

    const auto iWidth  = static_cast<int>(width);
    const auto iHeight = static_cast<int>(height);

    const auto planet_size = planetPicture.getSize();
    windowWidget.setWidgetGeometry(&planetPicture, {(iWidth - planet_size.x) / 2, iHeight / 2 - planet_size.y + 10},
                                   planet_size);

    const auto dune_size = duneLegacy.getSize();
    windowWidget.setWidgetGeometry(&duneLegacy, {(iWidth - dune_size.x) / 2, iHeight / 2 + 28}, dune_size);

    const auto border_size = buttonBorder.getSize();
    windowWidget.setWidgetGeometry(&buttonBorder, {(iWidth - border_size.x) / 2, iHeight / 2 + 59}, border_size);

    const auto menu_size = menuButtonsVBox.getSize();
    windowWidget.setWidgetGeometry(&menuButtonsVBox, {(iWidth - 160) / 2, iHeight / 2 + 64}, menu_size);
}
