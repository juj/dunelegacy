
#include <Menu/CustomGameMenu.h>
#include <Menu/CustomGamePlayers.h>
#include <Menu/MultiPlayerMenu.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

#include <Network/ENetHelper.h>
#include <Network/NetworkManager.h>

#include <GUI/MsgBox.h>

#include <globals.h>

#include <misc/string_util.h>

MultiPlayerMenu::MultiPlayerMenu() {
    // set up window

    MultiPlayerMenu::setWindowWidget(&windowWidget);

    windowWidget.addWidget(&mainVBox, Point(24, 23), Point(getRendererWidth() - 48, getRendererHeight() - 46));

    captionLabel.setText("Multiplayer Game");
    captionLabel.setAlignment(Alignment_HCenter);
    mainVBox.addWidget(&captionLabel, 24);
    mainVBox.addWidget(Widget::create<VSpacer>(24).release());

    connectHBox.addWidget(Label::create("Host:").release(), 50);
    connectHostTextBox.setText("localhost");
    connectHBox.addWidget(&connectHostTextBox);
    connectHBox.addWidget(Widget::create<HSpacer>(20).release());
    connectHBox.addWidget(Label::create("Port:").release(), 50);
    connectPortTextBox.setText(std::to_string(DEFAULT_PORT));
    connectHBox.addWidget(&connectPortTextBox, 90);
    connectHBox.addWidget(Widget::create<HSpacer>(20).release());
    connectButton.setText(_("Connect"));
    connectButton.setOnClick([this] { onConnect(); });
    connectHBox.addWidget(&connectButton, 100);

    mainVBox.addWidget(Widget::create<Spacer>().release(), 0.05);
    mainVBox.addWidget(&connectHBox, 28);

    mainVBox.addWidget(Widget::create<VSpacer>(16).release());

    mainVBox.addWidget(Widget::create<Spacer>().release(), 0.05);
    mainVBox.addWidget(&mainHBox, 0.85);

    mainHBox.addWidget(&leftVBox, 180);

    createLANGameButton.setText(_("Create LAN Game"));
    createLANGameButton.setOnClick([this] { onCreateLANGame(); });
    leftVBox.addWidget(&createLANGameButton, 0.1);

    leftVBox.addWidget(Widget::create<VSpacer>(8).release());

    createInternetGameButton.setText(_("Create Internet Game"));
    createInternetGameButton.setOnClick([this] { onCreateInternetGame(); });
    leftVBox.addWidget(&createInternetGameButton, 0.1);

    leftVBox.addWidget(Widget::create<Spacer>().release(), 0.8);

    rightVBox.addWidget(&gameTypeButtonsHBox, 24);

    mainHBox.addWidget(Widget::create<HSpacer>(8).release());
    mainHBox.addWidget(Widget::create<Spacer>().release(), 0.05);

    LANGamesButton.setText(_("LAN Games"));
    LANGamesButton.setToggleButton(true);
    LANGamesButton.setOnClick([this] { onGameTypeChange(0); });
    gameTypeButtonsHBox.addWidget(&LANGamesButton, 0.35);

    internetGamesButton.setText(_("Internet Games"));
    internetGamesButton.setToggleButton(true);
    internetGamesButton.setOnClick([this] { onGameTypeChange(1); });
    gameTypeButtonsHBox.addWidget(&internetGamesButton, 0.35);

    gameTypeButtonsHBox.addWidget(Widget::create<Spacer>().release(), 0.3);

    gameList.setAutohideScrollbar(false);
    gameList.setOnSelectionChange([this](auto interactive) { onGameListSelectionChange(interactive); });
    gameList.setOnDoubleClick([this] { onJoin(); });
    rightVBox.addWidget(&gameList, 0.95);

    mainHBox.addWidget(&rightVBox, 0.9);
    mainHBox.addWidget(Widget::create<Spacer>().release(), 0.05);

    mainVBox.addWidget(Widget::create<Spacer>().release(), 0.05);
    mainVBox.addWidget(Widget::create<VSpacer>(10).release());

    mainVBox.addWidget(&buttonHBox, 24);

    buttonHBox.addWidget(Widget::create<HSpacer>(70).release());
    backButton.setText(_("Back"));
    backButton.setOnClick([] { onQuit(); });
    buttonHBox.addWidget(&backButton, 0.1);

    buttonHBox.addWidget(Widget::create<Spacer>().release(), 0.8);

    joinButton.setText(_("Join"));
    joinButton.setOnClick([this] { onJoin(); });
    buttonHBox.addWidget(&joinButton, 0.1);
    buttonHBox.addWidget(Widget::create<HSpacer>(90).release());

    // Start Network Manager
    sdl2::log_info("Starting network...");
    const auto& network = dune::globals::settings.network;
    dune::globals::pNetworkManager =
        std::make_unique<NetworkManager>(static_cast<uint16_t>(network.serverPort), network.metaServer);
    LANGameFinderAndAnnouncer* const pLANGFAA = dune::globals::pNetworkManager->getLANGameFinderAndAnnouncer();
    pLANGFAA->setOnNewServer([this](auto interactive) { onNewLANServer(interactive); });
    pLANGFAA->setOnUpdateServer([this](auto interactive) { onUpdateLANServer(interactive); });
    pLANGFAA->setOnRemoveServer([this](auto interactive) { onRemoveLANServer(interactive); });
    pLANGFAA->refreshServerList();

    onGameTypeChange(0);
}

