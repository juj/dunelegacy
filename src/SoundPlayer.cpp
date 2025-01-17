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

#include <SoundPlayer.h>

#include <globals.h>

#include <Game.h>
#include <House.h>
#include <Map.h>
#include <ScreenBorder.h>

#include <misc/exceptions.h>

SoundPlayer::SoundPlayer() : sfxVolume(dune::globals::settings.audio.sfxVolume) {

    if (!dune::globals::pSFXManager) {
        soundOn = false;
        return;
    }

    Mix_Volume(-1, sfxVolume);

    Mix_ReserveChannels(24); // Reserve a channel for voice over

    // clang-format off
    Mix_GroupChannels( 0,  1, static_cast<int>(ChannelGroup::Voice));
    Mix_GroupChannels( 2,  3, static_cast<int>(ChannelGroup::UI));
    Mix_GroupChannels( 4,  5, static_cast<int>(ChannelGroup::Credits));
    Mix_GroupChannels( 6,  8, static_cast<int>(ChannelGroup::Explosion));
    Mix_GroupChannels( 9, 10, static_cast<int>(ChannelGroup::ExplosionStructure));
    Mix_GroupChannels(11, 13, static_cast<int>(ChannelGroup::Gun));
    Mix_GroupChannels(14, 16, static_cast<int>(ChannelGroup::Rocket));
    Mix_GroupChannels(17, 18, static_cast<int>(ChannelGroup::Scream));
    Mix_GroupChannels(19, 21, static_cast<int>(ChannelGroup::Sonic));
    Mix_GroupChannels(22, 23, static_cast<int>(ChannelGroup::Other));
    // clang-format on

    soundOn = dune::globals::settings.audio.playSFX;
}

SoundPlayer::~SoundPlayer() = default;

void SoundPlayer::playVoice(Voice_enum id, HOUSETYPE houseID) const {
    if (!soundOn || !dune::globals::pSFXManager)
        return;

    const auto voice_index = static_cast<int>(id);
    if (voice_index < 0 || voice_index >= static_cast<int>(Voice_enum::NUM_VOICE))
        THROW(std::invalid_argument, "The voice ID %d is invalid!", voice_index);

    Mix_Chunk* tmp = nullptr;

    if ((tmp = dune::globals::pSFXManager->getVoice(id, houseID)) == nullptr) {
        THROW(std::invalid_argument, "There is no voice with ID %d!", voice_index);
    }

    const auto channel = Mix_PlayChannel(Mix_GroupAvailable(static_cast<int>(ChannelGroup::Voice)), tmp, 0);
    if (channel != -1) {
        Mix_Volume(channel, sfxVolume);
    }
}

void SoundPlayer::playSoundAt(Sound_enum soundID, const Coord& location) const {
    if (!soundOn)
        return;

    auto* const currentGameMap = dune::globals::currentGameMap;

    if (!currentGameMap->tileExists(location)
        || !currentGameMap->getTile(location)->isExploredByTeam(dune::globals::currentGame.get(),
                                                                dune::globals::pLocalHouse->getTeamID())) {
        return;
    }

    const auto realCoord = location * TILESIZE + Coord(TILESIZE / 2, TILESIZE / 2);

    const auto* const screenborder = dune::globals::screenborder.get();

    if (screenborder->isInsideScreen(realCoord, Coord(TILESIZE, TILESIZE))) {
        playSound(soundID, sfxVolume);
    } else if (screenborder->isInsideScreen(realCoord, Coord(TILESIZE * 16, TILESIZE * 16))) {
        playSound(soundID, (sfxVolume * 3) / 4);
    } else if (screenborder->isInsideScreen(realCoord, Coord(TILESIZE * 24, TILESIZE * 24))) {
        playSound(soundID, sfxVolume / 2);
    } else {
        playSound(soundID, sfxVolume / 4);
    }
}

void SoundPlayer::toggleSound() noexcept {
    soundOn = !soundOn && dune::globals::pSFXManager;
}

void SoundPlayer::setSound(bool value) noexcept {
    soundOn = value && dune::globals::pSFXManager;
}

void SoundPlayer::playSound(Mix_Chunk* sound) const {
    if (!soundOn)
        return;

    const auto channel = Mix_PlayChannel(-1, sound, 0);
    if (channel != -1) {
        Mix_Volume(channel, sfxVolume);
    }
}

void SoundPlayer::playSound(Sound_enum id) const {
    playSound(id, sfxVolume);
}

void SoundPlayer::playSound(Sound_enum soundID, int volume) const {
    if (!soundOn || !dune::globals::pSFXManager)
        return;

    const auto sound_index = static_cast<int>(soundID);
    if (sound_index < 0 || sound_index >= static_cast<int>(Sound_enum::NUM_SOUNDCHUNK))
        THROW(std::invalid_argument, "The sound ID %d is invalid!", sound_index);

    static constexpr ChannelGroup soundID2ChannelGroup[] = {
        ChannelGroup::UI,                 // Sound_PlaceStructure
        ChannelGroup::UI,                 // Sound_ButtonClick
        ChannelGroup::UI,                 // Sound_InvalidAction
        ChannelGroup::Credits,            // Sound_CreditsTick
        ChannelGroup::Credits,            // Sound_CreditsTickDown
        ChannelGroup::Credits,            // Sound_Tick
        ChannelGroup::UI,                 // Sound_RadarNoise
        ChannelGroup::Explosion,          // Sound_ExplosionGas
        ChannelGroup::Explosion,          // Sound_ExplosionTiny
        ChannelGroup::Explosion,          // Sound_ExplosionSmall
        ChannelGroup::Explosion,          // Sound_ExplosionMedium
        ChannelGroup::Explosion,          // Sound_ExplosionLarge
        ChannelGroup::ExplosionStructure, // Sound_ExplosionStructure
        ChannelGroup::Other,              // Sound_WormAttack
        ChannelGroup::Gun,                // Sound_Gun
        ChannelGroup::Rocket,             // Sound_Rocket
        ChannelGroup::Explosion,          // Sound_Bloom
        ChannelGroup::Scream,             // Sound_Scream1
        ChannelGroup::Scream,             // Sound_Scream2
        ChannelGroup::Scream,             // Sound_Scream3
        ChannelGroup::Scream,             // Sound_Scream4
        ChannelGroup::Scream,             // Sound_Scream5
        ChannelGroup::Scream,             // Sound_Trumpet
        ChannelGroup::Other,              // Sound_Drop
        ChannelGroup::Scream,             // Sound_Squashed
        ChannelGroup::Gun,                // Sound_MachineGun
        ChannelGroup::Sonic,              // Sound_Sonic
        ChannelGroup::Rocket,             // Sound_RocketSmall
    };

    static_assert(static_cast<size_t>(Sound_enum::NUM_SOUNDCHUNK) == std::size(soundID2ChannelGroup));

    Mix_Chunk* sound = nullptr;

    if ((sound = dune::globals::pSFXManager->getSound(soundID)) == nullptr)
        THROW(std::invalid_argument, "There is no sound with ID %d!", sound_index);

    const auto group = static_cast<int>(soundID2ChannelGroup[sound_index]);

    const auto channel = Mix_GroupAvailable(group);
    const auto actual  = Mix_PlayChannel(channel, sound, 0);
    if (actual != -1) {
        Mix_Volume(actual, volume);
    }
}
