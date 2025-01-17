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

#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "PictureButton.h"
#include "TextButton.h"
#include "Widget.h"

#include <functional>

/// A class for a scroll bar
class ScrollBar final : public Widget {
    using parent = Widget;

public:
    /// default constructor
    ScrollBar();

    ScrollBar(const ScrollBar&)            = delete;
    ScrollBar(ScrollBar&&)                 = delete;
    ScrollBar& operator=(const ScrollBar&) = delete;
    ScrollBar& operator=(ScrollBar&&)      = delete;

    /// destructor
    ~ScrollBar() override;

    /**
        Handles a mouse movement. This method is for example needed for the tooltip.
        \param  x               x-coordinate (relative to the left top corner of the widget)
        \param  y               y-coordinate (relative to the left top corner of the widget)
        \param  insideOverlay   true, if (x,y) is inside an overlay and this widget may be behind it, false otherwise
    */
    void handleMouseMovement(int32_t x, int32_t y, bool insideOverlay) override;

    /**
        Handles a left mouse click.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  pressed true = mouse button pressed, false = mouse button released
        \return true = click was processed by the widget, false = click was not processed by the widget
    */
    bool handleMouseLeft(int32_t x, int32_t y, bool pressed) override;

    /**
        Handles mouse wheel scrolling.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  up  true = mouse wheel up, false = mouse wheel down
        \return true = the mouse wheel scrolling was processed by the widget, false = mouse wheel scrolling was not
       processed by the widget
    */

    bool handleMouseWheel(int32_t x, int32_t y, bool up) override;

    /**
        Handles a key stroke. This method is neccessary for controlling an application
        without a mouse.
        \param  key the key that was pressed or released.
        \return true = key stroke was processed by the widget, false = key stroke was not processed by the widget
    */
    bool handleKeyPress(const SDL_KeyboardEvent& key) override;

    /**
        Draws this scroll bar to screen. This method is called before drawOverlay().
        \param  position    Position to draw the scroll bar to
    */
    void draw(Point position) override;

    /**
        This method resizes the scroll bar to width and height. This method should only
        called if the new size is a valid size for this scroll bar (See getMinimumSize).
        \param  width   the new width of this scroll bar
        \param  height  the new height of this scroll bar
    */
    void resize(uint32_t width, uint32_t height) override;

    using parent::resize;

    /**
        Returns the minimum size of this scroll bar. The scroll bar should not
        resized to a size smaller than this.
        \return the minimum size of this scroll bar
    */
    [[nodiscard]] [[nodiscard]] Point getMinimumSize() const override {
        auto tmp = GUIStyle::getInstance().getMinimumScrollBarArrowButtonSize();
        tmp.y    = tmp.y * 3;
        return tmp;
    }

    /**
        Sets the range of this ScrollBar.
        \param  minValue    the minimum value
        \param  maxValue    the maximum value
    */
    void setRange(int minValue, int maxValue) {
        if (minValue > maxValue) {
            return;
        }

        this->minValue_ = minValue;
        this->maxValue_ = maxValue;

        if (currentValue_ < minValue) {
            currentValue_ = minValue;
        } else if (currentValue_ > maxValue) {
            currentValue_ = maxValue;
        }

        updateSliderButton();
    }

    /**
        Gets the range start of this ScrollBar.
        \return the start value of the range
    */
    [[nodiscard]] [[nodiscard]] int getRangeMin() const noexcept { return minValue_; }

    /**
        Gets the range end of this ScrollBar.
        \return the end value of the range
    */
    [[nodiscard]] [[nodiscard]] int getRangeMax() const noexcept { return maxValue_; }

    /**
        Sets the big step size that is used when clicking between the arrows and the slider.
        This value is also used to determine the size of the slider.
        \param  stepSize    the new step size to advance scollbar when clicking inbetween
    */
    void setBigStepSize(int stepSize) {
        bigStepSize_ = stepSize;
        updateSliderButton();
    }

    /**
        Returns the current position
        \return the current value
    */
    [[nodiscard]] [[nodiscard]] int getCurrentValue() const noexcept { return currentValue_; }

    /**
        Sets the current position. Should be in range
        \param newValue the new position
    */
    void setCurrentValue(int newValue) {
        currentValue_ = newValue;
        if (currentValue_ < minValue_) {
            currentValue_ = minValue_;
        } else if (currentValue_ > maxValue_) {
            currentValue_ = maxValue_;
        }

        updateSliderButton();

        if (pOnChange_) {
            pOnChange_();
        }
    }

    /**
        Sets the function that should be called when this scroll bar changes its position.
        \param  pOnChange   A function to be called on change
    */
    void setOnChange(std::function<void()> pOnChange) { this->pOnChange_ = pOnChange; }

    /**
        Sets the color for this scrollbar.
        \param  color   the color (COLOR_DEFAULT = default color)
    */
    virtual void setColor(uint32_t color) {
        this->color_ = color;
        updateArrowButtonSurface();
    }

protected:
    /**
        This method is called whenever the textures of this widget are needed, e.g. before drawing. This method
        should be overwritten by subclasses if they like to defer texture creation as long as possible.
        This method should first check whether a renewal of the textures is necessary.
    */
    void updateTextures() override {
        parent::updateTextures();

        if (pBackground_)
            return;

        const auto& gui      = GUIStyle::getInstance();
        auto* const renderer = dune::globals::renderer.get();

        pBackground_ = gui.createWidgetBackground(getSize().x, getSize().y).createTexture(renderer);
    }

    /**
        This method frees all textures that are used by this scrollbar
    */
    void invalidateTextures() override {
        pBackground_.reset();

        parent::invalidateTextures();
    }

private:
    void updateSliderButton();

    void updateArrowButtonSurface();

    void onArrow1() { setCurrentValue(currentValue_ - 1); }

    void onArrow2() { setCurrentValue(currentValue_ + 1); }

    DuneTextureOwned pBackground_;
    PictureButton arrow1_;
    PictureButton arrow2_;
    TextButton sliderButton_;

    std::function<void()> pOnChange_; ///< function that is called when this scrollbar changes its position

    int currentValue_ = 1;
    int minValue_     = 1;
    int maxValue_     = 1;
    int bigStepSize_  = 10; ///< the step size when clicking between the arrows and the slider
    Point sliderPosition_;

    bool bDragSlider_              = false;
    int dragPositionFromSliderTop_ = false;

    uint32_t color_; ///< the color
};

#endif // SCROLLBAR_H