MultiPlayerMenu::~MultiPlayerMenu() {
    sdl2::log_info("Stopping network...");
    dune::globals::pNetworkManager.reset();
}

/**
    This method is called, when the child window is about to be closed.
    This child window will be closed after this method returns.
    \param  pChildWindow    The child window that will be closed
*/
void MultiPlayerMenu::onChildWindowClose([[maybe_unused]] Window* pChildWindow) {
    // Connection canceled
    // TODO
}

void MultiPlayerMenu::onCreateLANGame() {
    CustomGameMenu(true, true).showMenu(sdl_handler_);
}

void MultiPlayerMenu::onCreateInternetGame() {
    CustomGameMenu(true, false).showMenu(sdl_handler_);
}

void MultiPlayerMenu::onConnect() {
    const std::string hostname{connectHostTextBox.getText()};

    uint16_t port        = 0;
    const auto port_text = connectPortTextBox.getText();
    if (!parseString(port_text, port))
        THROW(std::invalid_argument, "NetworkManager: Invalid port '%s'!", port_text);

    auto* const network_manager = dune::globals::pNetworkManager.get();

    network_manager->setOnReceiveGameInfo(
        [this](const auto& settings, const auto& events) { onReceiveGameInfo(settings, events); });
    network_manager->setOnPeerDisconnected(
        [this](auto playername, auto host, auto cause) { onPeerDisconnected(playername, host, cause); });
    network_manager->connect(hostname, port, dune::globals::settings.general.playerName);

    openWindow(MsgBox::create(_("Connecting...")));
}

void MultiPlayerMenu::onPeerDisconnected([[maybe_unused]] const std::string& playername, bool bHost, int cause) {
    if (bHost) {
        auto* const network_manager = dune::globals::pNetworkManager.get();

        network_manager->setOnReceiveGameInfo(std::function<void(const GameInitSettings&, const ChangeEventList&)>());
        network_manager->setOnPeerDisconnected(std::function<void(const std::string&, bool, int)>());
        closeChildWindow();

        showDisconnectMessageBox(cause);
    }
}

void MultiPlayerMenu::onJoin() {
    const int selectedEntry = gameList.getSelectedIndex();
    if (selectedEntry >= 0) {
        const auto* pGameServerInfo = static_cast<GameServerInfo*>(gameList.getEntryPtrData(selectedEntry));

        auto* const network_manager = dune::globals::pNetworkManager.get();

        network_manager->setOnReceiveGameInfo(
            [this](const auto& settings, const auto& events) { onReceiveGameInfo(settings, events); });
        network_manager->setOnPeerDisconnected(
            [this](auto playername, auto host, auto cause) { onPeerDisconnected(playername, host, cause); });
        network_manager->connect(pGameServerInfo->serverAddress, dune::globals::settings.general.playerName);

        openWindow(MsgBox::create(_("Connecting...")));
    }
}

