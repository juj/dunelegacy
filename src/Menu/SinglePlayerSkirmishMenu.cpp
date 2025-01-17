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

#include <Menu/SinglePlayerSkirmishMenu.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

#include <GameInitSettings.h>
#include <sand.h>

#include "GUI/Spacer.h"

namespace {
constexpr std::array houseOrder = {HOUSETYPE::HOUSE_ATREIDES,  HOUSETYPE::HOUSE_ORDOS,  HOUSETYPE::HOUSE_HARKONNEN,
                                   HOUSETYPE::HOUSE_MERCENARY, HOUSETYPE::HOUSE_FREMEN, HOUSETYPE::HOUSE_SARDAUKAR};
}

SinglePlayerSkirmishMenu::SinglePlayerSkirmishMenu() {
    // set up window

    SinglePlayerSkirmishMenu::setWindowWidget(&windowWidget);

    const auto* const gfx = dune::globals::pGFXManager.get();

    // set up pictures in the background
    const auto* const pDuneLegacy = gfx->getUIGraphic(UI_DuneLegacy);
    duneLegacy.setTexture(pDuneLegacy);
    auto dest1 = calcAlignedDrawingRect(pDuneLegacy);
    dest1.y    = dest1.y + getHeight(pDuneLegacy) / 2 + 28;
    windowWidget.addWidget(&duneLegacy, dest1);

    const auto* const pMenuButtonBorder = gfx->getUIGraphic(UI_MenuButtonBorder);
    buttonBorder.setTexture(pMenuButtonBorder);
    auto dest2 = calcAlignedDrawingRect(pMenuButtonBorder);
    dest2.y    = dest2.y + getHeight(pMenuButtonBorder) / 2 + 59;
    windowWidget.addWidget(&buttonBorder, dest2);

    // set up menu buttons
    windowWidget.addWidget(&menuButtonsVBox, Point((getRendererWidth() - 160) / 2, getRendererHeight() / 2 + 64),
                           Point(160, 111));

    startButton.setText(_("Start"));
    startButton.setOnClick([&] { onStart(); });
    menuButtonsVBox.addWidget(&startButton);
    startButton.setActive();

    menuButtonsVBox.addWidget(Widget::create<VSpacer>(79).release());

    backButton.setText(_("Back"));
    backButton.setOnClick([&] { onCancel(); });
    menuButtonsVBox.addWidget(&backButton);

    // set up house choice

    const auto* pHouseSelect = gfx->getUIGraphic(UI_HouseSelect);
    auto dest                = calcAlignedDrawingRect(pHouseSelect);
    dest.y                   = dest.y - getHeight(pHouseSelect) / 2 + 10;
    windowWidget.addWidget(&houseChoiceContainer, dest);

    heraldPicture.setTexture(pHouseSelect);
    houseChoiceContainer.addWidget(&heraldPicture, Point(0, 0), getTextureSize(pHouseSelect));

    // House1 button
    houseChoiceContainer.addWidget(&house1Picture, Point(21, 54), Point(83, 91));
    houseChoiceContainer.addWidget(&house1SelectedPicture, Point(20, 53), Point(83, 91));

    house1Button.setOnClick([&] { onSelectHouseButton(0); });
    houseChoiceContainer.addWidget(&house1Button, Point(20, 53), Point(83, 91));

    // House2 button
    houseChoiceContainer.addWidget(&house2Picture, Point(117, 54), Point(83, 91));
    houseChoiceContainer.addWidget(&house2SelectedPicture, Point(116, 53), Point(83, 91));

    house2Button.setOnClick([&] { onSelectHouseButton(1); });
    houseChoiceContainer.addWidget(&house2Button, Point(116, 53), Point(83, 91));

    // House3 button
    houseChoiceContainer.addWidget(&house3Picture, Point(215, 54), Point(83, 91));
    houseChoiceContainer.addWidget(&house3SelectedPicture, Point(214, 53), Point(83, 91));

    house3Button.setOnClick([&] { onSelectHouseButton(2); });
    houseChoiceContainer.addWidget(&house3Button, Point(214, 53), Point(83, 91));

    const auto* const pArrowLeft          = gfx->getUIGraphic(UI_Herald_ArrowLeft);
    const auto* const pArrowLeftHighlight = gfx->getUIGraphic(UI_Herald_ArrowLeftHighlight);
    houseLeftButton.setTextures(pArrowLeft, pArrowLeft, pArrowLeftHighlight);
    houseLeftButton.setOnClick([&] { onHouseLeft(); });
    houseLeftButton.setVisible(false);
    houseChoiceContainer.addWidget(&houseLeftButton,
                                   Point(houseChoiceContainer.getSize().x / 2 - getWidth(pArrowLeft) - 85, 160),
                                   getTextureSize(pArrowLeft));

    const auto* const pArrowRight          = gfx->getUIGraphic(UI_Herald_ArrowRight);
    const auto* const pArrowRightHighlight = gfx->getUIGraphic(UI_Herald_ArrowRightHighlight);
    houseRightButton.setTextures(pArrowRight, pArrowRight, pArrowRightHighlight);
    houseRightButton.setOnClick([&] { onHouseRight(); });
    houseChoiceContainer.addWidget(&houseRightButton, Point(houseChoiceContainer.getSize().x / 2 + 85, 160),
                                   getTextureSize(pArrowRight));

    updateHouseChoice();

    onSelectHouseButton(1);

    // setup +/- Buttons to select mission

    missionCounter.setCount(mission);
    windowWidget.addWidget(&missionCounter,
                           Point(getRendererWidth() / 4 * 3 + 160 / 4 - 83 / 2, getRendererHeight() / 2 + 89),
                           missionCounter.getMinimumSize());

    const auto* const pPlus        = gfx->getUIGraphic(UI_Plus);
    const auto* const pPlusPressed = gfx->getUIGraphic(UI_Plus_Pressed);
    missionPlusButton.setTextures(pPlus, pPlusPressed);
    missionPlusButton.setOnClick([&] { onMissionIncrement(); });
    windowWidget.addWidget(
        &missionPlusButton,
        Point(getRendererWidth() / 4 * 3 + 160 / 4 - getWidth(pPlus) / 2 + 72, getRendererHeight() / 2 + 96),
        getTextureSize(pPlus));

    const auto* const pMinus        = gfx->getUIGraphic(UI_Minus);
    const auto* const pMinusPressed = gfx->getUIGraphic(UI_Minus_Pressed);
    missionMinusButton.setTextures(pMinus, pMinusPressed);
    missionMinusButton.setOnClick([&] { onMissionDecrement(); });
    windowWidget.addWidget(
        &missionMinusButton,
        Point(getRendererWidth() / 4 * 3 + 160 / 4 - getWidth(pMinus) / 2 + 72, getRendererHeight() / 2 + 109),
        getTextureSize(pMinus));
}

