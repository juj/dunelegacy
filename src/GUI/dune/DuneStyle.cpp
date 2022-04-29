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

#include <GUI/Widget.h>
#include <GUI/dune/DuneStyle.h>
#include <misc/draw_util.h>

#include "Renderer/DuneSurface.h"
#include <FileClasses/Font.h>
#include <FileClasses/FontManager.h>
#include <FileClasses/GFXManager.h>

#include <globals.h>

#include <SDL2/SDL_render.h>

#include <algorithm>
#include <string_view>

DuneSurfaceOwned DuneStyle::createSurfaceWithText(std::string_view text, uint32_t color, unsigned int fontsize) const {
    auto surface = fontManager_->getFont(scaledFontSize(fontsize))->createTextSurface(text, color);

    const auto scale = 1.f / getActualScale();

    const auto w = static_cast<float>(surface->w) * scale;
    const auto h = static_cast<float>(surface->h) * scale;

    return DuneSurfaceOwned{std::move(surface), w, h};
}

DuneSurfaceOwned DuneStyle::createSurfaceWithMultilineText(std::string_view text, uint32_t color, unsigned int fontsize,
                                                           bool bCentered) const {
    auto surface = fontManager_->getFont(scaledFontSize(fontsize))->createMultilineTextSurface(text, color, bCentered);

    const auto scale = 1.f / getActualScale();

    const auto w = static_cast<float>(surface->w) * scale;
    const auto h = static_cast<float>(surface->h) * scale;

    return DuneSurfaceOwned{std::move(surface), w, h};
}

uint32_t DuneStyle::scaledFontSize(uint32_t font_size) const {
    const auto size = static_cast<uint32_t>(std::round(getActualScale() * static_cast<float>(font_size)));

    return size;
}

int DuneStyle::getPhysicalTextHeight(unsigned FontNum) const {
    return fontManager_->getFont(scaledFontSize(FontNum))->getTextHeight();
}

float DuneStyle::getTextHeight(unsigned int FontNum) const {
    const auto height = getPhysicalTextHeight(FontNum);

    return static_cast<float>(height) / getActualScale();
}

float DuneStyle::getTextWidth(std::string_view text, unsigned int FontNum) const {
    const auto width = fontManager_->getFont(scaledFontSize(FontNum))->getTextWidth(text);

    return static_cast<float>(width) / getActualScale();
}

DuneTextureOwned
DuneStyle::createText(SDL_Renderer* renderer, std::string_view text, uint32_t color, unsigned fontSize) const {
    const auto surface = createSurfaceWithText(text, color, fontSize);

    return surface.createTexture(renderer);
}

DuneTextureOwned DuneStyle::createMultilineText(SDL_Renderer* renderer, std::string_view text, uint32_t color,
                                                unsigned fontSize, bool bCentered) const {
    const auto surface = createSurfaceWithMultilineText(text, color, fontSize, bCentered);

    return surface.createTexture(renderer);
}

DuneStyle::DuneStyle(FontManager* fontManager) : fontManager_{fontManager} {
    assert(fontManager_);
}

DuneStyle::~DuneStyle() = default;

Point DuneStyle::getMinimumLabelSize(std::string_view text, int fontSize) const {
    return {static_cast<int>(getTextWidth(text, fontSize)) + 12, static_cast<int>(getTextHeight(fontSize)) + 4};
}

