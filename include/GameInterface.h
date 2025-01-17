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

#ifndef GAMEINTERFACE_H
#define GAMEINTERFACE_H

#include <GUI/HBox.h>
#include <GUI/PictureButton.h>
#include <GUI/PictureLabel.h>
#include <GUI/StaticContainer.h>
#include <GUI/Window.h>
#include <GUI/dune/ChatManager.h>
#include <GUI/dune/NewsTicker.h>

#include <RadarView.h>

#include <memory>

#include "ObjectBase.h"

class ObjectInterface;

/// This class represents the in-game interface.
class GameInterface final : public Window {
    using parent = Window;

public:
    /// default constructor
    GameInterface(const GameContext& context);

    /// destructor
    ~GameInterface() override;

    /**
        Draws this window to screen. This method should be called every frame.
        \param  position    Position to draw the window to. The position of the window is added to this.
    */
    void draw(Point position) override;

    /**
        This method resizes the window to width and height.
        \param  width   the new width of this widget
        \param  height  the new height of this widget
    */
    void resize(uint32_t width, uint32_t height) override;

    using parent::resize;

    /**
        Checks whether the news ticker currently shows a message
        \return true if a message is shown, false otherwise
    */
    virtual bool newsTickerHasMessage() { return newsticker.hasMessage(); }

    /**
        This method adds a message to the news ticker
        \param  text    the message to add
    */
    virtual void addToNewsTicker(std::string text) { newsticker.addMessage(std::move(text)); }

    /**
        This method adds a urgent message to the news ticker
        \param  text    the urgent message to add
    */
    virtual void addUrgentMessageToNewsTicker(std::string text) { newsticker.addUrgentMessage(std::move(text)); }

    /**
        Returns the radar view
        \return the radar view
    */
    RadarView& getRadarView() { return radarView; }

    /**
        Returns the chat manager
        \return the chat manager
    */
    ChatManager& getChatManager() { return chatManager; }

    /**
        This method updates the object interface
    */
    virtual void updateObjectInterface();

private:
    void removeOldContainer();

    void draw_indicator(SDL_Renderer* renderer, const SDL_FRect& dest, float percent, uint32_t color);

    // windowWidget needs to be above pObjectContainer so that pObjectContainer's dtor
    // runs before windowWidget's dtor.
    StaticContainer windowWidget; ///< The main widget of this interface

    std::unique_ptr<ObjectInterface>
        pObjectContainer;       ///< The container holding information about the currently selected unit/structure
    uint32_t objectID{NONE_ID}; ///< The id of the currently selected object

    HBox topBarHBox; ///< The container for the top bar containing news ticker, options button and mentat button
    NewsTicker
        newsticker; ///< The news ticker showing news on the game (e.g. new starport prices, harvester fill level, etc.)
    PictureButton optionsButton; ///< Button for accessing the in-game menu
    PictureButton mentatButton;  ///< Button for accessing the mentat menu
    PictureLabel topBar;         ///< The background of the top bar

    PictureLabel sideBar; ///< The background of the side bar

    RadarView radarView; ///< This is the minimap/radar in the side bar

    ChatManager chatManager; ///< Manages chat shown overlaid with the main map

    std::vector<SDL_FRect> render_rects_;

    const GameContext context_;
};
#endif // GAMEINTERFACE_H
