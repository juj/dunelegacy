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

#include <FileClasses/Icnfile.h>

#include <FileClasses/Palette.h>

#include <misc/exceptions.h>

#include "globals.h"
#include <Definitions.h>

#include <SDL2/SDL_endian.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace {
inline constexpr auto SIZE_X = 16;
inline constexpr auto SIZE_Y = 16;
} // namespace

/// Constructor
/**
    The constructor reads from icnRWop and mapRWop all data and saves them internally. The SDL_RWops can be readonly but
    must support seeking.
    \param  icnRWop SDL_RWops to the icn-File. (can be readonly)
    \param  mapRWop SDL_RWops to the map-File. (can be readonly)
*/
Icnfile::Icnfile(SDL_RWops* icnRWop, SDL_RWops* mapRWop) {
    if (icnRWop == nullptr) {
        THROW(std::invalid_argument, "Icnfile::Icnfile(): icnRWop == nullptr!");
    }
    if (mapRWop == nullptr) {
        THROW(std::invalid_argument, "Icnfile::Icnfile(): mapRWop == nullptr!");
    }

    std::unique_ptr<uint8_t[]> pMapFiledata;

    const int64_t icnEndOffset = SDL_RWsize(icnRWop);
    if (icnEndOffset <= 0) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Cannot determine size of this *.icn-File!");
    }

    const auto icnFilesize = static_cast<size_t>(icnEndOffset);
    pIcnFiledata           = std::make_unique<uint8_t[]>(icnFilesize);

    if (SDL_RWread(icnRWop, &pIcnFiledata[0], icnFilesize, 1) != 1) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Reading this *.icn-File failed!");
    }

    const int64_t mapEndOffset = SDL_RWsize(mapRWop);
    if (mapEndOffset <= 0) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Cannot determine size of this *.map-File!");
    }

    const auto mapFilesize = static_cast<size_t>(mapEndOffset);
    pMapFiledata           = std::make_unique<uint8_t[]>(mapFilesize);

    if (SDL_RWread(mapRWop, &pMapFiledata[0], mapFilesize, 1) != 1) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Reading this *.map-File failed!");
    }

    // now we can start creating the Tilesetindex
    if (mapFilesize < 2) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): This *.map-File is too short!");
    }

    const Uint16 numTilesets = SDL_SwapLE16(*reinterpret_cast<Uint16*>(pMapFiledata.get()));

    if (mapFilesize < static_cast<size_t>(numTilesets * 2)) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): This *.map-File is too short!");
    }

    // calculate size for all entries
    Uint16 index = SDL_SwapLE16(reinterpret_cast<Uint16*>(pMapFiledata.get())[0]);
    for (int i = 1; i < numTilesets; i++) {
        const Uint16 tmp = SDL_SwapLE16(reinterpret_cast<Uint16*>(pMapFiledata.get())[i]);
        MapfileEntry newMapfileEntry;
        newMapfileEntry.numTiles = tmp - index;
        tilesets.push_back(newMapfileEntry);
        index = tmp;
    }
    MapfileEntry newMapfileEntry;
    newMapfileEntry.numTiles = mapFilesize / 2 - index;
    tilesets.push_back(newMapfileEntry);

    for (int i = 0; i < numTilesets; i++) {
        index = SDL_SwapLE16(reinterpret_cast<Uint16*>(pMapFiledata.get())[i]);

        if (static_cast<unsigned int>(mapFilesize) < (index + tilesets[i].numTiles) * 2) {
            THROW(std::runtime_error, "Icnfile::Icnfile(): This *.map-File is too short!");
        }

        // now we can read in
        for (unsigned int j = 0; j < tilesets[i].numTiles; j++) {
            tilesets[i].tileIndices.push_back(SDL_SwapLE16(reinterpret_cast<Uint16*>(pMapFiledata.get())[index + j]));
        }
    }

    pMapFiledata.reset();
    // reading MAP-File is now finished

    // check if we can access first section in ICN-File
    if (icnFilesize < 0x20) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: No SSET-Section found!\n");
    }

    SSET = pIcnFiledata.get() + 0x18;

    // check SSET-Section
    if (SSET[0] != 'S' || SSET[1] != 'S' || SSET[2] != 'E' || SSET[3] != 'T') {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: No SSET-Section found!\n");
    }

    SSET_Length = SDL_SwapBE32(*reinterpret_cast<const Uint32*>(SSET + 4)) - 8;
    SSET += 16;

    if (pIcnFiledata.get() + icnFilesize < SSET + SSET_Length) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: SSET-Section is bigger than ICN-File!\n");
    }

    RPAL = SSET + SSET_Length;

    // check RPAL-Section
    if (RPAL[0] != 'R' || RPAL[1] != 'P' || RPAL[2] != 'A' || RPAL[3] != 'L') {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: No RPAL-Section found!\n");
    }

    RPAL_Length = SDL_SwapBE32(*reinterpret_cast<const Uint32*>(RPAL + 4));
    RPAL += 8;

    if (pIcnFiledata.get() + icnFilesize < RPAL + RPAL_Length) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: RPAL-Section is bigger than ICN-File!\n");
    }

    RTBL = RPAL + RPAL_Length;

    // check RTBL-Section
    if (RTBL[0] != 'R' || RTBL[1] != 'T' || RTBL[2] != 'B' || RTBL[3] != 'L') {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: No RTBL-Section found!\n");
    }

    RTBL_Length = SDL_SwapBE32(*reinterpret_cast<const Uint32*>(RTBL + 4));
    RTBL += 8;

    if (pIcnFiledata.get() + icnFilesize < RTBL + RTBL_Length) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: RTBL-Section is bigger than ICN-File!\n");
    }

    numFiles = SSET_Length / (SIZE_X * SIZE_Y / 2);

    if (RTBL_Length < numFiles) {
        THROW(std::runtime_error, "Icnfile::Icnfile(): Invalid ICN-File: RTBL-Section is too small!\n");
    }
}

