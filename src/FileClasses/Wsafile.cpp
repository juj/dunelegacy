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

#include <FileClasses/Wsafile.h>

#include <FileClasses/Decode.h>
#include <FileClasses/Palette.h>

#include "globals.h"
#include <Definitions.h>

#include <SDL2/SDL_endian.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

/// Destructor
/**
    Frees all memory.
*/
Wsafile::~Wsafile() = default;

/// Returns a picture in this wsa-File
/**
    This method returns a SDL_Surface containing the nth frame of this animation.
    \param  frameNumber specifies which frame to return (zero based)
    \return nth frame in this animation
*/
sdl2::surface_ptr Wsafile::getPicture(uint32_t frameNumber) const {
    if (frameNumber >= numFrames) {
        THROW(std::invalid_argument,
              "Wsafile::getPicture(): Requested frame number is %ud but the file contains only %ud frames!",
              frameNumber, numFrames);
    }

    // create new picture surface
    auto pic = sdl2::surface_ptr{SDL_CreateRGBSurface(0, sizeX, sizeY, 8, 0, 0, 0, 0)};
    if (pic == nullptr) {
        THROW(std::runtime_error, "Wsafile::getPicture(): Cannot create surface!");
    }

    dune::globals::palette.applyToSurface(pic.get());

    const unsigned char* const RESTRICT pImage = &decodedFrames[static_cast<size_t>(frameNumber) * sizeX * sizeY];
    auto* const RESTRICT pixels                = static_cast<unsigned char*>(pic->pixels);

    sdl2::surface_lock lock{pic.get()};

    // Now we can copy line by line
    for (auto y = ptrdiff_t{0}; y < static_cast<ptrdiff_t>(sizeY); ++y) {
        memcpy(pixels + y * pic->pitch, pImage + y * sizeX, sizeX);
    }

    return pic;
}

/// Returns a picture-row
/**
    This method returns a SDL_Surface containing the complete animation.
    \param  numFramesX  the maximum number of frames in X direction
    \return the complete animation
*/
sdl2::surface_ptr Wsafile::getAnimationAsPictureRow(int numFramesX) const {

    numFramesX           = std::min(numFramesX, static_cast<int>(numFrames));
    const int numFramesY = (numFrames + numFramesX - 1) / numFramesX;

    // create new picture surface
    auto pic = sdl2::surface_ptr{SDL_CreateRGBSurface(0, sizeX * numFramesX, sizeY * numFramesY, 8, 0, 0, 0, 0)};
    if (pic == nullptr) {
        THROW(std::runtime_error, "Wsafile::getAnimationAsPictureRow(): Cannot create surface!");
    }

    dune::globals::palette.applyToSurface(pic.get());

    char* const RESTRICT pixels = static_cast<char*>(pic->pixels);

    sdl2::surface_lock lock{pic.get()};

    for (auto y = 0; y < numFramesY; y++) {
        for (auto x = 0; x < numFramesX; x++) {
            const auto i = y * numFramesX + x;
            if (i >= numFrames) {
                return pic;
            }

            const unsigned char* const RESTRICT pImage =
                &decodedFrames[i * static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY)];

            // Now we can copy this frame line by line
            for (auto line = ptrdiff_t{0}; line < ptrdiff_t{sizeY}; ++line) {
                memcpy(pixels + (static_cast<ptrdiff_t>(y) * sizeY + line) * pic->pitch
                           + x * static_cast<ptrdiff_t>(sizeX),
                       pImage + line * sizeX, sizeX);
            }
        }
    }

    return pic;
}

/// Returns an animation
/**
    This method returns a new animation object with all pictures from startindex to endindex
    in it.
    \param  startindex  index of the first picture
    \param  endindex    index of the last picture
    \param  bDoublePic  if true, the picture is scaled up by a factor of 2
    \param  bSetColorKey    if true, black is set as transparency
    \return a new animation object
*/
std::unique_ptr<Animation>
Wsafile::getAnimation(unsigned int startindex, unsigned int endindex, bool bDoublePic, bool bSetColorKey) const {
    auto animation = std::make_unique<Animation>();

    for (unsigned int i = startindex; i <= endindex; i++) {
        animation->addFrame(getPicture(i), bDoublePic, bSetColorKey);
    }

    return animation;
}

/// Helper method to decode one frame
/**
    This helper method decodes one frame.
    \param  pFiledata       Pointer to the data of this wsa-File
    \param  index           Array with startoffsets
    \param  numberOfFrames  Number of frames to decode
    \param  pDecodedFrames  memory to copy decoded frames to (must be x*y*NumberOfFrames bytes long)
    \param  x               x-dimension of one frame
    \param  y               y-dimension of one frame
*/
void Wsafile::decodeFrames(const unsigned char* pFiledata, uint32_t* index, int numberOfFrames,
                           unsigned char* pDecodedFrames, int x, int y) const {
    for (auto i = ptrdiff_t{0}; i < ptrdiff_t{numberOfFrames}; ++i) {
        auto dec80 = std::make_unique<unsigned char[]>(static_cast<size_t>(x) * y * 2);

        decode80(pFiledata + SDL_SwapLE32(index[i]), dec80.get(), 0);

        decode40(dec80.get(), pDecodedFrames + i * x * y);

        dec80.reset();

        if (i < numberOfFrames - 1) {
            memcpy(pDecodedFrames + (i + 1) * x * y, pDecodedFrames + i * x * y, ptrdiff_t{x} * y);
        }
    }
}