void MultiPlayerMenu::onQuit() {
    SDL_Event quitEvent;
    quitEvent.type = SDL_QUIT;
    SDL_PushEvent(&quitEvent);
}

void MultiPlayerMenu::onGameTypeChange(int buttonID) {
    MetaServerClient* pMetaServerClient = dune::globals::pNetworkManager->getMetaServerClient();
    if (buttonID == 0 && internetGamesButton.getToggleState()) {
        // LAN Games

        gameList.clearAllEntries();
        InternetGameList.clear();

        for (GameServerInfo& gameServerInfo : LANGameList) {
            std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress)
                                    + " : " + std::to_string(gameServerInfo.serverAddress.port) + ") - "
                                    + gameServerInfo.mapName + " (" + std::to_string(gameServerInfo.numPlayers) + "/"
                                    + std::to_string(gameServerInfo.maxPlayers) + ")";
            gameList.addEntry(description, &gameServerInfo);
        }

        // stop listening on internet games
        pMetaServerClient->setOnGameServerInfoList(std::function<void(std::list<GameServerInfo>&)>());
        pMetaServerClient->setOnMetaServerError(std::function<void(int, const std::string&)>());
    } else if (buttonID == 1 && LANGamesButton.getToggleState()) {
        // Internet Games

        gameList.clearAllEntries();
        InternetGameList.clear();

        // start listening on internet games
        pMetaServerClient->setOnGameServerInfoList([this](const auto& list) { onGameServerInfoList(list); });
        pMetaServerClient->setOnMetaServerError(
            [this](auto cause, const auto& message) { onMetaServerError(cause, message); });
    }

    LANGamesButton.setToggleState(buttonID == 0);
    internetGamesButton.setToggleState(buttonID == 1);
}

void MultiPlayerMenu::onGameListSelectionChange([[maybe_unused]] bool bInteractive) { }

void MultiPlayerMenu::onNewLANServer(GameServerInfo gameServerInfo) {
    LANGameList.push_back(gameServerInfo);
    const std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress)
                                  + " : " + std::to_string(gameServerInfo.serverAddress.port) + ") - "
                                  + gameServerInfo.mapName + " (" + std::to_string(gameServerInfo.numPlayers) + "/"
                                  + std::to_string(gameServerInfo.maxPlayers) + ")";
    gameList.addEntry(description, &LANGameList.back());
}

void MultiPlayerMenu::onUpdateLANServer(GameServerInfo gameServerInfo) {
    size_t index = 0;
    for (GameServerInfo& curGameServerInfo : LANGameList) {
        if (curGameServerInfo == gameServerInfo) {
            curGameServerInfo = gameServerInfo;
            break;
        }

        index++;
    }

    if (index < LANGameList.size()) {
        const std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress)
                                      + " : " + std::to_string(gameServerInfo.serverAddress.port) + ") - "
                                      + gameServerInfo.mapName + " (" + std::to_string(gameServerInfo.numPlayers) + "/"
                                      + std::to_string(gameServerInfo.maxPlayers) + ")";

        gameList.setEntry(index, description);
    }
}

void MultiPlayerMenu::onRemoveLANServer(GameServerInfo gameServerInfo) {
    for (int i = 0; i < gameList.getNumEntries(); i++) {
        const auto* pGameServerInfo = static_cast<GameServerInfo*>(gameList.getEntryPtrData(i));
        if (*pGameServerInfo == gameServerInfo) {
            gameList.removeEntry(i);
            break;
        }
    }

    LANGameList.remove(gameServerInfo);
}

