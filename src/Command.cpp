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

#include <Command.h>

#include <globals.h>

#include <Game.h>
#include <House.h>

#include <misc/exceptions.h>

#include <structures/BuilderBase.h>
#include <structures/ConstructionYard.h>
#include <structures/Palace.h>
#include <structures/StarPort.h>
#include <structures/TurretBase.h>
#include <units/Carryall.h>
#include <units/Devastator.h>
#include <units/GroundUnit.h>
#include <units/Harvester.h>
#include <units/InfantryBase.h>
#include <units/MCV.h>
#include <units/UnitBase.h>

Command::Command(uint8_t playerID, uint8_t* data, uint32_t length) : playerID(playerID) {
    if (length % 4 != 0) {
        THROW(std::invalid_argument, "Command::Command(): Length must be multiple of 4!");
    }

    if (length < 4) {
        THROW(std::invalid_argument, "Command::Command(): Command must be at least 4 bytes long!");
    }

    commandID = static_cast<CMDTYPE>(*reinterpret_cast<uint32_t*>(data));

    if (commandID >= CMDTYPE::CMD_MAX) {
        THROW(std::invalid_argument, "Command::Command(): CommandID unknown!");
    }

    const auto count = (length - 4) / 4;

    parameter.reserve(count);

    auto* pData = reinterpret_cast<uint32_t*>(data + 4);
    for (auto i = 0u; i < count; i++) {
        parameter.push_back(*pData);
        pData++;
    }
}

Command::Command(InputStream& stream) {
    playerID  = stream.readUint8();
    commandID = static_cast<CMDTYPE>(stream.readUint32());
    parameter = stream.readUint32Vector();
}

Command::~Command() = default;

void Command::save(OutputStream& stream) const {
    stream.writeUint8(playerID);
    stream.writeUint32(static_cast<uint32_t>(commandID));
    stream.writeUint32Vector(parameter);
    stream.flush();
}