/// Helper method for reading the complete wsa-file into memory.
/**
    This method reads the complete file into memory. A pointer to this memory is returned and
    should be freed with free when no longer needed.

*/
std::unique_ptr<unsigned char[]> Wsafile::readfile(SDL_RWops* rwop, int* filesize) const {
    if (filesize == nullptr) {
        THROW(std::runtime_error, "Wsafile::readfile(): filesize == nullptr!");
    }

    if (rwop == nullptr) {
        THROW(std::runtime_error, "Wsafile::readfile(): rwop == nullptr!");
    }

    const auto endOffset = SDL_RWsize(rwop);
    if (endOffset < 0) {
        THROW(std::runtime_error, "Wsafile::readfile(): Cannot determine size of this *.wsa-File!");
    }
    const auto wsaFilesize = static_cast<size_t>(endOffset);

    if (wsaFilesize < 10) {
        THROW(std::runtime_error, "Wsafile::readfile(): No valid WSA-File: File too small!");
    }

    auto pFiledata = std::make_unique<unsigned char[]>(wsaFilesize);

    if (SDL_RWread(rwop, pFiledata.get(), wsaFilesize, 1) != 1) {
        THROW(std::runtime_error, "Wsafile::readfile(): Reading this *.wsa-File failed!");
    }

    *filesize = wsaFilesize;
    return pFiledata;
}

/// Helper method for reading and concatenating various WSA-Files.
/**
    This methods reads from the RWops all data and concatenates all the frames to one animation. The SDL_RWops
    can be readonly but must support seeking.
    \param  rwops        SDL_RWops for each wsa-File should be in this initializer list.
*/
void Wsafile::readdata(const std::initializer_list<SDL_RWops*>& rwops) {
    const auto numFiles = rwops.size();

    std::vector<std::unique_ptr<unsigned char[]>> pFiledata(numFiles);
    std::vector<uint32_t*> index(numFiles);
    std::vector<uint16_t> numberOfFrames(numFiles);
    std::vector<bool> extended(numFiles);

    numFrames = 0;
    looped    = false;

    for (auto i = size_t{0}; auto rwop : rwops) {
        int wsaFilesize   = 0;
        pFiledata[i]      = readfile(rwop, &wsaFilesize);
        numberOfFrames[i] = SDL_SwapLE16(*reinterpret_cast<Uint16*>(pFiledata[i].get()));

        if (i == 0) {
            sizeX = SDL_SwapLE16(*reinterpret_cast<Uint16*>(pFiledata[0].get() + 2));
            sizeY = SDL_SwapLE16(*reinterpret_cast<Uint16*>(pFiledata[0].get() + 4));
        } else {
            if (sizeX != SDL_SwapLE16(*reinterpret_cast<Uint16*>(pFiledata[i].get() + 2))
                || sizeY != SDL_SwapLE16(*reinterpret_cast<Uint16*>(pFiledata[i].get() + 4))) {
                THROW(std::runtime_error,
                      "Wsafile::readdata(): The wsa-files have different image dimensions. Cannot concatenate them!");
            }
        }

        if (reinterpret_cast<unsigned short*>(pFiledata[i].get())[6] == 0) {
            index[i] = reinterpret_cast<uint32_t*>(pFiledata[i].get() + 10);
        } else {
            index[i] = reinterpret_cast<uint32_t*>(pFiledata[i].get() + 8);
        }

        if (index[i][0] == 0) {
            // extended animation
            if (i == 0U) {
                sdl2::log_info("Extended WSA-File!");
            }
            index[i]++;
            numberOfFrames[i]--;
            extended[i] = true;
        } else {
            extended[i] = false;
        }

        if (i == 0U) {
            if (index[0][numberOfFrames[0] + 1] == 0) {
                // index[numberOfFrames[0]] point to end of file
                // => no loop
                looped = false;
            } else {
                // index[numberOfFrames[0]] point to loop frame
                // => looped animation
                //  sdl2::log_info("Looped WSA-File!");
                looped = true;
            }
        }

        if (pFiledata[i].get() + wsaFilesize
            < reinterpret_cast<unsigned char*>(index[i]) + sizeof(uint32_t) * numberOfFrames[i]) {
            THROW(std::runtime_error, "Wsafile::readdata(): No valid WSA-File: File too small!");
        }

        numFrames += numberOfFrames[i];
        ++i;
    }

    decodedFrames.clear();
    decodedFrames.resize(static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY) * numFrames);

    assert(decodedFrames.size() >= static_cast<size_t>(sizeX) * sizeY);
    decodeFrames(pFiledata[0].get(), index[0], numberOfFrames[0], decodedFrames.data(), sizeX, sizeY);
    pFiledata[0].reset();

    if (numFiles > 1) {
        auto* nextFreeFrame =
            &decodedFrames[(numberOfFrames[0] * static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY))];

        for (auto i = 1U; i < numFiles; ++i) {
            if (extended[i]) {
                // copy last frame
                memcpy(nextFreeFrame, nextFreeFrame - static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY),
                       static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY));
            }
            assert(nextFreeFrame + static_cast<ptrdiff_t>(sizeX) * sizeY <= &decodedFrames.back());
            decodeFrames(pFiledata[i].get(), index[i], numberOfFrames[i], nextFreeFrame, sizeX, sizeY);
            nextFreeFrame += static_cast<ptrdiff_t>(numberOfFrames[i]) * sizeX * sizeY;
            pFiledata[i].reset();
        }
    }
}
