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

#ifndef WIDGETWITHBACKGROUND_H
#define WIDGETWITHBACKGROUND_H

#include "Widget.h"

#include <misc/SDL2pp.h>

#include "Renderer/DuneSurface.h"

struct SDL_Texture;

class WidgetWithBackground : public Widget {
    using parent = Widget;

public:
    WidgetWithBackground()                            = default;
    WidgetWithBackground(const WidgetWithBackground&) = delete;
    WidgetWithBackground(WidgetWithBackground&&)      = default;

    ~WidgetWithBackground() override;

    WidgetWithBackground& operator=(const WidgetWithBackground&) = delete;
    WidgetWithBackground& operator=(WidgetWithBackground&&)      = default;

    /**
        This method sets a transparent background for this widget.
        \param bTransparent true = the background is transparent, false = the background is not transparent
    */
    virtual void setTransparentBackground(bool bTransparent);

    void setBackground(const DuneTexture* pBackground);
    void setBackground(DuneTextureOwned background);

    /**
        This method resizes the widget to width and height. This method should only be
        called if the new size is a valid size for this widget (See resizingXAllowed,
        resizingYAllowed, getMinimumSize).
        \param  width   the new width of this widget
        \param  height  the new height of this widget
    */
    void resize(uint32_t width, uint32_t height) override;

    using parent::resize;

    /**
        Draws this widget to screen
        \param  position    Position to draw the progress bar to
    */
    void draw(Point position) override;

protected:
    /**
        Draw the background.
        \param  position    Position to draw the background
     */
    virtual void draw_background(Point position);

    /**
        This method frees all textures that are used by this widget
    */
    void invalidateTextures() override;

    [[nodiscard]] const DuneTexture* getBackground() const noexcept { return pBackground_; }
    [[nodiscard]] const DuneTexture* getBackground();

    void setBackground(SDL_Surface* surface);

    virtual DuneSurfaceOwned createBackground();

    void setCenterBackground(bool value) { center_background_ = value; }
    bool getCenterBackground() const { return center_background_; }

private:
    const DuneTexture* pBackground_{}; ///< background texture

    bool bTransparentBackground_{}; ///< true = no background is drawn
    bool bSelfGeneratedBackground_{
        true}; ///< true = background is created by this window, false = created by someone else
    bool center_background_{true};

    sdl2::texture_ptr localTexture_;
    DuneTexture localDuneTexture_;
};

#endif // WIDGETWITHBACKGROUND_H