/// Destructor
/**
    Frees all memory.
*/
Icnfile::~Icnfile() = default;

/// Returns one tile in the icn-File
/**
    This method returns a SDL_Surface containing the nth tile/picture in the icn-File.
    \param  indexOfFile specifies which tile/picture to return (zero based)
    \return nth tile/picture in this icn-File
*/
sdl2::surface_ptr Icnfile::getPicture(uint32_t indexOfFile) const {
    if (indexOfFile >= numFiles) {
        THROW(std::invalid_argument,
              "Icnfile::getPicture(): Specified index (%ud) is not valid for a icn file with %ud tiles!", indexOfFile,
              numFiles);
    }

    // check if palette is in range
    if (RTBL[indexOfFile] >= RPAL_Length / 16) {
        THROW(std::runtime_error, "Icnfile::getPicture(): Invalid palette!");
    }

    const uint8_t* const RESTRICT palettestart = RPAL + static_cast<ptrdiff_t>(16) * RTBL[indexOfFile];
    const uint8_t* const RESTRICT filestart    = SSET + indexOfFile * (static_cast<size_t>(SIZE_X) * SIZE_Y / 2);

    // create new picture surface
    sdl2::surface_ptr pic{SDL_CreateRGBSurface(0, SIZE_X, SIZE_Y, 8, 0, 0, 0, 0)};
    if (pic == nullptr) {
        THROW(std::runtime_error, "Icnfile::getPicture(): Cannot create surface!");
    }

    dune::globals::palette.applyToSurface(pic.get());
    sdl2::surface_lock lock{pic.get()};

    // Now we can copy to surface
    auto* RESTRICT dest = static_cast<unsigned char*>(pic->pixels);
    for (int y = 0; y < SIZE_Y; ++y) {
        for (int x = 0; x < SIZE_X; x += 2) {
            unsigned char pixel = filestart[(y * SIZE_X + x) / 2];
            pixel               = pixel >> 4;
            dest[x]             = palettestart[pixel];

            pixel       = filestart[(y * SIZE_X + x) / 2];
            pixel       = pixel & 0x0F;
            dest[x + 1] = palettestart[pixel];
        }
        dest += pic->pitch;
    }

    return pic;
}

