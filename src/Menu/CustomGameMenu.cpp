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

#include <Menu/CustomGameMenu.h>
#include <Menu/CustomGamePlayers.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/INIFile.h>
#include <FileClasses/TextManager.h>

#include <GUI/GUIStyle.h>
#include <GUI/Spacer.h>
#include <GUI/dune/DuneStyle.h>
#include <GUI/dune/GameOptionsWindow.h>
#include <GUI/dune/LoadSaveWindow.h>

#include <misc/FileSystem.h>
#include <misc/draw_util.h>
#include <misc/fnkdat.h>
#include <misc/string_util.h>

#include <GameInitSettings.h>
#include <INIMap/INIMapPreviewCreator.h>

#include <globals.h>

#include <memory>

CustomGameMenu::CustomGameMenu(bool multiplayer, bool LANServer)
    : bMultiplayer(multiplayer), bLANServer(LANServer), currentGameOptions(dune::globals::settings.gameOptions) {
    // set up window

    CustomGameMenu::setWindowWidget(&windowWidget);

    windowWidget.addWidget(&mainVBox, Point(24, 23), Point(getRendererWidth() - 48, getRendererHeight() - 32));

    captionLabel.setText(bMultiplayer ? (bLANServer ? _("LAN Game") : _("Internet Game")) : _("Custom Game"));
    captionLabel.setAlignment(Alignment_HCenter);
    mainVBox.addWidget(&captionLabel, 24);
    mainVBox.addWidget(Widget::create<VSpacer>(24).release());

    mainVBox.addWidget(Widget::create<Spacer>().release(), 0.05);

    mainVBox.addWidget(&mainHBox, 0.80);

    mainHBox.addWidget(Widget::create<Spacer>().release(), 0.05);
    mainHBox.addWidget(&leftVBox, 0.8);

    leftVBox.addWidget(&mapTypeButtonsHBox, 24);

    singleplayerMapsButton.setText(_("SP Maps"));
    singleplayerMapsButton.setToggleButton(true);
    singleplayerMapsButton.setOnClick([this] { onMapTypeChange(0); });
    mapTypeButtonsHBox.addWidget(&singleplayerMapsButton);

    singleplayerUserMapsButton.setText(_("SP User Maps"));
    singleplayerUserMapsButton.setToggleButton(true);
    singleplayerUserMapsButton.setOnClick([this] { onMapTypeChange(1); });
    mapTypeButtonsHBox.addWidget(&singleplayerUserMapsButton);

    multiplayerMapsButton.setText(_("MP Maps"));
    multiplayerMapsButton.setToggleButton(true);
    multiplayerMapsButton.setOnClick([this] { onMapTypeChange(2); });
    mapTypeButtonsHBox.addWidget(&multiplayerMapsButton);

    multiplayerUserMapsButton.setText(_("MP User Maps"));
    multiplayerUserMapsButton.setToggleButton(true);
    multiplayerUserMapsButton.setOnClick([this] { onMapTypeChange(3); });
    mapTypeButtonsHBox.addWidget(&multiplayerUserMapsButton);

    dummyButton.setEnabled(false);
    mapTypeButtonsHBox.addWidget(&dummyButton, 17);
    mapList.setAutohideScrollbar(false);
    mapList.setOnSelectionChange([this](auto interactive) { onMapListSelectionChange(interactive); });
    mapList.setOnDoubleClick([this] { onNext(); });
    leftVBox.addWidget(&mapList, 0.95);

    leftVBox.addWidget(Widget::create<VSpacer>(10).release());

    multiplePlayersPerHouseCheckbox.setText(_("Multiple players per house"));
    optionsHBox.addWidget(&multiplePlayersPerHouseCheckbox);
    optionsHBox.addWidget(Widget::create<Spacer>().release());
    gameOptionsButton.setText(_("Game Options..."));
    gameOptionsButton.setOnClick([this] { onGameOptions(); });
    optionsHBox.addWidget(&gameOptionsButton, 140);

    leftVBox.addWidget(Widget::create<Spacer>().release(), 0.05);

    leftVBox.addWidget(&optionsHBox, 0.05);

    mainHBox.addWidget(Widget::create<HSpacer>(8).release());
    mainHBox.addWidget(Widget::create<Spacer>().release(), 0.05);

    mainHBox.addWidget(&rightVBox, 180);
    mainHBox.addWidget(Widget::create<Spacer>().release(), 0.05);
    minimap.setSurface(GUIStyle::getInstance().createButtonSurface(130, 130, _("Choose map"), true, false));
    rightVBox.addWidget(&minimap);

    rightVBox.addWidget(Widget::create<VSpacer>(10).release());
    rightVBox.addWidget(&mapPropertiesHBox, 0.01);
    mapPropertiesHBox.addWidget(&mapPropertyNamesVBox, 75);
    mapPropertiesHBox.addWidget(&mapPropertyValuesVBox, 105);
    mapPropertyNamesVBox.addWidget(Label::create(_("Size") + ":").release());
    mapPropertyValuesVBox.addWidget(&mapPropertySize);
    mapPropertyNamesVBox.addWidget(Label::create(_("Players") + ":").release());
    mapPropertyValuesVBox.addWidget(&mapPropertyPlayers);
    mapPropertyNamesVBox.addWidget(Label::create(_("Author") + ":").release());
    mapPropertyValuesVBox.addWidget(&mapPropertyAuthors);
    mapPropertyNamesVBox.addWidget(Label::create(_("License") + ":").release());
    mapPropertyValuesVBox.addWidget(&mapPropertyLicense);
    rightVBox.addWidget(Widget::create<Spacer>().release());

    mainVBox.addWidget(Widget::create<Spacer>().release(), 0.05);

    mainVBox.addWidget(Widget::create<VSpacer>(20).release());
    mainVBox.addWidget(&buttonHBox, 24);
    mainVBox.addWidget(Widget::create<VSpacer>(14).release(), 0.0);

    buttonHBox.addWidget(Widget::create<HSpacer>(70).release());
    cancelButton.setText(_("Back"));
    cancelButton.setOnClick([this] { onCancel(); });
    buttonHBox.addWidget(&cancelButton, 0.1);

    buttonHBox.addWidget(Widget::create<Spacer>().release(), 0.0625);

    buttonHBox.addWidget(Widget::create<Spacer>().release(), 0.25);
    loadButton.setText(_("Load"));
    loadButton.setVisible(bMultiplayer);
    loadButton.setEnabled(bMultiplayer);
    loadButton.setOnClick([this] { onLoad(); });
    buttonHBox.addWidget(&loadButton, 0.175);
    buttonHBox.addWidget(Widget::create<Spacer>().release(), 0.25);

    buttonHBox.addWidget(Widget::create<Spacer>().release(), 0.0625);

    nextButton.setText(_("Next"));
    nextButton.setOnClick([this] { onNext(); });
    buttonHBox.addWidget(&nextButton, 0.1);
    buttonHBox.addWidget(Widget::create<HSpacer>(90).release());

    onMapTypeChange(0);
}

