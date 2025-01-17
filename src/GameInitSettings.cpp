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

#include <GameInitSettings.h>

#include <misc/IFileStream.h>
#include <misc/IMemoryStream.h>
#include <misc/exceptions.h>

#include <globals.h>

#include "misc/Random.h"

GameInitSettings::PlayerInfo::PlayerInfo(std::string newPlayerName, std::string newPlayerClass)
    : playerName(std::move(newPlayerName)), playerClass(std::move(newPlayerClass)) { }

GameInitSettings::PlayerInfo::PlayerInfo(InputStream& stream) {
    playerName  = stream.readString();
    playerClass = stream.readString();
}

GameInitSettings::PlayerInfo::PlayerInfo(const PlayerInfo&) = default;

GameInitSettings::PlayerInfo::PlayerInfo(PlayerInfo&&) noexcept = default;

GameInitSettings::PlayerInfo& GameInitSettings::PlayerInfo::operator=(const PlayerInfo&) = default;

GameInitSettings::PlayerInfo& GameInitSettings::PlayerInfo::operator=(PlayerInfo&&) noexcept = default;

GameInitSettings::PlayerInfo::~PlayerInfo() = default;

void GameInitSettings::PlayerInfo::save(OutputStream& stream) const {
    stream.writeString(playerName);
    stream.writeString(playerClass);
}

GameInitSettings::HouseInfo::HouseInfo(HOUSETYPE newHouseID, int newTeam) : houseID(newHouseID), team(newTeam) { }

GameInitSettings::HouseInfo::HouseInfo(InputStream& stream) {
    houseID = static_cast<HOUSETYPE>(stream.readSint32());
    team    = stream.readSint32();

    const auto numPlayerInfo = stream.readUint32();
    for (auto i = 0U; i < numPlayerInfo; i++) {
        playerInfoList.emplace_back(stream);
    }
}

GameInitSettings::HouseInfo::HouseInfo(const HouseInfo&) = default;

GameInitSettings::HouseInfo::HouseInfo(HouseInfo&&) noexcept = default;

GameInitSettings::HouseInfo& GameInitSettings::HouseInfo::operator=(const HouseInfo&) = default;

GameInitSettings::HouseInfo& GameInitSettings::HouseInfo::operator=(HouseInfo&&) noexcept = default;

GameInitSettings::HouseInfo::~HouseInfo() = default;

void GameInitSettings::HouseInfo::save(OutputStream& stream) const {
    stream.writeSint32(static_cast<int32_t>(houseID));
    stream.writeSint32(team);

    stream.writeUint32(static_cast<uint32_t>(playerInfoList.size()));
    for (const auto& playerInfo : playerInfoList) {
        playerInfo.save(stream);
    }
}

void GameInitSettings::HouseInfo::addPlayerInfo(PlayerInfo&& newPlayerInfo) {
    playerInfoList.emplace_back(std::move(newPlayerInfo));
}

GameInitSettings::GameInitSettings() = default;

GameInitSettings::GameInitSettings(HOUSETYPE newHouseID, const SettingsClass::GameOptionsClass& gameOptions)
    : gameType(GameType::Campaign), houseID(newHouseID), mission(1), alreadyShownTutorialHints(0),
      gameOptions(gameOptions) {
    filename = getScenarioFilename(houseID, mission);
}

GameInitSettings::GameInitSettings(const GameInitSettings& prevGameInitInfoClass, int nextMission,
                                   uint32_t alreadyPlayedRegions, uint32_t alreadyShownTutorialHints) {
    *this                           = prevGameInitInfoClass;
    mission                         = nextMission;
    this->alreadyPlayedRegions      = alreadyPlayedRegions;
    this->alreadyShownTutorialHints = alreadyShownTutorialHints;
    filename                        = getScenarioFilename(houseID, mission);
}

GameInitSettings::GameInitSettings(HOUSETYPE newHouseID, int newMission,
                                   const SettingsClass::GameOptionsClass& gameOptions)
    : gameType(GameType::Skirmish), houseID(newHouseID), mission(newMission), gameOptions(gameOptions) {
    filename = getScenarioFilename(houseID, mission);
}

GameInitSettings::GameInitSettings(std::filesystem::path&& mapfile, std::string&& filedata,
                                   bool multiplePlayersPerHouse, const SettingsClass::GameOptionsClass& gameOptions)
    : gameType(GameType::CustomGame), filename(std::move(mapfile)), filedata(std::move(filedata)),
      multiplePlayersPerHouse(multiplePlayersPerHouse), gameOptions(gameOptions) { }

GameInitSettings::GameInitSettings(std::filesystem::path&& mapfile, std::string&& filedata, std::string&& serverName,
                                   bool multiplePlayersPerHouse, const SettingsClass::GameOptionsClass& gameOptions)
    : gameType(GameType::CustomMultiplayer), filename(std::move(mapfile)), filedata(std::move(filedata)),
      servername(std::move(serverName)), multiplePlayersPerHouse(multiplePlayersPerHouse), gameOptions(gameOptions) { }

GameInitSettings::GameInitSettings(std::filesystem::path&& savegame)
    : gameType(GameType::LoadSavegame), filename(std::move(savegame)) {
    checkSaveGame(filename);
}

GameInitSettings::GameInitSettings(std::filesystem::path&& savegame, std::string&& filedata, std::string&& serverName)
    : gameType(GameType::LoadMultiplayer), filename(savegame), filedata(filedata), servername(serverName) {
    IMemoryStream memStream(filedata.c_str(), filedata.size());
    checkSaveGame(memStream);
}

