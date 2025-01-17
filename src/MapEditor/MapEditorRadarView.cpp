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

#include <MapEditor/MapEditorRadarView.h>

#include <MapEditor/MapEditor.h>

#include <globals.h>

#include <sand.h>

#include <Definitions.h>
#include <ScreenBorder.h>

#include <misc/draw_util.h>

#include <cstddef>

MapEditorRadarView::MapEditorRadarView(MapEditor* pMapEditor) : pMapEditor(pMapEditor) {
    radarSurface =
        sdl2::surface_ptr{SDL_CreateRGBSurface(0, RADARWIDTH, RADARHEIGHT, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    SDL_FillRect(radarSurface.get(), nullptr, COLOR_BLACK);

    auto* const renderer = dune::globals::renderer.get();
    radarTexture.reset(
        SDL_CreateTexture(renderer, SCREEN_FORMAT, SDL_TEXTUREACCESS_STREAMING, RADARWIDTH, RADARHEIGHT));
}

MapEditorRadarView::~MapEditorRadarView() = default;

int MapEditorRadarView::getMapSizeX() const {
    return pMapEditor->getMap().getSizeX();
}

int MapEditorRadarView::getMapSizeY() const {
    return pMapEditor->getMap().getSizeY();
}

void MapEditorRadarView::draw(Point position) {
    const SDL_Rect radarPosition = {position.x + RADARVIEW_BORDERTHICKNESS, position.y + RADARVIEW_BORDERTHICKNESS,
                                    RADARWIDTH, RADARHEIGHT};

    const MapData& map             = pMapEditor->getMap();
    auto* const renderer           = dune::globals::renderer.get();
    const auto* const screenborder = dune::globals::screenborder.get();

    int scale   = 1;
    int offsetX = 0;
    int offsetY = 0;

    calculateScaleAndOffsets(map.getSizeX(), map.getSizeY(), scale, offsetX, offsetY);

    updateRadarSurface(map, scale, offsetX, offsetY);

    SDL_UpdateTexture(radarTexture.get(), nullptr, radarSurface->pixels, radarSurface->pitch);

    Dune_RenderCopy(renderer, radarTexture.get(), radarPosition.x, radarPosition.y);

    // draw viewport rect on radar
    SDL_Rect radarRect{};
    radarRect.x = (screenborder->getLeft() * map.getSizeX() * scale) / (map.getSizeX() * TILESIZE) + offsetX;
    radarRect.y = (screenborder->getTop() * map.getSizeY() * scale) / (map.getSizeY() * TILESIZE) + offsetY;
    radarRect.w =
        ((screenborder->getRight() - screenborder->getLeft()) * map.getSizeX() * scale) / (map.getSizeX() * TILESIZE);
    radarRect.h =
        ((screenborder->getBottom() - screenborder->getTop()) * map.getSizeY() * scale) / (map.getSizeY() * TILESIZE);

    if (radarRect.x < offsetX) {
        radarRect.w -= radarRect.x;
        radarRect.x = offsetX;
    }

    if (radarRect.y < offsetY) {
        radarRect.h -= radarRect.y;
        radarRect.y = offsetY;
    }

    const int offsetFromRightX = 128 - map.getSizeX() * scale - offsetX;
    if (radarRect.x + radarRect.w > radarPosition.w - offsetFromRightX) {
        radarRect.w = radarPosition.w - offsetFromRightX - radarRect.x - 1;
    }

    const int offsetFromBottomY = 128 - map.getSizeY() * scale - offsetY;
    if (radarRect.y + radarRect.h > radarPosition.h - offsetFromBottomY) {
        radarRect.h = radarPosition.h - offsetFromBottomY - radarRect.y - 1;
    }

    renderDrawRect(renderer, radarPosition.x + radarRect.x, radarPosition.y + radarRect.y,
                   radarPosition.x + (radarRect.x + radarRect.w), radarPosition.y + (radarRect.y + radarRect.h),
                   COLOR_WHITE);
}

void MapEditorRadarView::updateRadarSurface(const MapData& map, int scale, int offsetX, int offsetY) {

    SDL_FillRect(radarSurface.get(), nullptr, COLOR_BLACK);

    sdl2::surface_lock lock{radarSurface.get()};

    for (int y = 0; y < map.getSizeY(); y++) {
        for (int x = 0; x < map.getSizeX(); x++) {

            uint32_t color = getColorByTerrainType(map(x, y));

            if (map(x, y) == Terrain_Sand) {
                std::vector<Coord>& spiceFields = pMapEditor->getSpiceFields();

                for (auto& spiceField : spiceFields) {
                    if (spiceField.x == x && spiceField.y == y) {
                        color = COLOR_THICKSPICE;
                        break;
                    }
                    if (distanceFrom(spiceField, Coord(x, y)) <= 5) {
                        color = COLOR_SPICE;
                        break;
                    }
                }
            }

            // check for classic map items (spice blooms, special blooms)
            std::vector<Coord>& spiceBlooms = pMapEditor->getSpiceBlooms();
            for (const auto& spiceBloom : spiceBlooms) {
                if (spiceBloom.x == x && spiceBloom.y == y) {
                    color = COLOR_BLOOM;
                    break;
                }
            }

            std::vector<Coord>& specialBlooms = pMapEditor->getSpecialBlooms();
            for (const auto& specialBloom : specialBlooms) {
                if (specialBloom.x == x && specialBloom.y == y) {
                    color = COLOR_BLOOM;
                    break;
                }
            }

            color = MapRGBA(radarSurface->format, color);

            auto* radar_pixels = static_cast<uint8_t*>(radarSurface->pixels);
            const auto pitch   = radarSurface->pitch;

            for (int j = 0; j < scale; j++) {
                auto* RESTRICT p = reinterpret_cast<uint32_t*>(
                                       radar_pixels + (offsetY + static_cast<ptrdiff_t>(scale) * y + j) * pitch)
                                 + (offsetX + static_cast<ptrdiff_t>(scale) * x);

                for (int i = 0; i < scale; i++, p++) {
                    // Do not use putPixel here to avoid overhead
                    *p = color;
                }
            }
        }
    }

    for (const auto& unit : pMapEditor->getUnitList()) {
        const auto house_id = static_cast<int>(unit.house_);
        const auto color    = SDL2RGB(dune::globals::palette[dune::globals::houseToPaletteIndex[house_id]]);

        if (unit.position_.x >= 0 && unit.position_.x < map.getSizeX() && unit.position_.y >= 0
            && unit.position_.y < map.getSizeY()) {

            const auto x0 = offsetX + scale * unit.position_.x;
            const auto y0 = offsetY + scale * unit.position_.y;

            for (int i = 0; i < scale; i++) {
                for (int j = 0; j < scale; j++) {
                    putPixel(radarSurface.get(), x0 + i, y0 + j, color);
                }
            }
        }
    }

    for (const auto& structure : pMapEditor->getStructureList()) {
        const auto structureSize = getStructureSize(structure.itemID_);
        const auto house_id      = static_cast<int>(structure.house_);
        const auto color         = SDL2RGB(dune::globals ::palette[dune::globals::houseToPaletteIndex[house_id]]);

        for (int y = 0; y < structureSize.y; y++) {
            const auto y0 = offsetY + scale * (structure.position_.y + y);

            for (int x = 0; x < structureSize.x; x++) {

                if (x >= 0 && x < map.getSizeX() && y >= 0 && y < map.getSizeY()) {
                    const auto x0 = offsetX + scale * (structure.position_.x + x);

                    for (int i = 0; i < scale; i++) {
                        for (int j = 0; j < scale; j++) {
                            putPixel(radarSurface.get(), x0 + i, y0 + j, color);
                        }
                    }
                }
            }
        }
    }
}
