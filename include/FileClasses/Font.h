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

#ifndef FONT_H
#define FONT_H

#include "misc/SDL2pp.h"

#include <string_view>

class Font {
protected:
    Font() = default;

public:
    virtual ~Font();

    Font(const Font&)            = delete;
    Font(Font&&)                 = delete;
    Font& operator=(const Font&) = delete;
    Font& operator=(Font&&)      = delete;

    [[nodiscard]] virtual sdl2::surface_ptr
    createTextSurface(std::string_view text, uint32_t baseColor = 0xFFFFFFFFu) const = 0;

    [[nodiscard]] virtual sdl2::surface_ptr
    createMultilineTextSurface(std::string_view text, uint32_t color, bool bCentered) const;

    /// Returns the number of pixels a text needs
    /**
        This methods returns the number of pixels this text would need if printed.
        \param  text    The text to be checked for it's length in pixel
        \return Number of pixels needed
    */
    [[nodiscard]] virtual int getTextWidth(std::string_view text) const = 0;

    /// Returns the number of pixels this font needs in y-direction.
    /**
        This methods returns the height of this font.
        \return Number of pixels needed
    */
    [[nodiscard]] virtual int getTextHeight() const = 0;
};

#endif // FONT_H