GameInitSettings::GameInitSettings(InputStream& stream) {
    gameType = static_cast<GameType>(stream.readSint8());
    houseID  = static_cast<HOUSETYPE>(stream.readSint8());

    filename = stream.readString();
    filedata = stream.readString();

    mission                   = stream.readUint8();
    alreadyPlayedRegions      = stream.readUint32();
    alreadyShownTutorialHints = stream.readUint32();
    randomSeed                = stream.readUint8Vector();

    multiplePlayersPerHouse                  = stream.readBool();
    gameOptions.gameSpeed                    = static_cast<int>(stream.readUint32());
    gameOptions.concreteRequired             = stream.readBool();
    gameOptions.structuresDegradeOnConcrete  = stream.readBool();
    gameOptions.fogOfWar                     = stream.readBool();
    gameOptions.startWithExploredMap         = stream.readBool();
    gameOptions.instantBuild                 = stream.readBool();
    gameOptions.onlyOnePalace                = stream.readBool();
    gameOptions.rocketTurretsNeedPower       = stream.readBool();
    gameOptions.sandwormsRespawn             = stream.readBool();
    gameOptions.killedSandwormsDropSpice     = stream.readBool();
    gameOptions.manualCarryallDrops          = stream.readBool();
    gameOptions.maximumNumberOfUnitsOverride = stream.readSint32();

    const auto numHouseInfo = stream.readUint32();
    for (auto i = 0U; i < numHouseInfo; i++) {
        houseInfoList.emplace_back(stream);
    }
}

GameInitSettings::GameInitSettings(const GameInitSettings&) = default;

GameInitSettings::GameInitSettings(GameInitSettings&&) noexcept = default;

GameInitSettings& GameInitSettings::operator=(const GameInitSettings&) = default;

GameInitSettings& GameInitSettings::operator=(GameInitSettings&&) noexcept = default;

GameInitSettings::~GameInitSettings() = default;

void GameInitSettings::save(OutputStream& stream) const {
    stream.writeSint8(static_cast<int8_t>(gameType));
    stream.writeSint8(static_cast<int8_t>(houseID));

    stream.writeString(reinterpret_cast<const char*>(filename.u8string().c_str()));
    stream.writeString(filedata);

    stream.writeUint8(static_cast<Uint8>(mission));
    stream.writeUint32(alreadyPlayedRegions);
    stream.writeUint32(alreadyShownTutorialHints);
    stream.writeUint8Vector(randomSeed);

    stream.writeBool(multiplePlayersPerHouse);
    stream.writeUint32(gameOptions.gameSpeed);
    stream.writeBool(gameOptions.concreteRequired);
    stream.writeBool(gameOptions.structuresDegradeOnConcrete);
    stream.writeBool(gameOptions.fogOfWar);
    stream.writeBool(gameOptions.startWithExploredMap);
    stream.writeBool(gameOptions.instantBuild);
    stream.writeBool(gameOptions.onlyOnePalace);
    stream.writeBool(gameOptions.rocketTurretsNeedPower);
    stream.writeBool(gameOptions.sandwormsRespawn);
    stream.writeBool(gameOptions.killedSandwormsDropSpice);
    stream.writeBool(gameOptions.manualCarryallDrops);
    stream.writeSint32(gameOptions.maximumNumberOfUnitsOverride);

    stream.writeUint32(houseInfoList.size());
    for (const auto& houseInfo : houseInfoList) {
        houseInfo.save(stream);
    }
}

const std::vector<uint8_t>& GameInitSettings::getRandomSeed() noexcept {
    if (randomSeed.empty())
        randomSeed = RandomFactory::createRandomSeed("game master seed");

    return randomSeed;
}

std::string GameInitSettings::getScenarioFilename(HOUSETYPE newHouse, int mission) {
    if ((static_cast<int>(newHouse) < 0) || (newHouse >= HOUSETYPE::NUM_HOUSES)) {
        THROW(std::invalid_argument, "GameInitSettings::getScenarioFilename(): Invalid house id "
                                         + std::to_string(static_cast<int>(newHouse)) + ".");
    }

    if ((mission < 0) || (mission > 22)) {
        THROW(std::invalid_argument,
              "GameInitSettings::getScenarioFilename(): There is no mission number " + std::to_string(mission) + ".");
    }

    std::string name = "SCEN?0??.INI";
    name[4]          = dune::globals::houseChar[static_cast<int>(newHouse)];

    name[6] = static_cast<char>('0' + (mission / 10));
    name[7] = static_cast<char>('0' + (mission % 10));

    return name;
}

void GameInitSettings::checkSaveGame(const std::filesystem::path& savegame) {
    IFileStream fs;

    if (!fs.open(savegame)) {
        THROW(std::runtime_error, "Cannot open savegame. Make sure you have read access to this savegame!");
    }

    checkSaveGame(fs);

    fs.close();
}

void GameInitSettings::checkSaveGame(InputStream& stream) {
    uint32_t magicNum        = 0;
    uint32_t savegameVersion = 0;
    std::string duneVersion;
    try {
        magicNum        = stream.readUint32();
        savegameVersion = stream.readUint32();
        duneVersion     = stream.readString();
    } catch (std::exception&) {
        THROW(std::runtime_error, "Cannot load this savegame,\n because it seems to be truncated!");
    }

    if (magicNum != SAVEMAGIC) {
        THROW(std::runtime_error, "Cannot load this savegame,\n because it has a wrong magic number!");
    }

    if (savegameVersion < SAVEGAMEVERSION) {
        THROW(std::runtime_error,
              "Cannot load this savegame,\n because it was created with an older version:\n" + duneVersion);
    }

    if (savegameVersion > SAVEGAMEVERSION) {
        THROW(std::runtime_error,
              "Cannot load this savegame,\n because it was created with a newer version:\n" + duneVersion);
    }
}
