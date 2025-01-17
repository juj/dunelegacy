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

#ifndef WIDGET_H
#define WIDGET_H

#include "Renderer/DuneTexture.h"

#include <misc/SDL2pp.h>

#include <algorithm>
#include <concepts>
#include <functional>

/// A point class for representing a point.
class Point final {
public:
    /// Default constructor.
    Point() : x(0), y(0) { }

    /** A constructor that initializes the point with x and y
        \param x the x coordinate
        \param y the y coordinate
    */
    Point(int32_t x, int32_t y) : x(x), y(y) { }

    /** operator for adding two points.
        \param p the point to add
        \return the sum of both points
    */
    Point operator+(const Point& p) const { return {x + p.x, y + p.y}; }

    /** operator for subtracting two points.
        \param p the point to add
        \return this point minus p
    */
    Point operator-(const Point& p) const { return {x - p.x, y - p.y}; }

    /** operator for multiplying every both coordinates with an integer value.
        \param c the scalar to multiply with
        \return this point times c
    */
    Point operator*(int32_t c) const { return {x * c, y * c}; }

    /** operator for dividing every both coordinates by an integer value.
        \param c the scalar to devide by
        \return this point divided by c
    */
    Point operator/(int32_t c) const { return {x / c, y / c}; }

    /** operator for comparing two Points.
        \param op the other operator
        \return true if both coordinates are equal, false otherwise
    */
    bool operator==(const Point& op) const { return (x == op.x) && (y == op.y); }

    /// The x coordinate
    int32_t x;

    /// The y coordinate
    int32_t y;
};

/**
    Return the dimensions of the given surface
    \param  pSurface    the surface to measure
    \return the size of pSurface
*/
inline Point getSurfaceSize(SDL_Surface* pSurface) {
    return {pSurface->w, pSurface->h};
}

/**
    Return the dimensions of the given texture
    \param  pTexture    the texture to measure
    \return the size of pTexture
*/
inline Point getTextureSize(SDL_Texture* pTexture) {
    Point p;
    if (pTexture != nullptr) {
        SDL_QueryTexture(pTexture, nullptr, nullptr, &p.x, &p.y);
    }
    return p;
}

/**
    Return the dimensions of the given texture
    \param  pTexture    the texture to measure
    \return the size of pTexture
*/
inline Point getTextureSize(const DuneTexture* pTexture) {
    Point p;
    if (pTexture != nullptr) {
        p.x = static_cast<int>(std::ceil(pTexture->width_));
        p.y = static_cast<int>(std::ceil(pTexture->height_));
    }
    return p;
}

// forward declarations
class Window;
template<class WidgetData>
class Container;

/// The abstract base class for all widgets
class Widget {
public:
    template<class WidgetData>
    friend class Container;

    friend class Window;

protected:
    /** The default constructor.
     */
    Widget();

public:
    /** Destructor
        Removes this widget from its parent.
    */
    virtual ~Widget() = 0;