void Command::executeCommand(const GameContext& context) const {
    auto& [game, map, objectManager] = context;

    switch (commandID) {

        case CMDTYPE::CMD_PLACE_STRUCTURE: {
            if (parameter.size() != 3) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_PLACE_STRUCTURE needs 3 Parameters!");
            }
            auto* const pConstYard = objectManager.getObject<ConstructionYard>(parameter[0]);
            if (pConstYard == nullptr) {
                return;
            }
            pConstYard->doPlaceStructure(static_cast<int>(parameter[1]), static_cast<int>(parameter[2]));
        } break;

        case CMDTYPE::CMD_UNIT_MOVE2POS: {
            if (parameter.size() != 4) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_UNIT_MOVE2POS needs 4 Parameters!");
            }
            auto* const unit = objectManager.getObject<UnitBase>(parameter[0]);
            if (unit == nullptr) {
                return;
            }
            unit->doMove2Pos(context, static_cast<int>(parameter[1]), static_cast<int>(parameter[2]),
                             static_cast<bool>(parameter[3]));
        } break;

        case CMDTYPE::CMD_UNIT_MOVE2OBJECT: {
            if (parameter.size() != 2) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_UNIT_MOVE2OBJECT needs 2 Parameters!");
            }
            auto* const unit = objectManager.getObject<UnitBase>(parameter[0]);
            if (unit == nullptr) {
                return;
            }
            unit->doMove2Object(context, static_cast<int>(parameter[1]));
        } break;

        case CMDTYPE::CMD_UNIT_ATTACKPOS: {
            if (parameter.size() != 4) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_UNIT_ATTACKPOS needs 4 Parameters!");
            }
            auto* const unit = objectManager.getObject<UnitBase>(parameter[0]);
            if (unit == nullptr) {
                return;
            }
            unit->doAttackPos(context, static_cast<int>(parameter[1]), static_cast<int>(parameter[2]),
                              static_cast<bool>(parameter[3]));
        } break;

        case CMDTYPE::CMD_UNIT_ATTACKOBJECT: {
            if (parameter.size() != 2) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_UNIT_ATTACKOBJECT needs 2 Parameters!");
            }
            auto* const pUnit = objectManager.getObject<UnitBase>(parameter[0]);
            if (pUnit == nullptr) {
                return;
            }
            pUnit->doAttackObject(context, static_cast<int>(parameter[1]), true);
        } break;

        case CMDTYPE::CMD_INFANTRY_CAPTURE: {
            if (parameter.size() != 2) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_INFANTRY_CAPTURE needs 2 Parameters!");
            }
            auto* const pInfantry = objectManager.getObject<InfantryBase>(parameter[0]);
            if (pInfantry == nullptr) {
                return;
            }
            pInfantry->doCaptureStructure(context, static_cast<int>(parameter[1]));
        } break;

        case CMDTYPE::CMD_UNIT_REQUESTCARRYALLDROP: {
            if (parameter.size() != 3) {
                THROW(std::invalid_argument,
                      "Command::executeCommand(): CMD_UNIT_REQUESTCARRYALLDROP needs 3 Parameters!");
            }
            auto* const pGroundUnit = objectManager.getObject<GroundUnit>(parameter[0]);
            if (pGroundUnit == nullptr) {
                return;
            }
            pGroundUnit->doRequestCarryallDrop(context, static_cast<int>(parameter[1]), static_cast<int>(parameter[2]));
        } break;

        case CMDTYPE::CMD_UNIT_SENDTOREPAIR: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_UNIT_SENDTOREPAIR needs 1 Parameter!");
            }
            auto* const pGroundUnit = objectManager.getObject<GroundUnit>(parameter[0]);
            if (pGroundUnit == nullptr) {
                return;
            }
            pGroundUnit->doRepair(context);
        } break;

        case CMDTYPE::CMD_UNIT_SETMODE: {
            if (parameter.size() != 2) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_UNIT_SETMODE needs 2 Parameter!");
            }
            auto* const pUnit = objectManager.getObject<UnitBase>(parameter[0]);
            if (pUnit == nullptr) {
                return;
            }
            pUnit->doSetAttackMode(context, static_cast<ATTACKMODE>(parameter[1]));
        } break;

        case CMDTYPE::CMD_DEVASTATOR_STARTDEVASTATE: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument,
                      "Command::executeCommand(): CMD_DEVASTATOR_STARTDEVASTATE needs 1 Parameter!");
            }
            auto* const pDevastator = objectManager.getObject<Devastator>(parameter[0]);
            if (pDevastator == nullptr) {
                return;
            }
            pDevastator->doStartDevastate();
        } break;

        case CMDTYPE::CMD_MCV_DEPLOY: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_MCV_DEPLOY needs 1 Parameter!");
            }
            auto* const pMCV = objectManager.getObject<MCV>(parameter[0]);
            if (pMCV == nullptr) {
                return;
            }
            pMCV->doDeploy();
        } break;

        case CMDTYPE::CMD_HARVESTER_RETURN: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_HARVESTER_RETURN needs 1 Parameter!");
            }
            auto* const pHarvester = objectManager.getObject<Harvester>(parameter[0]);
            if (pHarvester == nullptr) {
                return;
            }
            pHarvester->doReturn();
        } break;

        case CMDTYPE::CMD_STRUCTURE_SETDEPLOYPOSITION: {
            if (parameter.size() != 3) {
                THROW(std::invalid_argument,
                      "Command::executeCommand(): CMD_STRUCTURE_SETDEPLOYPOSITION needs 3 Parameters!");
            }
            auto* const pStructure = objectManager.getObject<StructureBase>(parameter[0]);
            if (pStructure == nullptr) {
                return;
            }
            pStructure->doSetDeployPosition(static_cast<int>(parameter[1]), static_cast<int>(parameter[2]));
        } break;

        case CMDTYPE::CMD_STRUCTURE_REPAIR: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_STRUCTURE_REPAIR needs 1 Parameter!");
            }
            auto* const pStructure = objectManager.getObject<StructureBase>(parameter[0]);
            if (pStructure == nullptr) {
                return;
            }
            pStructure->doRepair(context);
        } break;

        case CMDTYPE::CMD_BUILDER_UPGRADE: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_BUILDER_UPGRADE needs 1 Parameter!");
            }
            auto* const pBuilder = objectManager.getObject<BuilderBase>(parameter[0]);
            if (pBuilder == nullptr) {
                return;
            }
            pBuilder->doUpgrade(context);
        } break;

        case CMDTYPE::CMD_BUILDER_PRODUCEITEM: {
            if (parameter.size() != 3) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_BUILDER_PRODUCEITEM needs 3 Parameter!");
            }
            auto* const pBuilder = objectManager.getObject<BuilderBase>(parameter[0]);
            if (pBuilder == nullptr) {
                return;
            }
            pBuilder->doProduceItem(static_cast<ItemID_enum>(parameter[1]), static_cast<bool>(parameter[2]));
        } break;

        case CMDTYPE::CMD_BUILDER_CANCELITEM: {
            if (parameter.size() != 3) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_BUILDER_CANCELITEM needs 3 Parameter!");
            }
            auto* const pBuilder = objectManager.getObject<BuilderBase>(parameter[0]);
            if (pBuilder == nullptr) {
                return;
            }
            pBuilder->doCancelItem(static_cast<ItemID_enum>(parameter[1]), static_cast<bool>(parameter[2]));
        } break;

        case CMDTYPE::CMD_BUILDER_SETONHOLD: {
            if (parameter.size() != 2) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_BUILDER_SETONHOLD needs 2 Parameters!");
            }
            auto* const pBuilder = objectManager.getObject<BuilderBase>(parameter[0]);
            if (pBuilder == nullptr) {
                return;
            }
            pBuilder->doSetOnHold(static_cast<bool>(parameter[1]));
        } break;

        case CMDTYPE::CMD_PALACE_SPECIALWEAPON: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_PALACE_SPECIALWEAPON needs 1 Parameter!");
            }
            auto* const palace = objectManager.getObject<Palace>(parameter[0]);
            if (palace == nullptr) {
                return;
            }
            palace->doSpecialWeapon(context);
        } break;

        case CMDTYPE::CMD_PALACE_DEATHHAND: {
            if (parameter.size() != 3) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_PALACE_DEATHHAND needs 3 Parameter!");
            }
            auto* const palace = objectManager.getObject<Palace>(parameter[0]);
            if (palace == nullptr) {
                return;
            }
            palace->doLaunchDeathhand(context, static_cast<int>(parameter[1]), static_cast<int>(parameter[2]));
        } break;

        case CMDTYPE::CMD_STARPORT_PLACEORDER: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_STARPORT_PLACEORDER needs 1 Parameter!");
            }
            auto* const pStarport = objectManager.getObject<StarPort>(parameter[0]);
            if (pStarport == nullptr) {
                return;
            }
            pStarport->doPlaceOrder();
        } break;

        case CMDTYPE::CMD_STARPORT_CANCELORDER: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_STARPORT_CANCELORDER needs 1 Parameter!");
            }
            auto* const pStarport = objectManager.getObject<StarPort>(parameter[0]);
            if (pStarport == nullptr) {
                return;
            }
            pStarport->doCancelOrder();
        } break;

        case CMDTYPE::CMD_TURRET_ATTACKOBJECT: {
            if (parameter.size() != 2) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_TURRET_ATTACKOBJECT needs 2 Parameters!");
            }
            auto* const pTurret = objectManager.getObject<TurretBase>(parameter[0]);
            if (pTurret == nullptr) {
                return;
            }
            pTurret->doAttackObject(context, static_cast<int>(parameter[1]));
        } break;

        case CMDTYPE::CMD_TEST_SYNC: {
            if (parameter.size() != 1) {
                THROW(std::invalid_argument, "Command::executeCommand(): CMD_TEST_SYNC needs 1 Parameters!");
            }

            const auto currentSeed = game.randomGen.getState();
            if (currentSeed[0] != parameter[0]) {
                sdl2::log_info("Warning: Game is asynchronous in game cycle %d! Saved seed and current seed do not "
                               "match: %ud != %ud",
                               game.getGameCycleCount(), parameter[0], currentSeed[0]);
#ifdef TEST_SYNC
                context.game.saveGame("test.sav");
                exit(0);
#endif
            }
        } break;

        default: {
            THROW(std::invalid_argument, "Command::executeCommand(): Unknown CommandID!");
        }
    }
}