sdl2::surface_ptr
DuneStyle::createLabelSurface(uint32_t width, uint32_t height, const std::vector<std::string>& textLines, int fontSize,
                              Alignment_Enum alignment, uint32_t textcolor, uint32_t textshadowcolor,
                              uint32_t backgroundcolor) const {

    // create surfaces
    sdl2::surface_ptr surface{SDL_CreateRGBSurface(0, width, height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (surface == nullptr)
        return nullptr;

    SDL_FillRect(surface.get(), nullptr, backgroundcolor);

    if (textcolor == COLOR_DEFAULT)
        textcolor = defaultForegroundColor;
    if (textshadowcolor == COLOR_DEFAULT)
        textshadowcolor = defaultShadowColor;

    const auto scale = getActualScale();

    const auto fontheight = getPhysicalTextHeight(fontSize);
    const auto spacing    = static_cast<int>(std::round(2 * scale));

    auto textpos_y = 0;

    const auto line_count = static_cast<int>(textLines.size());

    if (alignment & Alignment_VCenter) {
        const auto textheight = fontheight * line_count + spacing * (line_count - 1);

        textpos_y = (static_cast<int>(height) - textheight) / 2;
    } else if (alignment & Alignment_Bottom) {
        const auto textheight = fontheight * line_count + spacing * (line_count - 1);

        textpos_y = static_cast<int>(height) - textheight - spacing;
    } else {
        // Alignment_Top
        textpos_y = spacing;
    }

    for (const auto& textLine : textLines) {
        if (!textLine.empty()) {
            auto textSurface1 = createSurfaceWithText(textLine, textshadowcolor, fontSize);
            auto textSurface2 = createSurfaceWithText(textLine, textcolor, fontSize);

            if (textSurface1 && textSurface2) {
                SDL_SetSurfaceBlendMode(textSurface1.get(), SDL_BLENDMODE_NONE);
                SDL_SetSurfaceBlendMode(textSurface2.get(), SDL_BLENDMODE_BLEND);

                auto textRect1 = calcDrawingRect(textSurface1.get(), 0, textpos_y + 4);
                auto textRect2 = calcDrawingRect(textSurface2.get(), 0, textpos_y + 3);

                if (alignment & Alignment_Left) {
                    textRect1.x = 4;
                    textRect2.x = 3;
                } else if (alignment & Alignment_Right) {
                    textRect1.x = static_cast<int>(width) - textSurface1->w - 4;
                    textRect2.x = static_cast<int>(width) - textSurface2->w - 3;
                } else {
                    // Alignment_HCenter
                    textRect1.x = ((surface->w - textSurface1->w) / 2) + 3;
                    textRect2.x = ((surface->w - textSurface2->w) / 2) + 2;
                }

                SDL_BlitSurface(textSurface1.get(), nullptr, surface.get(), &textRect1);

                SDL_BlitSurface(textSurface2.get(), nullptr, surface.get(), &textRect2);
            }
        }

        textpos_y += fontheight + spacing;
    }

    return surface;
}

DuneTextureOwned
DuneStyle::createLabel(SDL_Renderer* renderer, uint32_t width, uint32_t height,
                       const std::vector<std::string>& textLines, int fontSize, Alignment_Enum alignment,
                       Uint32 textcolor, Uint32 textshadowcolor, Uint32 backgroundcolor) const {

    const auto [scaled_width, scaled_height] = getPhysicalSize(width, height);

    const auto surface = createLabelSurface(scaled_width, scaled_height, textLines, fontSize, alignment, textcolor,
                                            textshadowcolor, backgroundcolor);

    sdl2::texture_ptr texture{SDL_CreateTextureFromSurface(renderer, surface.get())};

    return DuneTextureOwned{std::move(texture), static_cast<float>(width), static_cast<float>(height)};
}

Point DuneStyle::getMinimumCheckboxSize(std::string_view text) const {
    return {static_cast<int>(getTextWidth(text, 14)) + 20 + 17, static_cast<int>(getTextHeight(14)) + 8};
}

DuneSurfaceOwned
DuneStyle::createCheckboxSurface(uint32_t width, uint32_t height, std::string_view text, bool checked, bool activated,
                                 uint32_t textcolor, uint32_t textshadowcolor, uint32_t backgroundcolor) const {
    const auto [scaled_width, scaled_height] = getPhysicalSize(width, height);

    sdl2::surface_ptr surface{
        SDL_CreateRGBSurface(0, scaled_width, scaled_height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface)
        return {};

    SDL_FillRect(surface.get(), nullptr, backgroundcolor);

    if (textcolor == COLOR_DEFAULT)
        textcolor = defaultForegroundColor;
    if (textshadowcolor == COLOR_DEFAULT)
        textshadowcolor = defaultShadowColor;

    if (activated)
        textcolor = brightenUp(textcolor);

    const auto scale = getActualScale();

    const auto line_width = scale_to_physical_integer(2);

    const auto box_x1 = scale_to_physical_integer(4);
    const auto box_y1 = scale_to_physical_integer(5);
    const auto box_y2 = surface->h - box_y1;
    const auto box_x2 = box_x1 + (box_y2 - box_y1);

    for (auto i = 0; i < line_width; ++i)
        drawRect(surface.get(), box_x1 + i, box_y1 + i, box_x2 - i, box_y2 - i, textcolor);

    if (checked) {
        int x1       = box_x1 + 2;
        int y1       = box_y1 + 2;
        int x2       = box_x2 - 2;
        const int y2 = box_y2 - 2;

        while (y1 < y2 + 1) {
            // North-West to South-East
            putPixel(surface.get(), x1, y1, textcolor);
            putPixel(surface.get(), x1 + 1, y1, textcolor);
            putPixel(surface.get(), x1 - 1, y1, textcolor);

            // North-East to South-West
            putPixel(surface.get(), x2, y1, textcolor);
            putPixel(surface.get(), x2 + 1, y1, textcolor);
            putPixel(surface.get(), x2 - 1, y1, textcolor);

            x1++;
            y1++;
            x2--;
        }
    }

    if (!text.empty()) {
        const auto textSurface1 = createSurfaceWithText(text, textshadowcolor, 14);
        SDL_Rect textRect1{box_x2 + scale_to_physical_integer(4 + 2),
                           (surface->h - textSurface1->h) / 2 + scale_to_physical_integer(1), textSurface1->w,
                           textSurface1->h};
        SDL_BlitSurface(textSurface1.get(), nullptr, surface.get(), &textRect1);

        const auto textSurface2 = createSurfaceWithText(text, textcolor, 14);
        SDL_Rect textRect2{box_x2 + scale_to_physical_integer(4 + 1), (surface->h - textSurface2->h) / 2,
                           textSurface2->w, textSurface2->h};
        SDL_BlitSurface(textSurface2.get(), nullptr, surface.get(), &textRect2);
    }

    return DuneSurfaceOwned{std::move(surface), static_cast<float>(width), static_cast<float>(height)};
}

Point DuneStyle::getMinimumRadioButtonSize(std::string_view text) const {
    return {static_cast<int>(getTextWidth(text, 14)) + 16 + 15, static_cast<int>(getTextHeight(14)) + 8};
}

DuneSurfaceOwned DuneStyle::createRadioButtonSurface(uint32_t width, uint32_t height, std::string_view text,
                                                     bool checked, bool activated, uint32_t textcolor,
                                                     uint32_t textshadowcolor, uint32_t backgroundcolor) const {
    const auto [scaled_width, scaled_height] = getPhysicalSize(width, height);

    sdl2::surface_ptr surface{
        SDL_CreateRGBSurface(0, scaled_width, scaled_height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface)
        return {};

    SDL_FillRect(surface.get(), nullptr, backgroundcolor);

    if (textcolor == COLOR_DEFAULT)
        textcolor = defaultForegroundColor;
    if (textshadowcolor == COLOR_DEFAULT)
        textshadowcolor = defaultShadowColor;

    if (activated)
        textcolor = brightenUp(textcolor);

    drawHLineNoLock(surface.get(), 8, 7, 13, textcolor);
    drawHLineNoLock(surface.get(), 7, 8, 14, textcolor);
    drawHLineNoLock(surface.get(), 6, 9, 8, textcolor);
    drawHLineNoLock(surface.get(), 13, 9, 15, textcolor);
    drawHLineNoLock(surface.get(), 5, 10, 6, textcolor);
    drawHLineNoLock(surface.get(), 15, 10, 16, textcolor);
    drawHLineNoLock(surface.get(), 4, 11, 6, textcolor);
    drawHLineNoLock(surface.get(), 15, 11, 17, textcolor);

    drawVLineNoLock(surface.get(), 4, 12, 15, textcolor);
    drawVLineNoLock(surface.get(), 5, 12, 15, textcolor);
    drawVLineNoLock(surface.get(), 16, 12, 15, textcolor);
    drawVLineNoLock(surface.get(), 17, 12, 15, textcolor);

    drawHLineNoLock(surface.get(), 4, 16, 6, textcolor);
    drawHLineNoLock(surface.get(), 15, 16, 17, textcolor);
    drawHLineNoLock(surface.get(), 5, 17, 6, textcolor);
    drawHLineNoLock(surface.get(), 15, 17, 16, textcolor);
    drawHLineNoLock(surface.get(), 6, 18, 8, textcolor);
    drawHLineNoLock(surface.get(), 13, 18, 15, textcolor);
    drawHLineNoLock(surface.get(), 7, 19, 14, textcolor);
    drawHLineNoLock(surface.get(), 8, 20, 13, textcolor);

    if (checked) {
        drawHLineNoLock(surface.get(), 9, 11, 12, textcolor);
        drawHLineNoLock(surface.get(), 8, 12, 13, textcolor);
        drawHLineNoLock(surface.get(), 8, 13, 13, textcolor);
        drawHLineNoLock(surface.get(), 8, 14, 13, textcolor);
        drawHLineNoLock(surface.get(), 8, 15, 13, textcolor);
        drawHLineNoLock(surface.get(), 9, 16, 12, textcolor);
    }

    if (text.empty()) {
        const auto textSurface1 = createSurfaceWithText(text, textshadowcolor, 14);
        SDL_Rect textRect1 =
            calcDrawingRect(textSurface1.get(), 8 + 2 + 15, surface->h / 2 + 5, HAlign::Left, VAlign::Center);
        SDL_BlitSurface(textSurface1.get(), nullptr, surface.get(), &textRect1);

        const auto textSurface2 = createSurfaceWithText(text, textcolor, 14);
        SDL_Rect textRect2 =
            calcDrawingRect(textSurface2.get(), 8 + 1 + 15, surface->h / 2 + 4, HAlign::Left, VAlign::Center);
        SDL_BlitSurface(textSurface2.get(), nullptr, surface.get(), &textRect2);
    }

    return DuneSurfaceOwned{std::move(surface), static_cast<float>(width), static_cast<float>(height)};
}

DuneSurfaceOwned DuneStyle::createDropDownBoxButton(int size, bool pressed, bool activated, uint32_t color) const {
    if (color == COLOR_DEFAULT)
        color = defaultForegroundColor;

    const auto scaled_size = scale_to_physical_integer(size);

    // create surfaces
    sdl2::surface_ptr surface{
        SDL_CreateRGBSurface(0, scaled_size, scaled_size, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface)
        return {};

    const auto one = std::max(1, scale_to_physical_integer(1));

    // create button background
    if (!pressed) {
        // normal mode
        SDL_FillRect(surface.get(), nullptr, buttonBackgroundColor);
        for (auto i = 0; i < one; ++i) {
            drawRect(surface.get(), i, i, surface->w - 1 - i, surface->h - 1 - i, buttonBorderColor);
            drawHLine(surface.get(), one + i, one + i, surface->w - one - 1 - i, buttonEdgeTopLeftColor);
            drawVLine(surface.get(), one + i, one + i, surface->h - one - 1 - i, buttonEdgeTopLeftColor);
            drawHLine(surface.get(), one + i, surface->h - one - 1 - i, surface->w - one - 1 - i,
                      buttonEdgeBottomRightColor);
            drawVLine(surface.get(), surface->w - one - 1 - i, one, surface->h - one - 1 - i,
                      buttonEdgeBottomRightColor);
        }
    } else {
        // pressed button mode
        SDL_FillRect(surface.get(), nullptr, pressedButtonBackgroundColor);
        for (auto i = 0; i < one; ++i) {
            drawRect(surface.get(), i, i, surface->w - 1 - i, surface->h - 1 - i, buttonBorderColor);
            drawRect(surface.get(), one + i, one + i, surface->w - one - 1 - i, surface->h - one - 1 - i,
                     buttonEdgeBottomRightColor);
        }
    }

    const auto col = (pressed || activated) ? brightenUp(color) : color;

    int x1 = scale_to_physical_integer(2) + one;
    int x2 = scaled_size - x1 - one;
    int y  = scaled_size / 3 - one;

    // Nudge x1 a bit if we have an odd width, otherwise the triangle will be missing its lower point.
    if ((x2 - x1) & 1)
        ++x1;

    // draw separated hline
    {
        const SDL_Rect hline{x1, y, x2 - x1 + 1, one};

        SDL_FillRect(surface.get(), &hline, col);
    }

    y += scale_to_physical_integer(2);

    const auto sdl_color  = RGBA2SDL(col);
    const auto half_color = COLOR_RGBA(sdl_color.r, sdl_color.g, sdl_color.b, sdl_color.a / 2);

    const auto y0 = y;

    // down arrow
    for (; x1 <= x2; ++x1, --x2, ++y) {
        drawHLine(surface.get(), x1, y, x2, col);
        if (y != y0 && x1 != x2) {
            putPixel(surface.get(), x1 - 1, y, half_color);
            putPixel(surface.get(), x2 + 1, y, half_color);
        }
    }

    assert(2 == x1 - x2); // Make sure the triangle had a point at the bottom.

    return DuneSurfaceOwned{std::move(surface), size, size};
}

Point DuneStyle::getMinimumButtonSize(std::string_view text) const {
    return {static_cast<int>(getTextWidth(text, 12)) + 12, static_cast<int>(getTextHeight(12))};
}

sdl2::surface_ptr DuneStyle::createButtonSurface(uint32_t width, uint32_t height, std::string_view text, bool pressed,
                                                 bool activated, uint32_t textcolor, uint32_t textshadowcolor) const {
    sdl2::surface_ptr surface =
        sdl2::surface_ptr{SDL_CreateRGBSurface(0, width, height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface) {
        return nullptr;
    }

    // create button background
    if (!pressed) {
        // normal mode
        SDL_FillRect(surface.get(), nullptr, buttonBackgroundColor);
        drawRect(surface.get(), 0, 0, surface->w - 1, surface->h - 1, buttonBorderColor);
        drawHLine(surface.get(), 1, 1, surface->w - 2, buttonEdgeTopLeftColor);
        drawVLine(surface.get(), 1, 1, surface->h - 2, buttonEdgeTopLeftColor);
        drawHLine(surface.get(), 1, surface->h - 2, surface->w - 2, buttonEdgeBottomRightColor);
        drawVLine(surface.get(), surface->w - 2, 1, surface->h - 2, buttonEdgeBottomRightColor);
    } else {
        // pressed button mode
        SDL_FillRect(surface.get(), nullptr, pressedButtonBackgroundColor);
        drawRect(surface.get(), 0, 0, surface->w - 1, surface->h - 1, buttonBorderColor);
        drawRect(surface.get(), 1, 1, surface->w - 2, surface->h - 2, buttonEdgeBottomRightColor);
    }

    // create text on this button
    int fontsize = 0;
    if ((width < getTextWidth(text, 14) + 12) || (height < getTextHeight(14) + 2)) {
        fontsize = 12;
    } else {
        fontsize = 14;
    }

    if (textcolor == COLOR_DEFAULT)
        textcolor = defaultForegroundColor;
    if (textshadowcolor == COLOR_DEFAULT)
        textshadowcolor = defaultShadowColor;

    if (!text.empty()) {
        const auto textSurface1 = createSurfaceWithText(text, textshadowcolor, fontsize);
        if (textSurface1) {
            SDL_Rect textRect1 =
                calcDrawingRect(textSurface1.get(), surface->w / 2 + 2 + (pressed ? 1 : 0),
                                surface->h / 2 + 3 + (pressed ? 1 : 0), HAlign::Center, VAlign::Center);
            SDL_BlitSurface(textSurface1.get(), nullptr, surface.get(), &textRect1);
        }

        const auto textSurface2 =
            createSurfaceWithText(text, (activated) ? brightenUp(textcolor) : textcolor, fontsize);

        if (textSurface2) {
            SDL_Rect textRect2 =
                calcDrawingRect(textSurface2.get(), surface->w / 2 + 1 + (pressed ? 1 : 0),
                                surface->h / 2 + 2 + (pressed ? 1 : 0), HAlign::Center, VAlign::Center);
            SDL_BlitSurface(textSurface2.get(), nullptr, surface.get(), &textRect2);
        }
    }

    return surface;
}

namespace {

constexpr SDL_FRect offset_rect(const SDL_FRect& reference, float x, float y, float w, float h) {
    return {reference.x + x, reference.y + y, reference.w + w, reference.h + h};
}

constexpr SDL_Rect offset_rect(const SDL_Rect& reference, int x, int y, int w, int h) {
    return {reference.x + x, reference.y + y, reference.w + w, reference.h + h};
}

void fill_rectangle(SDL_Renderer* renderer, const SDL_FRect& dest, float x, float y, float w, float h, Uint32 color) {
    const auto c         = RGBA2SDL(color);
    const SDL_FRect rect = offset_rect(dest, x, y, w, h);

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRectF(renderer, &rect);
}

void draw_rectangle(SDL_Renderer* renderer, const SDL_FRect& dest, float x, float y, float w, float h, Uint32 color) {
    const auto c         = RGBA2SDL(color);
    const SDL_FRect rect = offset_rect(dest, x, y, w, h);

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRectF(renderer, &rect);
}

} // namespace

void DuneStyle::RenderButton(SDL_Renderer* renderer, const SDL_FRect& dest, const DuneTexture* content,
                             bool pressed) const {

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    // SDL_RenderDrawLine(renderer, dest.x, dest.y, dest.x + dest.w, dest.y + dest.h);

    // create button background
    if (!pressed) {
        // normal mode

        // Draw the outline
        {
            static constexpr auto c = RGBA2SDL(buttonBorderColor);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_RenderFillRectF(renderer, &dest);
        }

        // Fill the interior
        fill_rectangle(renderer, dest, 1, 1, -2, -2, buttonBackgroundColor);

        // Draw top/left edge
        {
            static constexpr auto c = RGBA2SDL(buttonEdgeTopLeftColor);
            const std::array top_left{SDL_FRect{dest.x + 1, dest.y + 1, dest.w - 2, 1},
                                      SDL_FRect{dest.x + 1, dest.y + 1, 1, dest.h - 2}};

            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_RenderFillRectsF(renderer, top_left.data(), top_left.size());
        }

        // Draw bottom/right edge
        {
            static constexpr auto c = RGBA2SDL(buttonEdgeBottomRightColor);
            const std::array bottom_right{SDL_FRect{dest.x + 1, dest.y + dest.h - 2, dest.w - 2, 1},
                                          SDL_FRect{dest.x + dest.w - 2, dest.y + 1, 1, dest.h - 2}};

            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_RenderFillRectsF(renderer, bottom_right.data(), bottom_right.size());
        }
    } else {
        // pressed button mode
        static constexpr auto c = RGBA2SDL(pressedButtonBackgroundColor);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRectF(renderer, &dest);

        draw_rectangle(renderer, dest, 0, 0, -1, -1, buttonBorderColor);

        draw_rectangle(renderer, dest, 1, 1, -2, -2, buttonEdgeBottomRightColor);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (!content || !content->texture_)
        return;

    const auto content_x = dest.x + (dest.w - content->width_) / 2;
    const auto content_y = dest.y + (dest.h - content->height_) / 2;

    if (pressed)
        content->draw(renderer, content_x + 1, content_y + 1);
    else
        content->draw(renderer, content_x, content_y);
}

DuneSurfaceOwned DuneStyle::createButtonText(uint32_t width, uint32_t height, std::string_view text, bool activated,
                                             uint32_t textcolor, uint32_t textshadowcolor) const {
    // create text on this button
    int fontsize = 0;
    if ((width < getTextWidth(text, 14) + 12) || (height < getTextHeight(14) + 2)) {
        fontsize = 12;
    } else {
        fontsize = 14;
    }

    if (textcolor == COLOR_DEFAULT)
        textcolor = defaultForegroundColor;
    if (textshadowcolor == COLOR_DEFAULT)
        textshadowcolor = defaultShadowColor;

    if (text.empty())
        return {};

    const auto scale = getActualScale();

    const auto shadow_surface = createSurfaceWithText(text, textshadowcolor, fontsize);
    const auto text_surface   = createSurfaceWithText(text, activated ? brightenUp(textcolor) : textcolor, fontsize);

    if (!shadow_surface || !text_surface)
        return {};

    const auto dx = static_cast<int>(std::round(0.75f * scale));
    const auto dy = static_cast<int>(std::round(0.75f * scale));

    const auto actual_width  = std::max(shadow_surface->w + dx, text_surface->w);
    const auto actual_height = std::max(shadow_surface->h + dy, text_surface->h);

    sdl2::surface_ptr surface{
        SDL_CreateRGBSurface(0, actual_width, actual_height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};

    if (!surface)
        return {};

    SDL_SetSurfaceBlendMode(surface.get(), SDL_BLENDMODE_BLEND);

    { // Scope
        const auto x = (surface->w - shadow_surface->w) / 2 + dx;
        const auto y = (surface->h - shadow_surface->h) / 2 + dy;

        SDL_Rect dest{x, y, shadow_surface->w, shadow_surface->h};

        SDL_SetSurfaceBlendMode(shadow_surface.get(), SDL_BLENDMODE_NONE);
        SDL_BlitSurface(shadow_surface.get(), nullptr, surface.get(), &dest);
    }

    { // Scope
        const auto x = (surface->w - text_surface->w) / 2;
        const auto y = (surface->h - text_surface->h) / 2;

        SDL_Rect dest{x, y, text_surface->w, text_surface->h};

        SDL_SetSurfaceBlendMode(shadow_surface.get(), SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(text_surface.get(), nullptr, surface.get(), &dest);
    }

    const auto inverse_scale = 1.f / scale;

    return DuneSurfaceOwned{std::move(surface), static_cast<float>(actual_width) * inverse_scale,
                            static_cast<float>(actual_height) * inverse_scale};
}

Point DuneStyle::getMinimumTextBoxSize(int fontSize) const {
    return {10, static_cast<int>(getTextHeight(fontSize)) + 6};
}

DuneSurfaceOwned
DuneStyle::createTextBoxSurface(uint32_t width, uint32_t height, std::string_view text, bool carret, int fontSize,
                                Alignment_Enum alignment, uint32_t textcolor, uint32_t textshadowcolor) const {
    const auto [scaled_width, scaled_height] = getPhysicalSize(width, height);

    sdl2::surface_ptr surface =
        sdl2::surface_ptr{SDL_CreateRGBSurface(0, scaled_width, scaled_height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface)
        return {};

    SDL_FillRect(surface.get(), nullptr, buttonBackgroundColor);
    drawRect(surface.get(), 0, 0, surface->w - 1, surface->h - 1, buttonBorderColor);

    drawHLine(surface.get(), 1, 1, surface->w - 2, buttonEdgeBottomRightColor);
    drawHLine(surface.get(), 1, 2, surface->w - 2, buttonEdgeBottomRightColor);
    drawVLine(surface.get(), 1, 1, surface->h - 2, buttonEdgeBottomRightColor);
    drawVLine(surface.get(), 2, 1, surface->h - 2, buttonEdgeBottomRightColor);
    drawHLine(surface.get(), 1, surface->h - 2, surface->w - 2, buttonEdgeTopLeftColor);
    drawVLine(surface.get(), surface->w - 2, 1, surface->h - 2, buttonEdgeTopLeftColor);

    if (textcolor == COLOR_DEFAULT)
        textcolor = defaultForegroundColor;
    if (textshadowcolor == COLOR_DEFAULT)
        textshadowcolor = defaultShadowColor;

    SDL_Rect cursorPos{};

    // create text in this text box
    if (!text.empty()) {
        const auto textSurface1 = createSurfaceWithText(text, textshadowcolor, fontSize);
        const auto textSurface2 = createSurfaceWithText(text, textcolor, fontSize);
        SDL_Rect textRect1 = calcDrawingRect(textSurface1.get(), 0, surface->h / 2 + 4, HAlign::Left, VAlign::Center);
        SDL_Rect textRect2 = calcDrawingRect(textSurface2.get(), 0, surface->h / 2 + 3, HAlign::Left, VAlign::Center);

        if (alignment & Alignment_Left) {
            textRect1.x = 6;
            textRect2.x = 5;
        } else if (alignment & Alignment_Right) {
            textRect1.x = scaled_width - textSurface1->w - 5;
            textRect2.x = scaled_width - textSurface2->w - 4;
        } else {
            // Alignment_HCenter
            textRect1.x = ((surface->w - textSurface1->w) / 2) + 3;
            textRect2.x = ((surface->w - textSurface2->w) / 2) + 2;
        }

        if (textRect1.w > surface->w - 10) {
            textRect1.x -= (textSurface1->w - (surface->w - 10));
            textRect2.x -= (textSurface2->w - (surface->w - 10));
        }

        cursorPos.x = textRect2.x + textSurface2->w + 2;

        SDL_BlitSurface(textSurface1.get(), nullptr, surface.get(), &textRect1);

        SDL_BlitSurface(textSurface2.get(), nullptr, surface.get(), &textRect2);

        cursorPos.w = 1;
    } else {
        if (alignment & Alignment_Left) {
            cursorPos.x = 6;
        } else if (alignment & Alignment_Right) {
            cursorPos.x = scaled_width - 5;
        } else {
            // Alignment_HCenter
            cursorPos.x = surface->w / 2;
        }
        cursorPos.w = 1;
    }

    cursorPos.y = surface->h / 2 - 8;
    cursorPos.h = 16;

    if (carret) {
        drawVLine(surface.get(), cursorPos.x - 2, cursorPos.y, cursorPos.y + cursorPos.h, textcolor);
        drawVLine(surface.get(), cursorPos.x - 1, cursorPos.y, cursorPos.y + cursorPos.h, textcolor);
    }

    return DuneSurfaceOwned{std::move(surface), static_cast<float>(width), static_cast<float>(height)};
}

Point DuneStyle::getMinimumScrollBarArrowButtonSize() const {
    return {17, 17};
}

sdl2::surface_ptr DuneStyle::createScrollBarArrowButton(bool down, bool pressed, bool activated, uint32_t color) const {
    if (color == COLOR_DEFAULT) {
        color = defaultForegroundColor;
    }

    // create surfaces
    sdl2::surface_ptr surface =
        sdl2::surface_ptr{SDL_CreateRGBSurface(0, 17, 17, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface) {
        return nullptr;
    }

    // create button background
    if (!pressed) {
        // normal mode
        SDL_FillRect(surface.get(), nullptr, buttonBackgroundColor);
        drawRect(surface.get(), 0, 0, surface->w - 1, surface->h - 1, buttonBorderColor);
        drawHLine(surface.get(), 1, 1, surface->w - 2, buttonEdgeTopLeftColor);
        drawVLine(surface.get(), 1, 1, surface->h - 2, buttonEdgeTopLeftColor);
        drawHLine(surface.get(), 1, surface->h - 2, surface->w - 2, buttonEdgeBottomRightColor);
        drawVLine(surface.get(), surface->w - 2, 1, surface->h - 2, buttonEdgeBottomRightColor);
    } else {
        // pressed button mode
        SDL_FillRect(surface.get(), nullptr, pressedButtonBackgroundColor);
        drawRect(surface.get(), 0, 0, surface->w - 1, surface->h - 1, buttonBorderColor);
        drawRect(surface.get(), 1, 1, surface->w - 2, surface->h - 2, buttonEdgeBottomRightColor);
    }

    const int col = (pressed | activated) ? brightenUp(color) : color;

    // draw arrow
    if (down) {
        // down arrow
        drawHLine(surface.get(), 3, 4, 13, col);
        drawHLine(surface.get(), 4, 5, 12, col);
        drawHLine(surface.get(), 5, 6, 11, col);
        drawHLine(surface.get(), 6, 7, 10, col);
        drawHLine(surface.get(), 7, 8, 9, col);
        drawHLine(surface.get(), 8, 9, 8, col);
    } else {
        // up arrow
        drawHLine(surface.get(), 8, 5, 8, col);
        drawHLine(surface.get(), 7, 6, 9, col);
        drawHLine(surface.get(), 6, 7, 10, col);
        drawHLine(surface.get(), 5, 8, 11, col);
        drawHLine(surface.get(), 4, 9, 12, col);
        drawHLine(surface.get(), 3, 10, 13, col);
    }

    return surface;
}

int DuneStyle::getListBoxEntryHeight() const {
    return 16;
}

DuneSurfaceOwned DuneStyle::createListBoxEntry(int width, std::string_view text, bool selected, uint32_t color) const {
    if (color == COLOR_DEFAULT)
        color = defaultForegroundColor;

    const auto height = getListBoxEntryHeight();

    const auto [scaled_width, scaled_height] = getPhysicalSize(width, height);

    // create surfaces
    sdl2::surface_ptr surface{
        SDL_CreateRGBSurface(0, scaled_width, scaled_height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (!surface)
        return {};

    SDL_FillRect(surface.get(), nullptr, selected ? buttonBackgroundColor : COLOR_TRANSPARENT);

    const auto textSurface = createSurfaceWithText(text, color, 12);

    SDL_Rect textRect{3, 2 + (surface->h - textSurface->h) / 2, textSurface->w, textSurface->h};

    SDL_BlitSurface(textSurface.get(), nullptr, surface.get(), &textRect);

    return DuneSurfaceOwned{std::move(surface), static_cast<float>(width), static_cast<float>(height)};
}

sdl2::surface_ptr
DuneStyle::createProgressBarOverlay(uint32_t width, uint32_t height, double percent, uint32_t color) const {
    sdl2::surface_ptr pSurface{SDL_CreateRGBSurface(0, static_cast<int>(width), static_cast<int>(height), SCREEN_BPP,
                                                    RMASK, GMASK, BMASK, AMASK)};
    if (!pSurface)
        return nullptr;

    SDL_FillRect(pSurface.get(), nullptr, COLOR_TRANSPARENT);

    if (color == COLOR_DEFAULT) {
        // default color

        const int max_i = std::max(static_cast<int>(std::round(percent * 0.01 * (static_cast<int>(width) - 4))), 0);

        const SDL_Rect dest = {2, 2, max_i, static_cast<int>(height) - 4};
        SDL_FillRect(pSurface.get(), &dest, COLOR_HALF_TRANSPARENT);
    } else {
        const auto max_i = static_cast<int>(std::round(percent * 0.01 * width));

        const SDL_Rect dest = {0, 0, max_i, static_cast<int>(height)};
        SDL_FillRect(pSurface.get(), &dest, color);
    }

    return pSurface;
}

DuneTextureOwned DuneStyle::createToolTip(SDL_Renderer* renderer, std::string_view text) const {
    const auto helpTextSurface = createSurfaceWithText(text, COLOR_YELLOW, 12);
    if (!helpTextSurface) {
        return {};
    }

    // create surfaces
    const auto surface = sdl2::surface_ptr{SDL_CreateRGBSurface(0, helpTextSurface->w + 5, helpTextSurface->h + 2,
                                                                SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};
    if (surface == nullptr) {
        return {};
    }

    SDL_FillRect(surface.get(), nullptr, COLOR_HALF_TRANSPARENT);

    drawRect(surface.get(), 0, 0, helpTextSurface->w + 5 - 1, helpTextSurface->h + 2 - 1, COLOR_YELLOW);

    SDL_Rect textRect = calcDrawingRect(helpTextSurface.get(), 3, 3);
    SDL_BlitSurface(helpTextSurface.get(), nullptr, surface.get(), &textRect);

    sdl2::texture_ptr texture{SDL_CreateTextureFromSurface(renderer, surface.get())};

    const auto scale = 1.f / getActualScale();

    return DuneTextureOwned{std::move(texture), static_cast<float>(surface->w) * scale,
                            static_cast<float>(surface->h) * scale};
}

void DuneStyle::drawBackground(SDL_Renderer* renderer, const SDL_FRect& rect) {
    if (rect.w <= 0 || rect.h <= 0)
        THROW(std::invalid_argument, "Attempting to draw an empty background!");

    auto* tile = dune::globals::pGFXManager->getUIGraphic(UI_Background_Tile);

    const auto scale = 1.f / getActualScale();

    const auto tile_width  = tile->width_ * scale;
    const auto tile_height = tile->height_ * scale;

    SDL_FRect dest{rect.x, rect.y, tile_width, tile_height};

    const auto src = tile->source_.as_sdl();

    for (auto y = rect.y; y <= rect.y + rect.h; y += tile_height) {
        dest.y = y;
        for (auto x = rect.x; x <= rect.x + rect.w; x += tile_width) {
            dest.x = x;
            SDL_RenderCopyF(renderer, tile->texture_, &src, &dest);
        }
    }
}

void DuneStyle::drawMainBackground(SDL_Renderer* renderer, const SDL_FRect& rect) {
    drawBackground(renderer, rect);

    auto* const gfx = dune::globals::pGFXManager.get();

    auto* harkonnen = gfx->getUIGraphic(UI_Background_Logo, HOUSETYPE::HOUSE_HARKONNEN);
    auto* atreides  = gfx->getUIGraphic(UI_Background_Logo, HOUSETYPE::HOUSE_ATREIDES);
    auto* ordos     = gfx->getUIGraphic(UI_Background_Logo, HOUSETYPE::HOUSE_ORDOS);

    if (harkonnen)
        harkonnen->draw(renderer, 11, 11);

    if (atreides)
        atreides->draw(renderer, rect.w - 11 - atreides->width_, 11);

    if (ordos)
        ordos->draw(renderer, 11, rect.h - 11 - ordos->height_);
}

DuneSurfaceOwned DuneStyle::createWidgetBackground(int width, int height) const {
    const auto [scaled_width, scaled_height] = getPhysicalSize(width, height);

    sdl2::surface_ptr surface =
        sdl2::surface_ptr{SDL_CreateRGBSurface(0, width, height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK)};

    if (!surface)
        return {};

    SDL_FillRect(surface.get(), nullptr, pressedButtonBackgroundColor);
    drawRect(surface.get(), 0, 0, surface->w - 1, surface->h - 1, buttonBorderColor);
    drawRect(surface.get(), 1, 1, surface->w - 2, surface->h - 2, buttonEdgeBottomRightColor);

    return DuneSurfaceOwned{std::move(surface), static_cast<float>(width), static_cast<float>(height)};
}
