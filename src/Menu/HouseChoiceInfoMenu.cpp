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

#include <Menu/HouseChoiceInfoMenu.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

HouseChoiceInfoMenu::HouseChoiceInfoMenu(HOUSETYPE newHouse) : MentatMenu(HOUSETYPE::HOUSE_INVALID) {
    disableQuitting(true);

    house = newHouse;

    Animation* anim = nullptr;

    auto* const gfx          = dune::globals::pGFXManager.get();
    auto* const pTextManager = dune::globals::pTextManager.get();

    // clang-format off
    switch(house) {
        case HOUSETYPE::HOUSE_HARKONNEN:    anim = gfx->getAnimation(Anim_HarkonnenPlanet); break;
        case HOUSETYPE::HOUSE_ATREIDES:     anim = gfx->getAnimation(Anim_AtreidesPlanet);  break;
        case HOUSETYPE::HOUSE_ORDOS:        anim = gfx->getAnimation(Anim_OrdosPlanet);     break;
        case HOUSETYPE::HOUSE_FREMEN:       anim = gfx->getAnimation(Anim_FremenPlanet);    break;
        case HOUSETYPE::HOUSE_SARDAUKAR:    anim = gfx->getAnimation(Anim_SardaukarPlanet); break;
        case HOUSETYPE::HOUSE_MERCENARY:    anim = gfx->getAnimation(Anim_MercenaryPlanet); break;
        default: {
            THROW(std::invalid_argument, "HouseChoiceInfoMenu::HouseChoiceInfoMenu(): Invalid house id '%d'.", static_cast<int>(newHouse));
        }
    }
    // clang-format on

    planetAnimation.setAnimation(anim);
    windowWidget.addWidget(&planetAnimation, {256, 96}, planetAnimation.getMinimumSize());

    const auto* const pQuestionTexture = gfx->getUIGraphic(UI_MentatHouseChoiceInfoQuestion, newHouse);
    questionLabel.setTexture(pQuestionTexture);
    windowWidget.addWidget(&questionLabel, {0, 0}, getTextureSize(pQuestionTexture));
    questionLabel.setVisible(false);

    // init textbox but skip first line (this line contains "House ???")
    auto desc = pTextManager->getBriefingText(0, MISSION_DESCRIPTION, house);

    const auto linebreak = desc.find('\n');
    if (linebreak != decltype(desc)::npos)
        desc = desc.substr(linebreak + 1);
    setText(desc);

    const auto* const pMentatYes        = gfx->getUIGraphic(UI_MentatYes);
    const auto* const pMentatYesPressed = gfx->getUIGraphic(UI_MentatYes_Pressed);

    yesButton.setTextures(pMentatYes, pMentatYesPressed);
    yesButton.setEnabled(false);
    yesButton.setVisible(false);
    yesButton.setOnClick([this] { onYes(); });
    windowWidget.addWidget(&yesButton, {370, 340}, getTextureSize(pMentatYes));

    const auto* const pMentatNo        = gfx->getUIGraphic(UI_MentatNo);
    const auto* const pMentatNoPressed = gfx->getUIGraphic(UI_MentatNo_Pressed);

    noButton.setTextures(pMentatNo, pMentatNoPressed);
    noButton.setEnabled(false);
    noButton.setVisible(false);
    noButton.setOnClick([&] { onNo(); });
    windowWidget.addWidget(&noButton, {480, 340}, getTextureSize(pMentatNo));
}

HouseChoiceInfoMenu::~HouseChoiceInfoMenu() = default;

void HouseChoiceInfoMenu::onMentatTextFinished() {
    yesButton.setEnabled(true);
    yesButton.setVisible(true);
    noButton.setEnabled(true);
    noButton.setVisible(true);

    questionLabel.setVisible(true);
}

void HouseChoiceInfoMenu::onYes() {
    quit(MENU_QUIT_HOUSECHOICE_YES);
}

void HouseChoiceInfoMenu::onNo() {
    quit(MENU_QUIT_DEFAULT);
}