/// Returns an array of pictures in the icn-File
/**
    This method returns a SDL_Surface containing multiple tiles/pictures. Which tiles to include is specified by
   MapfileIndex. The MapfileIndex specifies the tileset. One tileset constists of multiple tiles of the icn-File. The
   last 3 parameters specify how to arrange the tiles:
     - If all 3 parameters are 0 then a "random" layout is choosen, which should look good.
     - If tilesX and tilesY is set to non-zero values then the result surface contains tilesX*tilesY tiles and this
   tilesN-times side by side.
     - If all there parameters are non-zero then the result surface is exactly in this arrangement.

    tilesX*tilesY*tilesN must always the number of tiles in this tileset.<br><br>
    Example:
    \code
    Tileset = 10,11,12,13,14,15,16,17,18,19,20,21
    tilesX = 2; tilesY = 2; tilesN = 3

    returned picture:
     10 11 14 15 18 19
     12 13 16 17 20 21
    \endcode
    <br>
    \param  mapfileIndex    specifies which tileset to use (zero based)
    \param  tilesX          how many tiles in x direction
    \param  tilesY          how many tiles in y direction
    \param  tilesN          how many tilesX*tilesY blocks in a row
    \return the result surface with tilesX*tilesY*tilesN tiles
*/
sdl2::surface_ptr Icnfile::getPictureArray(uint32_t mapfileIndex, int tilesX, int tilesY, int tilesN) {
    if (mapfileIndex >= tilesets.size()) {
        THROW(
            std::invalid_argument,
            "Icnfile::getPictureArray(): Specified map file index (%ud) is not valid for a map file with %ud tilesets!",
            mapfileIndex, tilesets.size());
    }

    if (tilesX == 0 && tilesY == 0 && tilesN == 0) {
        // guest what is best
        const auto tmp = tilesets[mapfileIndex].numTiles;
        if (tmp == 24) {
            // special case (radar station and light factory)
            tilesX = 2;
            tilesY = 2;
            tilesN = 6;
        } else if (tmp % 9 == 0) {
            tilesX = 3;
            tilesY = 3;
            tilesN = tmp / 9;
        } else if (tmp % 6 == 0) {
            tilesX = 3;
            tilesY = 2;
            tilesN = tmp / 6;
        } else if (tmp % 4 == 0) {
            tilesX = 2;
            tilesY = 2;
            tilesN = tmp / 4;
        } else if (tmp >= 40 && tmp % 5 == 0) {
            tilesX = tmp / 5;
            tilesY = 5;
            tilesN = 1;
        } else {
            tilesX = 1;
            tilesY = 1;
            tilesN = tmp;
        }

    } else if ((tilesX == 0 || tilesY == 0) && tilesN == 0) {
        THROW(std::invalid_argument, "Icnfile::getPictureArray(): Cannot read picture array with dimension 0!");
    } else if (tilesX == 0 && tilesY == 0 && tilesN != 0) {
        if (tilesets[mapfileIndex].numTiles % tilesN == 0) {
            // guest what is best
            const int tmp = tilesets[mapfileIndex].numTiles / tilesN;
            if (tmp % 3 == 0) {
                tilesX = tmp / 3;
                tilesY = 3;
            } else if (tmp % 2 == 0) {
                tilesX = tmp / 2;
                tilesY = 2;
            } else {
                tilesX = tmp;
                tilesY = 1;
            }
        } else {
            THROW(std::invalid_argument,
                  "Icnfile::getPictureArray(): Number of tiles for index %ud are %ud which are no multiple of the "
                  "requested number of blocks (%d)!",
                  mapfileIndex, tilesets[mapfileIndex].numTiles, tilesN);
        }
    } else {
        if (static_cast<unsigned int>(tilesX * tilesY * tilesN) != tilesets[mapfileIndex].numTiles) {
            THROW(std::invalid_argument,
                  "Icnfile::getPictureArray(): Number of tiles for index %ud are %ud which is not equal to the number "
                  "of requested tiles (%d*%d*%d)!",
                  mapfileIndex, tilesets[mapfileIndex].numTiles, tilesX, tilesY, tilesN);
        }
    }

    // create new picture surface
    sdl2::surface_ptr pic{SDL_CreateRGBSurface(0, SIZE_X * tilesX * tilesN, SIZE_Y * tilesY, 8, 0, 0, 0, 0)};
    if (pic == nullptr) {
        THROW(std::runtime_error, "Icnfile::getPictureArray(): Cannot create surface!");
    }

    dune::globals::palette.applyToSurface(pic.get());
    sdl2::surface_lock lock{pic.get()};

    int tileidx = 0;
    for (int n = 0; n < tilesN; n++) {
        for (int tile_y = 0; tile_y < tilesY; ++tile_y) {
            for (int tile_x = 0; tile_x < tilesX; ++tile_x) {
                const int IndexOfFile = tilesets[mapfileIndex].tileIndices[tileidx];

                // check if palette is in range
                if (RTBL[IndexOfFile] >= RPAL_Length / 16) {
                    THROW(std::runtime_error, "Icnfile::getPictureArray(): Invalid palette!");
                }

                const uint8_t* const RESTRICT palettestart = RPAL + static_cast<ptrdiff_t>(16) * RTBL[IndexOfFile];
                const uint8_t* const RESTRICT filestart =
                    SSET + IndexOfFile * (static_cast<ptrdiff_t>(SIZE_X) * SIZE_Y / 2);

                // Now we can copy to surface
                unsigned char* RESTRICT dest = static_cast<unsigned char*>(pic->pixels)
                                             + static_cast<ptrdiff_t>(pic->pitch) * tile_y * SIZE_Y
                                             + (tile_x + static_cast<ptrdiff_t>(n) * tilesX) * SIZE_X;
                for (int y = 0; y < SIZE_Y; y++) {
                    for (int x = 0; x < SIZE_X; x += 2) {
                        const unsigned char pixel = filestart[(y * SIZE_X + x) / 2];
                        dest[x]                   = palettestart[pixel >> 4];
                        dest[x + 1]               = palettestart[pixel & 0x0F];
                    }
                    dest += pic->pitch;
                }

                tileidx++;
            }
        }
    }

    return pic;
}