CustomGameMenu::~CustomGameMenu() = default;

void CustomGameMenu::onChildWindowClose(Window* pChildWindow) {
    auto* pLoadSaveWindow = dynamic_cast<LoadSaveWindow*>(pChildWindow);
    if (pLoadSaveWindow != nullptr) {
        auto filename = pLoadSaveWindow->getFilename();

        if (filename != "") {
            auto savegamedata = readCompleteFile(filename);

            auto servername = dune::globals::settings.general.playerName + "'s Game";
            GameInitSettings gameInitSettings(getBasename(filename, true), std::move(savegamedata),
                                              std::move(servername));

            int ret =
                CustomGamePlayers(gameInitSettings, true, bLANServer).showMenu([&](const auto& e) { doInput(e); });
            if (ret != MENU_QUIT_DEFAULT) {
                quit(ret);
            }
        }
    }

    auto* const pGameOptionsWindow = dynamic_cast<GameOptionsWindow*>(pChildWindow);
    if (pGameOptionsWindow != nullptr) {
        currentGameOptions = pGameOptionsWindow->getGameOptions();
    }
}

void CustomGameMenu::onNext() {
    if (mapList.getSelectedIndex() < 0) {
        return;
    }

    auto mapFilename = currentMapDirectory / mapList.getSelectedEntry();
    mapFilename += ".ini";
    getCaseInsensitiveFilename(mapFilename);

    GameInitSettings gameInitSettings;
    if (bMultiplayer) {
        auto servername = dune::globals::settings.general.playerName + "'s Game";
        gameInitSettings =
            GameInitSettings(getBasename(mapFilename, true), readCompleteFile(mapFilename), std::move(servername),
                             multiplePlayersPerHouseCheckbox.isChecked(), currentGameOptions);
    } else {
        gameInitSettings = GameInitSettings(getBasename(mapFilename, true), readCompleteFile(mapFilename),
                                            multiplePlayersPerHouseCheckbox.isChecked(), currentGameOptions);
    }

    int ret = CustomGamePlayers(gameInitSettings, true, bLANServer).showMenu([&](const auto& e) { doInput(e); });
    if (ret != MENU_QUIT_DEFAULT) {
        quit(ret);
    }
}

void CustomGameMenu::onCancel() {
    quit();
}

void CustomGameMenu::onLoad() {
    auto [ok, savepath] = fnkdat("mpsave/", FNKDAT_USER | FNKDAT_CREAT);
    openWindow(LoadSaveWindow::create(false, _("Load Game"), savepath, "dls").release());
}

void CustomGameMenu::onGameOptions() {
    openWindow(GameOptionsWindow::create(currentGameOptions));
}

