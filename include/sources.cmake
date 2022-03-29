add_sources(HEADERS
	AITeamInfo.h
	AStarSearch.h
	Bullet.h
	Choam.h
	Colors.h
	Command.h
	CommandManager.h
	config.h
	CutScenes/CrossBlendVideoEvent.h
	CutScenes/CutScene.h
	CutScenes/CutSceneMusicTrigger.h
	CutScenes/CutSceneSoundTrigger.h
	CutScenes/CutSceneTrigger.h
	CutScenes/FadeInVideoEvent.h
	CutScenes/FadeOutVideoEvent.h
	CutScenes/Finale.h
	CutScenes/HoldPictureVideoEvent.h
	CutScenes/Intro.h
	CutScenes/Meanwhile.h
	CutScenes/Scene.h
	CutScenes/TextEvent.h
	CutScenes/VideoEvent.h
	CutScenes/WSAVideoEvent.h
	data.h
	DataTypes.h
	Definitions.h
	Explosion.h
	FileClasses/adl/opl.h
	FileClasses/adl/sound_adlib.h
	FileClasses/adl/surroundopl.h
	FileClasses/adl/wemuopl.h
	FileClasses/adl/woodyopl.h
	FileClasses/Animation.h
	FileClasses/Cpsfile.h
	FileClasses/Decode.h
	FileClasses/FileManager.h
	FileClasses/Font.h
	FileClasses/FontManager.h
	FileClasses/GFXConstants.h
	FileClasses/GFXManager.h
	FileClasses/Icnfile.h
	FileClasses/IndexedTextFile.h
	FileClasses/INIFile.h
	FileClasses/LoadSavePNG.h
	FileClasses/MentatTextFile.h
	FileClasses/music/ADLPlayer.h
	FileClasses/music/DirectoryPlayer.h
	FileClasses/music/MusicPlayer.h
	FileClasses/music/XMIPlayer.h
	FileClasses/Pakfile.h
	FileClasses/Palette.h
	FileClasses/Palfile.h
	FileClasses/PictureFactory.h
	FileClasses/POFile.h
	FileClasses/SaveTextureAsBmp.h
	FileClasses/SFXManager.h
	FileClasses/Shpfile.h
	FileClasses/SurfaceLoader.h
	FileClasses/TextManager.h
	FileClasses/TTFFont.h
	FileClasses/Vocfile.h
	FileClasses/Wsafile.h
	FileClasses/xmidi/databuf.h
	FileClasses/xmidi/xmidi.h
	fixmath/fix16.h
	fixmath/fix16_trig_sin_lut.h
	fixmath/fix32.h
	fixmath/fixmath.h
	fixmath/FixPoint.h
	fixmath/FixPoint16.h
	fixmath/FixPoint32.h
	fixmath/int64.h
	Game.h
	GameInitSettings.h
	GameInterface.h
	globals.h
	GUI/Button.h
	GUI/Checkbox.h
	GUI/ClickMap.h
	GUI/Container.h
	GUI/DropDownBox.h
	GUI/dune/AnimationLabel.h
	GUI/dune/BuilderList.h
	GUI/dune/ChatManager.h
	GUI/dune/DigitsCounter.h
	GUI/dune/DigitsTextBox.h
	GUI/dune/DuneStyle.h
	GUI/dune/GameOptionsWindow.h
	GUI/dune/InGameMenu.h
	GUI/dune/InGameSettingsMenu.h
	GUI/dune/LoadSaveWindow.h
	GUI/dune/MessageTicker.h
	GUI/dune/NewsTicker.h
	GUI/dune/WaitingForOtherPlayers.h
	GUI/GUIStyle.h
	GUI/HBox.h
	GUI/InvisibleButton.h
	GUI/Label.h
	GUI/ListBox.h
	GUI/MsgBox.h
	GUI/ObjectInterfaces/BuilderInterface.h
	GUI/ObjectInterfaces/DefaultObjectInterface.h
	GUI/ObjectInterfaces/DefaultStructureInterface.h
	GUI/ObjectInterfaces/MultiUnitInterface.h
	GUI/ObjectInterfaces/ObjectInterface.h
	GUI/ObjectInterfaces/PalaceInterface.h
	GUI/ObjectInterfaces/RadarInterface.h
	GUI/ObjectInterfaces/RefineryAndSiloInterface.h
	GUI/ObjectInterfaces/RepairYardInterface.h
	GUI/ObjectInterfaces/UnitInterface.h
	GUI/ObjectInterfaces/WindTrapInterface.h
	GUI/PictureButton.h
	GUI/PictureLabel.h
	GUI/ProgressBar.h
	GUI/QstBox.h
	GUI/RadioButton.h
	GUI/RadioButtonManager.h
	GUI/ScrollBar.h
	GUI/Spacer.h
	GUI/StaticContainer.h
	GUI/SymbolButton.h
	GUI/TextBox.h
	GUI/TextButton.h
	GUI/TextView.h
	GUI/VBox.h
	GUI/Widget.h
	GUI/WidgetWithBackground.h
	GUI/Window.h
	House.h
	INIMap/INIMap.h
	INIMap/INIMapEditorLoader.h
	INIMap/INIMapLoader.h
	INIMap/INIMapPreviewCreator.h
	Map.h
	MapEditor/ChoamWindow.h
	MapEditor/LoadMapWindow.h
	MapEditor/MapData.h
	MapEditor/MapEditor.h
	MapEditor/MapEditorInterface.h
	MapEditor/MapEditorOperation.h
	MapEditor/MapEditorRadarView.h
	MapEditor/MapGenerator.h
	MapEditor/MapInfo.h
	MapEditor/MapMirror.h
	MapEditor/MapSettingsWindow.h
	MapEditor/NewMapWindow.h
	MapEditor/PlayerSettingsWindow.h
	MapEditor/ReinforcementInfo.h
	MapEditor/ReinforcementsWindow.h
	MapEditor/TeamsWindow.h
	MapSeed.h
	Menu/AboutMenu.h
	Menu/BriefingMenu.h
	Menu/CampaignStatsMenu.h
	Menu/CustomGameMenu.h
	Menu/CustomGamePlayers.h
	Menu/CustomGameStatsMenu.h
	Menu/HouseChoiceInfoMenu.h
	Menu/HouseChoiceMenu.h
	Menu/MainMenu.h
	Menu/MapChoice.h
	Menu/MentatHelp.h
	Menu/MentatMenu.h
	Menu/MenuBase.h
	Menu/MultiPlayerMenu.h
	Menu/OptionsMenu.h
	Menu/SinglePlayerMenu.h
	Menu/SinglePlayerSkirmishMenu.h
	misc/BlendBlitter.h
	misc/DrawingRectHelper.h
	misc/draw_util.h
	misc/dune_clock.h
	misc/dune_endian.h
	misc/dune_localtime.h
	misc/dune_wait_event.h
	misc/exceptions.h
	misc/FileSystem.h
	misc/fnkdat.h
	misc/generator.h
	misc/IFileStream.h
	misc/IMemoryStream.h
	misc/InputStream.h
	misc/lemire_uniform_uint32_distribution.h
	misc/md5.h
	misc/OFileStream.h
	misc/OMemoryStream.h
	misc/OutputStream.h
	misc/Random.h
	misc/random_uint64_to_uint32.h
	misc/random_xoroshiro128plus.h
	misc/random_xorshift1024star.h
	misc/random_xoshiro256starstar.h
	misc/reverse.h
	misc/RobustList.h
	misc/Scaler.h
	misc/SDL2pp.h
	misc/sdl_support.h
	misc/sound_util.h
	misc/string_error.h
	misc/string_util.h
	misc/unique_or_nonowning_ptr.h
	mmath.h
	Network/ChangeEventList.h
	Network/CommandList.h
	Network/ENetHelper.h
	Network/ENetHttp.h
	Network/ENetPacketIStream.h
	Network/ENetPacketOStream.h
	Network/GameServerInfo.h
	Network/LANGameFinderAndAnnouncer.h
	Network/MetaServerClient.h
	Network/MetaServerCommands.h
	Network/NetworkManager.h
	ObjectBase.h
	ObjectData.h
	ObjectManager.h
	ObjectPointer.h
	players/AIPlayer.h
	players/CampaignAIPlayer.h
	players/HumanPlayer.h
	players/Player.h
	players/PlayerFactory.h
	players/QuantBot.h
	players/SmartBot.h
	RadarView.h
	RadarViewBase.h
	Renderer/DuneRenderer.h
	Renderer/DuneRotateTexture.h
	Renderer/DuneTexture.h
	Renderer/DuneTextures.h
	Renderer/DuneTileTexture.h
	sand.h
	ScreenBorder.h
	SoundPlayer.h
	structures/Barracks.h
	structures/BuilderBase.h
	structures/ConstructionYard.h
	structures/GunTurret.h
	structures/HeavyFactory.h
	structures/HighTechFactory.h
	structures/IX.h
	structures/LightFactory.h
	structures/Palace.h
	structures/Radar.h
	structures/Refinery.h
	structures/RepairYard.h
	structures/RocketTurret.h
	structures/Silo.h
	structures/StarPort.h
	structures/StructureBase.h
	structures/TurretBase.h
	structures/Wall.h
	structures/WindTrap.h
	structures/WOR.h
	Tile.h
	Trigger/ReinforcementTrigger.h
	Trigger/TimeoutTrigger.h
	Trigger/Trigger.h
	Trigger/TriggerManager.h
	units/AirUnit.h
	units/Carryall.h
	units/Devastator.h
	units/Deviator.h
	units/Frigate.h
	units/GroundUnit.h
	units/Harvester.h
	units/InfantryBase.h
	units/Launcher.h
	units/MCV.h
	units/Ornithopter.h
	units/Quad.h
	units/RaiderTrike.h
	units/Saboteur.h
	units/SandWorm.h
	units/SiegeTank.h
	units/Soldier.h
	units/SonicTank.h
	units/Tank.h
	units/TankBase.h
	units/TrackedUnit.h
	units/Trike.h
	units/Trooper.h
	units/UnitBase.h
)
