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

#ifndef RADIOBUTTON_H
#define RADIOBUTTON_H

#include "Button.h"
#include "GUIStyle.h"

#include "RadioButtonManager.h"

#include <string>

/// A class for a radio button implemented as a toggle button
class RadioButton final : public Button {
    using parent = Button;

public:
    /// Default constructor
    RadioButton();

    /// destructor
    ~RadioButton() override;

    void registerRadioButtonManager(RadioButtonManager* pNewRadioButtonManager);

    void unregisterFromRadioButtonManager();

    /**
        This method sets a new text for this radio button and resizes it
        to fit this text.
        \param  text The new text for this radio button
    */
    virtual void setText(const std::string& text) {
        this->text_ = text;
        resizeAll();
    }

    /**
        Get the text of this radio button.
        \return the text of this radio button
    */
    [[nodiscard]] const std::string& getText() const { return text_; }

    /**
        Sets the text color for this radio button.
        \param  textcolor       the color of the text (COLOR_DEFAULT = default color)
        \param  textshadowcolor the color of the shadow of the text (COLOR_DEFAULT = default color)
    */
    virtual void setTextColor(uint32_t textcolor, Uint32 textshadowcolor = COLOR_DEFAULT);

    /**
        This method sets the current toggle state. On radio buttons this is only
        effective for bToggleState == true, so you can only set a radio button to be selected
        but cannot deselect it
        \param bToggleState true = toggled, false = untoggled
    */
    void setToggleState(bool bToggleState) override;

    /**
        This method sets this radio button to checked or unchecked. It does the same as setToggleState().
        \param bChecked true = checked, false = unchecked
    */
    void setChecked(bool bChecked) { setToggleState(bChecked); }

    /**
        This method returns whether this radio button is checked. It is the same as getToggleState().
        \return true = checked, false = unchecked
    */
    [[nodiscard]] bool isChecked() const { return getToggleState(); }

    /**
        Draws this button to screen. This method is called before drawOverlay().
        \param  position    Position to draw the button to
    */
    void draw(Point position) override;

    /**
        This method resizes the radio button to width and height. This method should only
        called if the new size is a valid size for this radio button (See getMinimumSize).
        \param  width   the new width of this radio button
        \param  height  the new height of this radio button
    */
    void resize(uint32_t width, uint32_t height) override;

    using parent::resize;

    /**
        Returns the minimum size of this button. The button should not
        resized to a size smaller than this.
        \return the minimum size of this button
    */
    [[nodiscard]] Point getMinimumSize() const override {
        return GUIStyle::getInstance().getMinimumRadioButtonSize(text_);
    }

protected:
    /**
        This method is called whenever the textures of this widget are needed, e.g. before drawing. This method
        should be overwritten by subclasses if they like to defer texture creation as long as possible.
        This method should first check whether a renewal of the textures is necessary.
    */
    void updateTextures() override;

    /**
        This method frees all textures that are used by this radio button
    */
    void invalidateTextures() override;

private:
    uint32_t text_color_        = COLOR_DEFAULT; ///< Text color
    uint32_t text_shadow_color_ = COLOR_DEFAULT; ///< Text shadow color
    std::string text_;                           ///< Text of this radio button
    DuneTextureOwned pCheckedActiveTexture_; ///< Texture that is shown when the radio button is activated by keyboard
                                             ///< or by mouse hover

    RadioButtonManager* pRadioButtonManager_{}; ///< The Manager for managing the toggle states
};

#endif // RADIOBUTTON_H