void MultiPlayerMenu::onGameServerInfoList(const std::list<GameServerInfo>& gameServerInfoList) {
    // remove all game servers from the list that are not included in the sent list
    { // Scope
        auto oldListIter = InternetGameList.begin();
        int index        = 0;
        while (oldListIter != InternetGameList.end()) {
            GameServerInfo& gameServerInfo = *oldListIter;

            if (std::ranges::find(gameServerInfoList, gameServerInfo) == gameServerInfoList.end()) {
                // not found => remove
                gameList.removeEntry(index);
                oldListIter = InternetGameList.erase(oldListIter);
            } else {
                // found => move on
                ++oldListIter;
                ++index;
            }
        }
    }

    // now add all servers that are included for the first time and update the others
    for (const GameServerInfo& gameServerInfo : gameServerInfoList) {
        size_t oldListIndex = 0;
        std::list<GameServerInfo>::iterator oldListIter;
        for (oldListIter = InternetGameList.begin(); oldListIter != InternetGameList.end(); ++oldListIter) {
            if (*oldListIter == gameServerInfo) {
                // found => update
                *oldListIter = gameServerInfo;
                break;
            }

            oldListIndex++;
        }

        if (oldListIndex >= InternetGameList.size()) {
            // not found => add at the end
            InternetGameList.push_back(gameServerInfo);
            std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress)
                                    + " : " + std::to_string(gameServerInfo.serverAddress.port) + ") - "
                                    + gameServerInfo.mapName + " (" + std::to_string(gameServerInfo.numPlayers) + "/"
                                    + std::to_string(gameServerInfo.maxPlayers) + ")";
            gameList.addEntry(description, &InternetGameList.back());
        } else {
            // found => update
            std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress)
                                    + " : " + std::to_string(gameServerInfo.serverAddress.port) + ") - "
                                    + gameServerInfo.mapName + " (" + std::to_string(gameServerInfo.numPlayers) + "/"
                                    + std::to_string(gameServerInfo.maxPlayers) + ")";

            gameList.setEntry(oldListIndex, description);
        }
    }
}

void MultiPlayerMenu::onMetaServerError(int errorcause, const std::string& errorMessage) {
    switch (errorcause) {
        case METASERVERCOMMAND_ADD: {
            openWindow(MsgBox::create("MetaServer error on adding game server:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_UPDATE: {
            openWindow(MsgBox::create("MetaServer error on updating game server:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_REMOVE: {
            openWindow(MsgBox::create("MetaServer error on removing game server:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_LIST: {
            openWindow(MsgBox::create("MetaServer error on list game servers:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_EXIT:
        default: {
            openWindow(MsgBox::create("MetaServer error:\n" + errorMessage));
        } break;
    }
}

void MultiPlayerMenu::onReceiveGameInfo(const GameInitSettings& gameInitSettings,
                                        const ChangeEventList& changeEventList) {
    closeChildWindow();

    dune::globals::pNetworkManager->setOnPeerDisconnected({});

    auto pCustomGamePlayers = std::make_unique<CustomGamePlayers>(gameInitSettings, false);
    pCustomGamePlayers->onReceiveChangeEventList(changeEventList);
    const int ret = pCustomGamePlayers->showMenu(sdl_handler_);
    pCustomGamePlayers.reset();

    switch (ret) {
        case MENU_QUIT_DEFAULT: {
            // nothing
        } break;

        case MENU_QUIT_GAME_FINISHED: {
            quit(MENU_QUIT_GAME_FINISHED);
        } break;

        default: {
            showDisconnectMessageBox(ret);
        } break;
    }
}

void MultiPlayerMenu::showDisconnectMessageBox(int cause) {
    switch (cause) {
        case NETWORKDISCONNECT_QUIT: {
            openWindow(MsgBox::create(_("The game host quit the game!")));
        } break;

        case NETWORKDISCONNECT_TIMEOUT: {
            openWindow(MsgBox::create(_("The server timed out!")));
        } break;

        case NETWORKDISCONNECT_PLAYER_EXISTS: {
            openWindow(MsgBox::create(_("A player with the same name as yours already exists!")));
        } break;

        case NETWORKDISCONNECT_GAME_FULL: {
            openWindow(MsgBox::create(_("There is no free player slot in this game left!")));
        } break;

        default: {
            openWindow(MsgBox::create(_("The connection to the game host was lost!")));
        } break;
    }
}