/// Returns a row of pictures in the icn-File
/**
    This method returns a SDL_Surface containing multiple tiles/pictures. The returned surface contains all
    tiles from startIndex to endIndex.
    \param  startIndex      The first tile to use
    \param  endIndex        The last tile to use
    \param  maxRowLength    Used to limit the number of tiles per row and put the remaining tiles on the following rows
   (0 equals no row limitation) \return the result surface with (endIndex-startIndex+1) tiles.
*/
sdl2::surface_ptr Icnfile::getPictureRow(uint32_t startIndex, uint32_t endIndex, uint32_t maxRowLength) const {

    if (startIndex >= numFiles || endIndex >= numFiles || startIndex > endIndex) {
        THROW(std::invalid_argument,
              "Icnfile::getPictureRow(): Invalid start index (%ud) or end index (%ud) for an icn file with %ud tiles!",
              startIndex, endIndex, numFiles);
    }

    const auto numTiles = endIndex - startIndex + 1;
    const auto numCols  = maxRowLength == 0 ? numTiles : maxRowLength;
    const auto numRows  = (numTiles + numCols - 1) / numCols;

    // create new picture surface
    sdl2::surface_ptr pic{SDL_CreateRGBSurface(0, SIZE_X * numCols, SIZE_Y * numRows, 8, 0, 0, 0, 0)};
    if (pic == nullptr) {
        THROW(std::runtime_error, "Icnfile::getPictureRow(): Cannot create surface!");
    }

    dune::globals::palette.applyToSurface(pic.get());
    sdl2::surface_lock lock{pic.get()};

    uint32_t tileCount = 0u;
    for (uint32_t row = 0u; row < numRows && tileCount < numTiles; ++row) {
        for (uint32_t col = 0u; col < numCols && tileCount < numTiles; ++col) {
            const uint32_t indexOfFile = startIndex + tileCount;

            // check if palette is in range
            if (RTBL[indexOfFile] >= RPAL_Length / 16) {
                THROW(std::runtime_error, "Icnfile::getPictureRow(): Invalid palette!");
            }

            const uint8_t* const RESTRICT palettestart = RPAL + static_cast<ptrdiff_t>(16) * RTBL[indexOfFile];
            const uint8_t* const RESTRICT filestart = SSET + indexOfFile * (static_cast<size_t>(SIZE_X) * SIZE_Y / 2);

            // Now we can copy to surface
            unsigned char* RESTRICT dest = static_cast<unsigned char*>(pic->pixels)
                                         + row * static_cast<ptrdiff_t>(SIZE_Y) * pic->pitch
                                         + static_cast<size_t>(col) * SIZE_X;
            for (int y = 0; y < SIZE_Y; ++y) {
                for (int x = 0; x < SIZE_X; x += 2) {
                    const unsigned char pixel = filestart[(y * static_cast<ptrdiff_t>(SIZE_X) + x) / 2];

                    dest[x]     = palettestart[pixel >> 4];
                    dest[x + 1] = palettestart[pixel & 0x0F];
                }
                dest += pic->pitch;
            }

            tileCount++;
        }
    }

    return pic;
}