    Widget(const Widget&)            = delete;
    Widget(Widget&&)                 = default;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&)      = default;

    /**
        Sets the parent of this widget.
        \param pParent new parent (nullptr is also possible)
    */
    void setParent(Widget* pParent) noexcept { parent_ = pParent; }

    /**
        Returns the parent of this widget
        \return the parent of this widget or nullptr if it has no parent.
    */
    [[nodiscard]] Widget* getParent() const noexcept { return parent_; }

    /**
        Enable or disable this widget. A disabled widget is not responding
        to clicks and key strokes and might look different.
        \param  bEnabled    true = enable widget, false = disable widget
    */
    virtual void setEnabled(bool bEnabled) {
        if ((bEnabled == false) && (isActive() == true)) {
            setInactive();
        }
        enabled_ = bEnabled;
    }

    /**
        Returns whether this widget is enabled or not.
        \return true = widget is enabled, false = widget is disabled
    */
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    /**
        Sets this widget visible or invisible. An invisible widget does not
        responding to clicks and key presses.
        \return bVisible    true = visible, false = invisible
    */
    virtual void setVisible(bool bVisible) { visible_ = bVisible; }

    /**
        Returns whether this widget is visible or not.
        \return true = visible, false = invisible
    */
    [[nodiscard]] bool isVisible() const noexcept { return visible_; }

    /**
        Sets this widget active. The parent widgets are also activated and the
        currently active widget is set to inactive.
    */
    virtual void setActive();

    /**
        Sets this widget inactive. The next activatable widget is activated.
    */
    virtual void setInactive();

    /**
        Returns whether this widget is currently active.
        \return true = currently active, false = currently inactive
    */
    [[nodiscard]] bool isActive() const noexcept { return active_; }

    /**
        Returns whether this widget can be set active.
        \return true = activatable, false = not activatable
    */
    [[nodiscard]] virtual bool isActivatable() const { return false; }

    /**
        Returns whether this widget is an container.
        \return true = container, false = any other widget
    */
    [[nodiscard]] virtual bool isContainer() const { return false; }

    /**
        Returns the current size of this widget.
        \return current size of this widget
    */
    [[nodiscard]] const Point& getSize() const noexcept { return size_; }

    /**
        Returns the minimum size of this widget. The widget should not
        resized to a size smaller than this. If the widget is not resizeable
        in a direction this method returns the size in that direction.
        \return the minimum size of this widget
    */
    [[nodiscard]] virtual Point getMinimumSize() const { return {0, 0}; }

    /**
        Returns whether this widget is allowed to be resized in X direction.
        You should not resize a widget, that is not resizeable.
        \return true = resizeable in X direction, false = not resizeable in X direction
    */
    [[nodiscard]] bool resizingXAllowed() const { return resizeX_; }

    /**
        Returns whether this widget is allowed to be resized in Y direction.
        You should not resize a widget, that is not resizeable.
        \return true = resizeable in Y direction, false = not resizeable in Y direction
    */
    [[nodiscard]] bool resizingYAllowed() const { return resizeY_; }

    /**
        This method resizes the widget. This method should only be
        called if the new size is a valid size for this widget (See resizingXAllowed,
        resizingYAllowed, getMinimumSize).
        \param  newSize the new size of this widget
    */
    void resize(Point newSize) { resize(newSize.x, newSize.y); }

    /**
        This method resizes the widget to width and height. This method should only be
        called if the new size is a valid size for this widget (See resizingXAllowed,
        resizingYAllowed, getMinimumSize).
        \param  width   the new width of this widget
        \param  height  the new height of this widget
    */
    virtual void resize(uint32_t width, uint32_t height) {
        size_.x = width;
        size_.y = height;
    }

    /**
        This method resizes the widget and its parent (the surrounding container).
    */
    virtual void resizeAll();

    /**
        Handles a mouse movement.
        \param  x               x-coordinate (relative to the left top corner of the widget)
        \param  y               y-coordinate (relative to the left top corner of the widget)
        \param  insideOverlay   true, if (x,y) is inside an overlay and this widget may be behind it, false otherwise
    */
    virtual void handleMouseMovement(int32_t x, int32_t y, bool insideOverlay) { }

    /**
        Handles a left mouse click.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  pressed true = mouse button pressed, false = mouse button released
        \return true = click was processed by the widget, false = click was not processed by the widget
    */
    virtual bool handleMouseLeft(int32_t x, int32_t y, bool pressed) { return false; }

    /**
        Handles a right mouse click.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  pressed true = mouse button pressed, false = mouse button released
        \return true = click was processed by the widget, false = click was not processed by the widget
    */
    virtual bool handleMouseRight(int32_t x, int32_t y, bool pressed) { return false; }

    /**
        Handles mouse wheel scrolling.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  up  true = mouse wheel up, false = mouse wheel down
        \return true = the mouse wheel scrolling was processed by the widget, false = mouse wheel scrolling was not
       processed by the widget
    */
    virtual bool handleMouseWheel(int32_t x, int32_t y, bool up) { return false; }

    /**
        Handles a key stroke.
        \param  key the key that was pressed or released.
        \return true = key stroke was processed by the widget, false = key stroke was not processed by the widget
    */
    virtual bool handleKeyPress(const SDL_KeyboardEvent& key) {
        if (isActive() && (key.keysym.sym == SDLK_TAB)) {
            setInactive();
        }
        return false;
    }

    /**
        Handles a text input event.
        \param  textInput the text input that was performed.
        \return true = text input was processed by the widget, false = text input was not processed by the widget
    */
    virtual bool handleTextInput(const SDL_TextInputEvent& textInput) { return false; }

    /**
        Handles mouse movement in overlays.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \return true if (x,y) is in overlay of this widget, false otherwise
    */
    virtual bool handleMouseMovementOverlay(int32_t x, int32_t y) { return false; }

    /**
        Handles a left mouse click in overlays.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  pressed true = mouse button pressed, false = mouse button released
        \return true = click was processed by the widget, false = click was not processed by the widget
    */
    virtual bool handleMouseLeftOverlay(int32_t x, int32_t y, bool pressed) { return false; }

    /**
        Handles a right mouse click in overlays.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  pressed true = mouse button pressed, false = mouse button released
        \return true = click was processed by the widget, false = click was not processed by the widget
    */
    virtual bool handleMouseRightOverlay(int32_t x, int32_t y, bool pressed) { return false; }

    /**
        Handles mouse wheel scrolling in overlays.
        \param  x x-coordinate (relative to the left top corner of the widget)
        \param  y y-coordinate (relative to the left top corner of the widget)
        \param  up  true = mouse wheel up, false = mouse wheel down
        \return true = the mouse wheel scrolling was processed by the widget, false = mouse wheel scrolling was not
       processed by the widget
    */
    virtual bool handleMouseWheelOverlay(int32_t x, int32_t y, bool up) { return false; }

    /**
        Handles a key stroke in overlays.
        \param  key the key that was pressed or released.
        \return true = key stroke was processed by the widget, false = key stroke was not processed by the widget
    */
    virtual bool handleKeyPressOverlay(const SDL_KeyboardEvent& key) { return false; }

    /**
        Handles a text input event in overlays.
        \param  textInput the text input that was performed.
        \return true = text input was processed by the widget, false = text input was not processed by the widget
    */
    virtual bool handleTextInputOverlay(const SDL_TextInputEvent& textInput) { return false; }

    /**
        Draws this widget to screen. This method is called before drawOverlay().
        \param  position    Position to draw the widget to
    */
    virtual void draw(Point position) { }

    /**
        This method draws the parts of this widget that must be drawn after all the other
        widgets are drawn (e.g. tooltips). This method is called after draw().
        \param  position    Position to draw the widget to
    */
    virtual void drawOverlay(Point position) { }

    /**
        This method is called if a child widget is destroyed (see Widget::~Widget).
        \param  pChildWidget    widget to remove
    */
    virtual void removeChildWidget(Widget* pChildWidget) { }

    /**
        This method is called if the widget is removed from a container. If
        the widget was created via a named constructor (static create method) then this
        method automatically frees the memory of this object.
    */
    virtual void destroy();

    /**
        Sets the function that should be called when this widget gains focus.
        \param  pOnGainFocus    A function to call on focus gain
    */
    void setOnGainFocus(std::function<void()> pOnGainFocus) { this->pOnGainFocus_ = std::move(pOnGainFocus); }

    /**
        Sets the function that should be called when this widget loses focus.
        \param  pOnLostFocus    A function to call on focus loss
    */
    void setOnLostFocus(std::function<void()> pOnLostFocus) { this->pOnLostFocus_ = std::move(pOnLostFocus); }

    /**
        This static method creates a dynamic Widget object.
        The idea behind this method is to simply create a new spacer on the fly and
        add it to a container. If the container gets destroyed also this object will be freed.
        \param args ctor arguments
        \return The newly created object (will be automatically destroyed when it's parent widget is destroyed)
    */
    template<std::derived_from<Widget> TWidget, typename... Args>
    static std::unique_ptr<TWidget> create(Args&&... args) {

        auto w = std::make_unique<TWidget>(std::forward<Args>(args)...);

        w->pAllocated_ = true;

        return w;
    }