void CustomGameMenu::onMapTypeChange(int buttonID) {
    singleplayerMapsButton.setToggleState(buttonID == 0);
    singleplayerUserMapsButton.setToggleState(buttonID == 1);
    multiplayerMapsButton.setToggleState(buttonID == 2);
    multiplayerUserMapsButton.setToggleState(buttonID == 3);

    switch (buttonID) {
        case 0: {
            currentMapDirectory = getDuneLegacyDataDir() / "maps/singleplayer/";
        } break;
        case 1: {
            auto [ok, tmp]      = fnkdat("maps/singleplayer/", FNKDAT_USER | FNKDAT_CREAT);
            currentMapDirectory = tmp;
        } break;
        case 2: {
            currentMapDirectory = getDuneLegacyDataDir() / "maps/multiplayer/";
        } break;
        case 3: {
            auto [ok, tmp]      = fnkdat("maps/multiplayer/", FNKDAT_USER | FNKDAT_CREAT);
            currentMapDirectory = tmp;
        } break;
    }

    currentMapDirectory = currentMapDirectory.lexically_normal().make_preferred();

    mapList.clearAllEntries();

    for (const auto& file :
         getFileNamesList(currentMapDirectory, "ini", true, FileListOrder_Name_CaseInsensitive_Asc)) {
        std::string name{reinterpret_cast<const char*>(file.u8string().c_str())};
        if (name.size() > 4)
            name = name.substr(0, name.size() - 4);
        mapList.addEntry(name);
    }

    if (mapList.getNumEntries() > 0) {
        mapList.setSelectedItem(0);
    } else {
        minimap.setSurface(GUIStyle::getInstance().createButtonSurface(130, 130, _("No map available"), true, false));
        mapPropertySize.setText("");
        mapPropertyPlayers.setText("");
        mapPropertyAuthors.setText("");
        mapPropertyLicense.setText("");
    }
}

void CustomGameMenu::onMapListSelectionChange([[maybe_unused]] bool bInteractive) {
    nextButton.setEnabled(true);

    if (mapList.getSelectedIndex() < 0) {
        return;
    }

    auto mapFilename = currentMapDirectory / mapList.getSelectedEntry();
    mapFilename += ".ini";

    getCaseInsensitiveFilename(mapFilename);

    INIFile inimap(mapFilename);

    int sizeX = 0;
    int sizeY = 0;

    if (inimap.hasKey("MAP", "Seed")) {
        // old map format with seed value
        const int mapscale = inimap.getIntValue("BASIC", "MapScale", -1);

        switch (mapscale) {
            case 0: {
                sizeX = 62;
                sizeY = 62;
            } break;

            case 1: {
                sizeX = 32;
                sizeY = 32;
            } break;

            case 2: {
                sizeX = 21;
                sizeY = 21;
            } break;

            default: {
                sizeX = 64;
                sizeY = 64;
            }
        }
    } else {
        // new map format with saved map
        sizeX = inimap.getIntValue("MAP", "SizeX", 0);
        sizeY = inimap.getIntValue("MAP", "SizeY", 0);
    }

    mapPropertySize.setText(std::to_string(sizeX) + " x " + std::to_string(sizeY));

    sdl2::surface_ptr pMapSurface = nullptr;
    try {
        INIMapPreviewCreator mapPreviewCreator(&inimap);
        pMapSurface = mapPreviewCreator.createMinimapImageOfMap(1, DuneStyle::buttonBorderColor);
    } catch (...) {
        pMapSurface = sdl2::surface_ptr{GUIStyle::getInstance().createButtonSurface(130, 130, "Error", true, false)};
        loadButton.setEnabled(false);
    }
    minimap.setSurface(std::move(pMapSurface));

    int numPlayers = 0;
    if (inimap.hasSection("Atreides"))
        numPlayers++;
    if (inimap.hasSection("Ordos"))
        numPlayers++;
    if (inimap.hasSection("Harkonnen"))
        numPlayers++;
    if (inimap.hasSection("Fremen"))
        numPlayers++;
    if (inimap.hasSection("Mercenary"))
        numPlayers++;
    if (inimap.hasSection("Sardaukar"))
        numPlayers++;
    if (inimap.hasSection("Player1"))
        numPlayers++;
    if (inimap.hasSection("Player2"))
        numPlayers++;
    if (inimap.hasSection("Player3"))
        numPlayers++;
    if (inimap.hasSection("Player4"))
        numPlayers++;
    if (inimap.hasSection("Player5"))
        numPlayers++;
    if (inimap.hasSection("Player6"))
        numPlayers++;

    mapPropertyPlayers.setText(std::to_string(numPlayers));

    std::string authors = inimap.getStringValue("BASIC", "Author", "-");
    if (authors.size() > 11) {
        authors = authors.substr(0, 9) + "...";
    }
    mapPropertyAuthors.setText(authors);

    mapPropertyLicense.setText(inimap.getStringValue("BASIC", "License", "-"));
}