/// Returns a row of pictures in the icn-File
/**
    This method returns a SDL_Surface containing multiple tiles/pictures. The returned surface contains all
    tiles specified be the parameters.
    \param  indexes    the indexes of the tiles to be extracted.
    \return the result surface with all specified tiles in a row.
*/
sdl2::surface_ptr Icnfile::getPictureRow2(std::initializer_list<uint32_t> indexes) const {

    const auto numTiles = indexes.size();

    // create new picture surface
    sdl2::surface_ptr pic{SDL_CreateRGBSurface(0, static_cast<int>(SIZE_X * numTiles), SIZE_Y, 8, 0, 0, 0, 0)};
    if (pic == nullptr) {
        THROW(std::runtime_error, "Icnfile::getPictureRow2(): Cannot create surface!");
    }

    dune::globals::palette.applyToSurface(pic.get());
    sdl2::surface_lock lock{pic.get()};

    auto i = size_t{0};
    for (auto indexOfFile : indexes) {

        if (indexOfFile >= numFiles) {
            THROW(std::invalid_argument,
                  "Icnfile::getPictureRow2(): Specified index (%ud) is not valid for an icn file with %ud tiles!",
                  indexOfFile, numFiles);
        }

        // check if palette is in range
        if (RTBL[indexOfFile] >= RPAL_Length / 16) {
            THROW(std::runtime_error, "Icnfile::getPictureRow2(): Invalid palette!");
        }

        const uint8_t* const RESTRICT palettestart = RPAL + static_cast<ptrdiff_t>(16) * RTBL[indexOfFile];
        const uint8_t* const RESTRICT filestart    = SSET + indexOfFile * (static_cast<size_t>(SIZE_X) * SIZE_Y / 2);

        // Now we can copy to surface
        unsigned char* RESTRICT dest = static_cast<unsigned char*>(pic->pixels) + i * SIZE_X;
        for (int y = 0; y < SIZE_Y; y++) {
            for (int x = 0; x < SIZE_X; x += 2) {
                const unsigned char pixel = filestart[(y * static_cast<ptrdiff_t>(SIZE_X) + x) / 2];
                dest[x]                   = palettestart[pixel >> 4];
                dest[x + 1]               = palettestart[pixel & 0x0F];
            }
            dest += pic->pitch;
        }

        ++i;
    }

    return pic;
}