SinglePlayerSkirmishMenu::~SinglePlayerSkirmishMenu() = default;

void SinglePlayerSkirmishMenu::onStart() {
    const auto houseChoice = houseOrder[(currentHouseChoiceScrollPos + selectedButton) % houseOrder.size()];

    const auto& settings = dune::globals::settings;

    GameInitSettings init(houseChoice, mission, settings.gameOptions);

    for (const auto house : houseOrder) {
        if (house == houseChoice) {
            GameInitSettings::HouseInfo humanHouseInfo(houseChoice, 1);
            humanHouseInfo.addPlayerInfo({settings.general.playerName, HUMANPLAYERCLASS});
            init.addHouseInfo(humanHouseInfo);
        } else {
            GameInitSettings::HouseInfo aiHouseInfo(house, 2);
            aiHouseInfo.addPlayerInfo({getHouseNameByNumber(house), settings.ai.campaignAI});
            init.addHouseInfo(aiHouseInfo);
        }
    }

    startSinglePlayerGame(init, [&](const auto& e) { doInput(e); });

    quit();
}

void SinglePlayerSkirmishMenu::onCancel() {
    quit();
}

void SinglePlayerSkirmishMenu::onSelectHouseButton(int button) {
    selectedButton = button;

    house1Button.setEnabled(selectedButton != 0);
    house2Button.setEnabled(selectedButton != 1);
    house3Button.setEnabled(selectedButton != 2);

    house1Picture.setVisible(selectedButton != 0);
    house2Picture.setVisible(selectedButton != 1);
    house3Picture.setVisible(selectedButton != 2);

    house1SelectedPicture.setVisible(selectedButton == 0);
    house2SelectedPicture.setVisible(selectedButton == 1);
    house3SelectedPicture.setVisible(selectedButton == 2);
}

void SinglePlayerSkirmishMenu::onHouseLeft() {
    if (currentHouseChoiceScrollPos > 0) {
        currentHouseChoiceScrollPos--;
        selectedButton++;
        onSelectHouseButton(selectedButton);
        updateHouseChoice();

        houseLeftButton.setVisible(currentHouseChoiceScrollPos > 0);
        houseRightButton.setVisible(true);
    }
}

void SinglePlayerSkirmishMenu::onHouseRight() {
    if (currentHouseChoiceScrollPos < 3) {
        currentHouseChoiceScrollPos++;
        selectedButton--;
        onSelectHouseButton(selectedButton);
        updateHouseChoice();

        houseLeftButton.setVisible(true);
        houseRightButton.setVisible(currentHouseChoiceScrollPos < 3);
    }
}

void SinglePlayerSkirmishMenu::onMissionIncrement() {
    mission++;
    if (mission > 22) {
        mission = 1;
    }
    missionCounter.setCount(mission);
}

void SinglePlayerSkirmishMenu::onMissionDecrement() {
    mission--;
    if (mission < 1) {
        mission = 22;
    }
    missionCounter.setCount(mission);
}

void SinglePlayerSkirmishMenu::updateHouseChoice() {
    const auto* const gfx = dune::globals::pGFXManager.get();

    // House1 button
    house1Picture.setTexture(gfx->getUIGraphic(UI_Herald_Grey, houseOrder[currentHouseChoiceScrollPos + 0]));
    house1SelectedPicture.setTexture(gfx->getUIGraphic(UI_Herald_Colored, houseOrder[currentHouseChoiceScrollPos + 0]));

    // House2 button
    house2Picture.setTexture(gfx->getUIGraphic(UI_Herald_Grey, houseOrder[currentHouseChoiceScrollPos + 1]));
    house2SelectedPicture.setTexture(gfx->getUIGraphic(UI_Herald_Colored, houseOrder[currentHouseChoiceScrollPos + 1]));

    // House3 button
    house3Picture.setTexture(gfx->getUIGraphic(UI_Herald_Grey, houseOrder[currentHouseChoiceScrollPos + 2]));
    house3SelectedPicture.setTexture(gfx->getUIGraphic(UI_Herald_Colored, houseOrder[currentHouseChoiceScrollPos + 2]));
}