protected:
    /**
        This method is called by containers to enable a widget or disable a widget explicitly.
        It is the responsibility of the container to take care that there is only one active
        widget.
        \param  bActive true = activate this widget, false = deactivate this widget
    */
    virtual void setActive(bool bActive);

    /**
        This method enables or disables resizing of a widget. By default resizing in both directions is not allowed
        and this method should be called by all constructors of a concrete widget that is resizable.
        \param  bResizeX    true = resizing in x direction allowed, false = resizing in x direction not allowed
        \param  bResizeY    true = resizing in y direction allowed, false = resizing in y direction not allowed
    */
    virtual void enableResizing(bool bResizeX, bool bResizeY) {
        resizeX_ = bResizeX;
        resizeY_ = bResizeY;
    }

    /**
        This method activates or deactivates one specific widget in this widget/container. It is mainly used
        by widgets that are activated/deactivated and have to inform their parent container.
        \param  active  true = activate, false = deactivate
        \param  childWidget the widget to activate/deactivate
    */
    virtual void setActiveChildWidget(bool active, Widget* childWidget) { }

    /**
        This method is called whenever the textures of this widget are needed, e.g. before drawing. This method
        should be overwritten by subclasses if they like to defer texture creation as long as possible.
        This method should first check whether a renewal of the textures is necessary.
    */
    virtual void updateTextures() { }

    /**
        This method frees all textures that are used by this widget
    */
    virtual void invalidateTextures() { }

    /// If this widget is created via a named constructor (static create method) then bAllocated is true
    bool pAllocated_{};

private:
    bool visible_{true}; ///< Is this widget visible?
    bool enabled_{true}; ///< Is this widget enabled?
    bool active_{};      ///< Is this widget active?
    bool resizeX_{};     ///< Is this widget resizable in X direction?
    bool resizeY_{};     ///< Is this widget resizable in Y direction?
    Point size_;         ///< The size of this widget
    Widget* parent_{};   ///< The parent widget

    std::function<void()> pOnGainFocus_; ///< function that is called when this widget gains focus
    std::function<void()> pOnLostFocus_; ///< function that is called when this widget loses focus
};

#endif // WIDGET_H
