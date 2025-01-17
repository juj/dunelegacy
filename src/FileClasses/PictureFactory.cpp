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

#include <FileClasses/PictureFactory.h>

#include <globals.h>

#include <config.h>

#include <FileClasses/Cpsfile.h>
#include <FileClasses/FileManager.h>
#include <FileClasses/Font.h>
#include <FileClasses/FontManager.h>
#include <FileClasses/LoadSavePNG.h>
#include <FileClasses/TextManager.h>
#include <FileClasses/Wsafile.h>

#include <misc/Scaler.h>
#include <misc/draw_util.h>
#include <misc/exceptions.h>

#include <cstddef>
#include <memory>

PictureFactory::PictureFactory() {
    const auto* const file_manager = dune::globals::pFileManager.get();

    const auto ScreenPic = LoadCPS_RW(file_manager->openFile("SCREEN.CPS").get());
    if (ScreenPic == nullptr) {
        THROW(std::runtime_error, "PictureFactory::PictureFactory(): Cannot read SCREEN.CPS!");
    }

    const auto FamePic = LoadCPS_RW(file_manager->openFile("FAME.CPS").get());
    if (FamePic == nullptr) {
        THROW(std::runtime_error, "PictureFactory::PictureFactory(): Cannot read FAME.CPS!");
    }

    const auto ChoamPic = LoadCPS_RW(file_manager->openFile("CHOAM.CPS").get());
    if (ChoamPic == nullptr) {
        THROW(std::runtime_error, "PictureFactory::PictureFactory(): Cannot read CHOAM.CPS!");
    }

    creditsBorder = getSubPicture(ScreenPic.get(), 257, 2, 63, 13);

    backgroundTile = createBackgroundTile(FamePic.get());

    const auto& video = dune::globals::settings.video;

    background = createBackground(video.width, video.height);

    // decoration border
    decorationBorder.ball    = getSubPicture(ScreenPic.get(), 241, 124, 12, 11);
    decorationBorder.vspacer = getSubPicture(ScreenPic.get(), 241, 118, 12, 5);
    decorationBorder.hspacer = rotateSurfaceRight(decorationBorder.vspacer.get());
    decorationBorder.vborder = getSubPicture(ScreenPic.get(), 241, 71, 12, 13);
    decorationBorder.hborder = rotateSurfaceRight(decorationBorder.vborder.get());

    // simple Frame
    auto& simple_frame = frame[static_cast<int>(DecorationFrame::SimpleFrame)];

    simple_frame.leftUpperCorner = getSubPicture(ChoamPic.get(), 120, 17, 8, 8);
    putPixel(simple_frame.leftUpperCorner.get(), 7, 7, 0);
    putPixel(simple_frame.leftUpperCorner.get(), 6, 7, 0);
    putPixel(simple_frame.leftUpperCorner.get(), 7, 6, 0);

    simple_frame.rightUpperCorner = getSubPicture(ChoamPic.get(), 312, 17, 8, 8);
    putPixel(simple_frame.rightUpperCorner.get(), 0, 7, 0);
    putPixel(simple_frame.rightUpperCorner.get(), 0, 6, 0);
    putPixel(simple_frame.rightUpperCorner.get(), 1, 7, 0);

    simple_frame.leftLowerCorner = getSubPicture(ChoamPic.get(), 120, 31, 8, 8);
    putPixel(simple_frame.leftLowerCorner.get(), 7, 0, 0);
    putPixel(simple_frame.leftLowerCorner.get(), 6, 0, 0);
    putPixel(simple_frame.leftLowerCorner.get(), 7, 1, 0);

    simple_frame.rightLowerCorner = getSubPicture(ChoamPic.get(), 312, 31, 8, 8);
    putPixel(simple_frame.rightLowerCorner.get(), 0, 0, 0);
    putPixel(simple_frame.rightLowerCorner.get(), 1, 0, 0);
    putPixel(simple_frame.rightLowerCorner.get(), 0, 1, 0);

    simple_frame.hborder = getSubPicture(ChoamPic.get(), 128, 17, 1, 4);
    simple_frame.vborder = getSubPicture(ChoamPic.get(), 120, 25, 4, 1);

    // Decoration Frame 1
    auto& decoration_frame = frame[static_cast<int>(DecorationFrame::DecorationFrame1)];

    decoration_frame.leftUpperCorner = getSubPicture(ChoamPic.get(), 2, 57, 11, 12);
    putPixel(decoration_frame.leftUpperCorner.get(), 10, 11, 0);
    putPixel(decoration_frame.leftUpperCorner.get(), 9, 11, 0);
    putPixel(decoration_frame.leftUpperCorner.get(), 10, 10, 0);

    decoration_frame.rightUpperCorner = getSubPicture(ChoamPic.get(), 44, 57, 11, 12);
    putPixel(decoration_frame.rightUpperCorner.get(), 0, 11, 0);
    putPixel(decoration_frame.rightUpperCorner.get(), 0, 10, 0);
    putPixel(decoration_frame.rightUpperCorner.get(), 1, 11, 0);

    decoration_frame.leftLowerCorner = getSubPicture(ChoamPic.get(), 2, 132, 11, 11);
    putPixel(decoration_frame.leftLowerCorner.get(), 10, 0, 0);
    putPixel(decoration_frame.leftLowerCorner.get(), 9, 0, 0);
    putPixel(decoration_frame.leftLowerCorner.get(), 10, 1, 0);

    decoration_frame.rightLowerCorner = getSubPicture(ChoamPic.get(), 44, 132, 11, 11);
    putPixel(decoration_frame.rightLowerCorner.get(), 0, 0, 0);
    putPixel(decoration_frame.rightLowerCorner.get(), 1, 0, 0);
    putPixel(decoration_frame.rightLowerCorner.get(), 0, 1, 0);

    decoration_frame.hborder = getSubPicture(ChoamPic.get(), 13, 57, 1, 4);
    decoration_frame.vborder = getSubPicture(ChoamPic.get(), 2, 69, 4, 1);

    // Decoration Frame 2
    auto& decoration_frame2 = frame[static_cast<int>(DecorationFrame::DecorationFrame2)];

    decoration_frame2.leftUpperCorner = getSubPicture(ChoamPic.get(), 121, 41, 9, 9);
    drawHLine(decoration_frame2.leftUpperCorner.get(), 6, 6, 8, 0);
    drawHLine(decoration_frame2.leftUpperCorner.get(), 6, 7, 8, 0);
    drawHLine(decoration_frame2.leftUpperCorner.get(), 6, 8, 8, 0);

    decoration_frame2.rightUpperCorner = getSubPicture(ChoamPic.get(), 309, 41, 10, 9);
    drawHLine(decoration_frame2.rightUpperCorner.get(), 0, 6, 3, 0);
    drawHLine(decoration_frame2.rightUpperCorner.get(), 0, 7, 3, 0);
    drawHLine(decoration_frame2.rightUpperCorner.get(), 0, 8, 3, 0);

    decoration_frame2.leftLowerCorner = getSubPicture(ChoamPic.get(), 121, 157, 9, 10);
    drawHLine(decoration_frame2.leftLowerCorner.get(), 6, 0, 8, 0);
    drawHLine(decoration_frame2.leftLowerCorner.get(), 6, 1, 8, 0);
    drawHLine(decoration_frame2.leftLowerCorner.get(), 6, 2, 8, 0);
    drawHLine(decoration_frame2.leftLowerCorner.get(), 7, 3, 8, 0);

    decoration_frame2.rightLowerCorner = getSubPicture(ChoamPic.get(), 309, 158, 10, 9);
    drawHLine(decoration_frame2.rightLowerCorner.get(), 0, 0, 3, 0);
    drawHLine(decoration_frame2.rightLowerCorner.get(), 0, 1, 3, 0);
    drawHLine(decoration_frame2.rightLowerCorner.get(), 0, 2, 3, 0);

    decoration_frame2.hborder = getSubPicture(ChoamPic.get(), 133, 41, 1, 4);
    decoration_frame2.vborder = getSubPicture(ChoamPic.get(), 121, 51, 4, 1);

    for (auto& f : frame) {
        SDL_SetColorKey(f.leftUpperCorner.get(), SDL_TRUE, 0);
        SDL_SetColorKey(f.leftLowerCorner.get(), SDL_TRUE, 0);
        SDL_SetColorKey(f.rightUpperCorner.get(), SDL_TRUE, 0);
        SDL_SetColorKey(f.rightLowerCorner.get(), SDL_TRUE, 0);
        SDL_SetColorKey(f.hborder.get(), SDL_TRUE, 0);
        SDL_SetColorKey(f.vborder.get(), SDL_TRUE, 0);
    }

    // House Logos
    harkonnenLogo = getSubPicture(FamePic.get(), 10, 137, 53, 54);
    atreidesLogo  = getSubPicture(FamePic.get(), 66, 137, 53, 54);
    ordosLogo     = getSubPicture(FamePic.get(), 122, 137, 53, 54);

    gameStatsBackground = copySurface(background.get());

    {
        const auto FamePic2 = Scaler::defaultDoubleSurface(FamePic.get());
        const auto pSurface = getSubPicture(FamePic2.get(), 16, 160, 610, 74);
        SDL_Rect dest2      = calcDrawingRect(pSurface.get(), 16, 234);
        SDL_BlitSurface(pSurface.get(), nullptr, FamePic2.get(), &dest2);

        SDL_Rect dest3 = calcDrawingRect(pSurface.get(), 16, 234 + 74);
        SDL_BlitSurface(pSurface.get(), nullptr, FamePic2.get(), &dest3);

        SDL_Rect dest4 = calcAlignedDrawingRect(FamePic2.get(), gameStatsBackground.get());
        SDL_BlitSurface(FamePic2.get(), nullptr, gameStatsBackground.get(), &dest4);
    }

    messageBoxBorder = getSubPicture(ScreenPic.get(), 0, 17, 320, 22);

    if (file_manager->exists("MISC." + _("LanguageFileExtension"))) {
        mentatHouseChoiceQuestionSurface = Scaler::defaultDoubleSurface(
            LoadCPS_RW(file_manager->openFile("MISC." + _("LanguageFileExtension")).get()).get());
    } else {
        mentatHouseChoiceQuestionSurface =
            Scaler::defaultDoubleSurface(LoadCPS_RW(file_manager->openFile("MISC.CPS").get()).get());
    }

    // create builder list upper cap
    builderListUpperCap = sdl2::surface_ptr{SDL_CreateRGBSurface(0, 112, 21, 8, 0, 0, 0, 0)};
    if (builderListUpperCap == nullptr) {
        THROW(std::runtime_error, "PictureFactory::PictureFactory: Cannot create new Picture!");
    }
    dune::globals::palette.applyToSurface(builderListUpperCap.get());
    assert(builderListUpperCap->format->BitsPerPixel == 8);
    SDL_FillRect(builderListUpperCap.get(), nullptr, PALCOLOR_TRANSPARENT);

    {
        const auto builderListUpperCapLeft = getSubPicture(ChoamPic.get(), 64, 3, 42, 18);
        SDL_Rect dest5{0, 0, 42, 18};
        SDL_BlitSurface(builderListUpperCapLeft.get(), nullptr, builderListUpperCap.get(), &dest5);
    }
    {
        const auto builderListUpperCapMiddle = getSubPicture(ChoamPic.get(), 69, 3, 38, 13);
        SDL_Rect dest6{42, 0, 38, 13};
        SDL_BlitSurface(builderListUpperCapMiddle.get(), nullptr, builderListUpperCap.get(), &dest6);
    }
    {
        const auto builderListUpperCapRight = getSubPicture(ChoamPic.get(), 69, 3, 48, 21);
        SDL_Rect dest7{64, 0, 48, 21};
        SDL_BlitSurface(builderListUpperCapRight.get(), nullptr, builderListUpperCap.get(), &dest7);
    }
    replaceColor(builderListUpperCap.get(), 30, 0);
    SDL_SetColorKey(builderListUpperCap.get(), SDL_TRUE, 0);

    // create builder list lower cap
    builderListLowerCap = sdl2::surface_ptr{SDL_CreateRGBSurface(0, 112, 17, 8, 0, 0, 0, 0)};
    if (builderListLowerCap == nullptr) {
        THROW(std::runtime_error, "PictureFactory::PictureFactory: Cannot create new Picture!");
    }

    dune::globals::palette.applyToSurface(builderListLowerCap.get());
    assert(builderListLowerCap->format->BitsPerPixel == 8);
    SDL_FillRect(builderListLowerCap.get(), nullptr, PALCOLOR_TRANSPARENT);

    {
        const auto builderListLowerCapLeft = getSubPicture(ChoamPic.get(), 64, 149, 44, 17);
        SDL_Rect dest8{0, 0, 44, 17};
        SDL_BlitSurface(builderListLowerCapLeft.get(), nullptr, builderListLowerCap.get(), &dest8);
    }
    {
        const auto builderListLowerCapMiddle = getSubPicture(ChoamPic.get(), 68, 152, 40, 14);
        SDL_Rect dest9{44, 3, 40, 14};
        SDL_BlitSurface(builderListLowerCapMiddle.get(), nullptr, builderListLowerCap.get(), &dest9);
    }
    {
        const auto builderListLowerCapRight = getSubPicture(ChoamPic.get(), 68, 149, 48, 17);
        SDL_Rect dest10{64, 0, 48, 17};
        SDL_BlitSurface(builderListLowerCapRight.get(), nullptr, builderListLowerCap.get(), &dest10);
    }

    replaceColor(builderListLowerCap.get(), 30, 0);
    SDL_SetColorKey(builderListLowerCap.get(), SDL_TRUE, 0);
}

PictureFactory::~PictureFactory() = default;

sdl2::surface_ptr PictureFactory::createTopBar() const {
    auto topBar = getSubPicture(background.get(), 0, 0, dune::globals::settings.video.width - SIDEBARWIDTH, 32 + 12);

    const SDL_Rect dest1{0, 31, getWidth(topBar.get()), 12};
    SDL_FillRect(topBar.get(), &dest1, COLOR_TRANSPARENT);

    SDL_Rect dest2 = calcDrawingRect(decorationBorder.hborder.get(), 0, 32);
    for (dest2.x = 0; dest2.x < topBar->w; dest2.x += decorationBorder.hborder.get()->w) {
        SDL_Rect tmpDest = dest2;
        SDL_BlitSurface(decorationBorder.hborder.get(), nullptr, topBar.get(), &tmpDest);
    }

    drawVLine(topBar.get(), topBar->w - 7, 32, topBar->h - 1, 96);

    SDL_Rect dest3{getWidth(topBar.get()) - 6, getHeight(topBar.get()) - 12, 12, 5};
    SDL_BlitSurface(decorationBorder.hspacer.get(), nullptr, topBar.get(), &dest3);

    drawVLine(topBar.get(), topBar->w - 1, 0, topBar->h - 1, 0);

    return topBar;
}

sdl2::surface_ptr PictureFactory::createSideBar(bool bEditor) const {
    auto sideBar = getSubPicture(background.get(), 0, 0, SIDEBARWIDTH, dune::globals::settings.video.height);

    const SDL_Rect dest1{0, 0, 13, getHeight(sideBar.get())};
    SDL_FillRect(sideBar.get(), &dest1, COLOR_TRANSPARENT);

    SDL_Rect dest2 = calcDrawingRect(decorationBorder.vborder.get(), 0, 0);
    for (dest2.y = 0; dest2.y < sideBar->h; dest2.y += decorationBorder.vborder.get()->h) {
        SDL_Rect tmpDest = dest2;
        SDL_BlitSurface(decorationBorder.vborder.get(), nullptr, sideBar.get(), &tmpDest);
    }

    SDL_Rect dest3 = calcDrawingRect(decorationBorder.vspacer.get(), 0, 30, HAlign::Left, VAlign::Bottom);
    SDL_BlitSurface(decorationBorder.vspacer.get(), nullptr, sideBar.get(), &dest3);

    drawHLine(sideBar.get(), 0, 32 - decorationBorder.vspacer.get()->h - 2, decorationBorder.vspacer.get()->w - 1, 96);
    drawHLine(sideBar.get(), 0, 31, decorationBorder.vspacer.get()->w - 1, 0);

    SDL_Rect dest4 = calcDrawingRect(decorationBorder.ball.get(), 0, 32);
    SDL_BlitSurface(decorationBorder.ball.get(), nullptr, sideBar.get(), &dest4);

    drawHLine(sideBar.get(), 0, 43, decorationBorder.vspacer.get()->w - 1, 0);
    SDL_Rect dest5 = calcDrawingRect(decorationBorder.vspacer.get(), 0, 44);
    SDL_BlitSurface(decorationBorder.vspacer.get(), nullptr, sideBar.get(), &dest5);
    drawHLine(sideBar.get(), 0, 44 + decorationBorder.vspacer.get()->h, decorationBorder.vspacer.get()->w - 1, 96);

    const SDL_Rect dest6{13, 0, getWidth(sideBar.get()) - 1, 132};
    SDL_FillRect(sideBar.get(), &dest6, COLOR_TRANSPARENT);
    drawRect(sideBar.get(), 13, 1, sideBar->w - 2, 130, 115);

    SDL_Rect dest7 = calcDrawingRect(decorationBorder.vspacer.get(), 0, 130, HAlign::Left, VAlign::Bottom);
    SDL_BlitSurface(decorationBorder.vspacer.get(), nullptr, sideBar.get(), &dest7);

    drawHLine(sideBar.get(), 0, 132 - decorationBorder.vspacer.get()->h - 2, decorationBorder.vspacer.get()->w - 1, 96);
    drawHLine(sideBar.get(), 0, 131, decorationBorder.vspacer.get()->w - 1, 0);

    SDL_Rect dest8 = calcDrawingRect(decorationBorder.ball.get(), 0, 132);
    SDL_BlitSurface(decorationBorder.ball.get(), nullptr, sideBar.get(), &dest8);

    drawHLine(sideBar.get(), 0, 143, decorationBorder.vspacer.get()->w - 1, 0);
    SDL_Rect dest9 = calcDrawingRect(decorationBorder.vspacer.get(), 0, 144);
    SDL_BlitSurface(decorationBorder.vspacer.get(), nullptr, sideBar.get(), &dest9);
    drawHLine(sideBar.get(), 0, 144 + decorationBorder.vspacer.get()->h, decorationBorder.vspacer.get()->w - 1, 96);

    SDL_Rect dest10 = calcDrawingRect(decorationBorder.hspacer.get(), 13, 132);
    SDL_BlitSurface(decorationBorder.hspacer.get(), nullptr, sideBar.get(), &dest10);

    drawVLine(sideBar.get(), 18, 132, 132 + decorationBorder.hspacer.get()->h - 1, 96);
    drawHLine(sideBar.get(), 13, 132 + decorationBorder.hspacer.get()->h, sideBar->w - 1, 0);

    SDL_Rect dest11 = calcDrawingRect(decorationBorder.hborder.get(), 0, 132);
    for (dest11.x = 19; dest11.x < sideBar->w; dest11.x += decorationBorder.hborder.get()->w) {
        SDL_Rect tmpDest = dest11;
        SDL_BlitSurface(decorationBorder.hborder.get(), nullptr, sideBar.get(), &tmpDest);
    }

    if (bEditor) {
        SDL_Rect dest12 = calcDrawingRect(decorationBorder.vspacer.get(), 0, getHeight(sideBar.get()) - 32 - 14,
                                          HAlign::Left, VAlign::Bottom);
        SDL_BlitSurface(decorationBorder.vspacer.get(), nullptr, sideBar.get(), &dest12);

        drawHLine(sideBar.get(), 0, sideBar->h - 32 - 12 - decorationBorder.vspacer.get()->h - 2,
                  decorationBorder.vspacer.get()->w - 1, 96);
        drawHLine(sideBar.get(), 0, sideBar->h - 32 - 12 - 1, decorationBorder.vspacer.get()->w - 1, 0);

        SDL_Rect dest13 = calcDrawingRect(decorationBorder.ball.get(), 0, getHeight(sideBar.get()) - 32 - 12);
        SDL_BlitSurface(decorationBorder.ball.get(), nullptr, sideBar.get(), &dest13);

        drawHLine(sideBar.get(), 0, sideBar->h - 32 - 1, decorationBorder.vspacer.get()->w - 1, 0);
        SDL_Rect dest14 = calcDrawingRect(decorationBorder.vspacer.get(), 0, getHeight(sideBar.get()) - 32);
        SDL_BlitSurface(decorationBorder.vspacer.get(), nullptr, sideBar.get(), &dest14);
        drawHLine(sideBar.get(), 0, sideBar->h - 32 + decorationBorder.vspacer.get()->h,
                  decorationBorder.vspacer.get()->w - 1, 96);
    } else {
        SDL_Rect dest15 = calcDrawingRect(creditsBorder.get(), 46, 132);
        SDL_BlitSurface(creditsBorder.get(), nullptr, sideBar.get(), &dest15);
    }

    return sideBar;
}

sdl2::surface_ptr PictureFactory::createBottomBar() const {
    auto BottomBar = getSubPicture(background.get(), 0, 0, dune::globals::settings.video.width - SIDEBARWIDTH, 32 + 12);
    const SDL_Rect dest1{0, 0, getWidth(BottomBar.get()), 13};
    SDL_FillRect(BottomBar.get(), &dest1, COLOR_TRANSPARENT);

    SDL_Rect dest2 = calcDrawingRect(decorationBorder.hborder.get(), 0, 0);
    for (dest2.x = 0; dest2.x < BottomBar->w; dest2.x += decorationBorder.hborder.get()->w) {
        SDL_Rect tmpDest = dest2;
        SDL_BlitSurface(decorationBorder.hborder.get(), nullptr, BottomBar.get(), &tmpDest);
    }

    drawVLine(BottomBar.get(), BottomBar->w - 7, 0, 11, 96);

    SDL_Rect dest3{getWidth(BottomBar.get()) - 6, 0, 12, 5};
    SDL_BlitSurface(decorationBorder.hspacer.get(), nullptr, BottomBar.get(), &dest3);

    drawVLine(BottomBar.get(), BottomBar->w - 1, 0, BottomBar->h - 1, 0);

    return BottomBar;
}

sdl2::surface_ptr PictureFactory::createPlacingGrid(int size, int color) {
    sdl2::surface_ptr placingGrid{SDL_CreateRGBSurface(0, size, size, 8, 0, 0, 0, 0)};
    if (placingGrid == nullptr) {
        THROW(sdl_error, "Cannot create new surface: %s!", SDL_GetError());
    }

    dune::globals::palette.applyToSurface(placingGrid.get());
    sdl2::surface_lock lock{placingGrid.get()};

    auto* const pixels = static_cast<uint8_t*>(placingGrid->pixels);

    for (auto y = 0; y < size; y++) {
        auto* const RESTRICT out = pixels + static_cast<ptrdiff_t>(y) * placingGrid->pitch;
        for (auto x = 0; x < size; x++) {
            if (x % 2 == y % 2) {
                out[x] = static_cast<uint8_t>(color);
            } else {
                out[x] = 0;
            }
        }
    }

    SDL_SetColorKey(placingGrid.get(), SDL_TRUE, 0);

    return placingGrid;
}

void PictureFactory::drawFrame(SDL_Surface* Pic, DecorationFrame decorationType, SDL_Rect* dest) const {
    if (Pic == nullptr)
        return;

    if (decorationType >= DecorationFrame::NUM_DECORATIONFRAMES)
        return;

    const auto type = static_cast<int>(decorationType);

    if (type < 0)
        return;

    SDL_Rect tmp;
    if (dest == nullptr) {
        tmp.x = 0;
        tmp.y = 0;
        tmp.w = Pic->w;
        tmp.h = Pic->h;
        dest  = &tmp;
    }

    // corners
    SDL_Rect dest1 = calcDrawingRect(frame[type].leftUpperCorner.get(), dest->x, dest->y);
    SDL_BlitSurface(frame[type].leftUpperCorner.get(), nullptr, Pic, &dest1);

    SDL_Rect dest2 =
        calcDrawingRect(frame[type].rightUpperCorner.get(), dest->w - 1, dest->y, HAlign::Right, VAlign::Top);
    SDL_BlitSurface(frame[type].rightUpperCorner.get(), nullptr, Pic, &dest2);

    SDL_Rect dest3 =
        calcDrawingRect(frame[type].leftLowerCorner.get(), dest->x, dest->h - 1, HAlign::Left, VAlign::Bottom);
    SDL_BlitSurface(frame[type].leftLowerCorner.get(), nullptr, Pic, &dest3);

    SDL_Rect dest4 =
        calcDrawingRect(frame[type].rightLowerCorner.get(), dest->w - 1, dest->h - 1, HAlign::Right, VAlign::Bottom);
    SDL_BlitSurface(frame[type].rightLowerCorner.get(), nullptr, Pic, &dest4);

    // hborders
    SDL_Rect dest5 = calcDrawingRect(frame[type].hborder.get(), dest->x, dest->y);
    for (dest5.x = frame[type].leftUpperCorner.get()->w + dest->x;
         dest5.x <= dest->w - frame[type].rightUpperCorner.get()->w - 1; dest5.x += frame[type].hborder.get()->w) {
        SDL_Rect tmpDest = dest5;
        SDL_BlitSurface(frame[type].hborder.get(), nullptr, Pic, &tmpDest);
    }

    SDL_Rect dest6 = calcDrawingRect(frame[type].hborder.get(), dest->x, dest->h - 1, HAlign::Left, VAlign::Bottom);
    for (dest6.x = frame[type].leftLowerCorner.get()->w + dest->x;
         dest6.x <= dest->w - frame[type].rightLowerCorner.get()->w - 1; dest6.x += frame[type].hborder.get()->w) {
        SDL_Rect tmpDest = dest6;
        SDL_BlitSurface(frame[type].hborder.get(), nullptr, Pic, &tmpDest);
    }

    // vborders
    SDL_Rect dest7 = calcDrawingRect(frame[type].vborder.get(), dest->x, dest->y);
    for (dest7.y = frame[type].leftUpperCorner.get()->h + dest->y;
         dest7.y <= dest->h - frame[type].leftLowerCorner.get()->h - 1; dest7.y += frame[type].vborder.get()->h) {
        SDL_Rect tmpDest = dest7;
        SDL_BlitSurface(frame[type].vborder.get(), nullptr, Pic, &tmpDest);
    }

    SDL_Rect dest8 = calcDrawingRect(frame[type].vborder.get(), dest->w - 1, dest->y, HAlign::Right, VAlign::Top);
    for (dest8.y = frame[type].rightUpperCorner.get()->h + dest->y;
         dest8.y <= dest->h - frame[type].rightLowerCorner.get()->h - 1; dest8.y += frame[type].vborder.get()->h) {
        SDL_Rect tmpDest = dest8;
        SDL_BlitSurface(frame[type].vborder.get(), nullptr, Pic, &tmpDest);
    }
}

sdl2::surface_ptr
PictureFactory::createFrame(DecorationFrame decorationType, int width, int height, bool UseBackground) {
    sdl2::surface_ptr Pic;
    if (UseBackground) {
        Pic = getSubPicture(background.get(), 0, 0, width, height);
    } else {
        Pic = sdl2::surface_ptr{SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0)};
        if (Pic == nullptr) {
            THROW(sdl_error, "Cannot create new surface: %s!", SDL_GetError());
        }
        dune::globals::palette.applyToSurface(Pic.get());
        SDL_SetColorKey(Pic.get(), SDL_TRUE, 0);
    }

    drawFrame(Pic.get(), decorationType);

    return Pic;
}

sdl2::surface_ptr PictureFactory::createBackground() const {
    return copySurface(background.get());
}

sdl2::surface_ptr PictureFactory::createBackgroundTile() const {
    return copySurface(backgroundTile.get());
}

PictureFactory::DecorationBorderType PictureFactory::createDecorationBorder() const {
    const auto& d = decorationBorder;

    return {copySurface(d.ball.get()), copySurface(d.hspacer.get()), copySurface(d.vspacer.get()),
            copySurface(d.hborder.get()), copySurface(d.vborder.get())};
}

PictureFactory::BorderStyle PictureFactory::createBorderStyle(DecorationFrame type) const {
    const auto type_int = static_cast<int>(type);

    if (type_int < 0 || type >= DecorationFrame::NUM_DECORATIONFRAMES)
        THROW(std::invalid_argument, "PictureFactory::PictureFactory: Decoration type out-of-range (%d)!", type_int);

    const auto& f = frame[type_int];

    return {copySurface(f.leftUpperCorner.get()), copySurface(f.rightUpperCorner.get()),
            copySurface(f.leftLowerCorner.get()), copySurface(f.rightLowerCorner.get()),
            copySurface(f.hborder.get()),         copySurface(f.vborder.get())};
}

sdl2::surface_ptr PictureFactory::createBackgroundTile(SDL_Surface* fame_pic) const {
    const auto PatternNormal    = getSubPicture(fame_pic, 0, 1, 63, 67);
    const auto PatternHFlipped  = flipHSurface(getSubPicture(fame_pic, 0, 1, 63, 67).get());
    const auto PatternVFlipped  = flipVSurface(getSubPicture(fame_pic, 0, 1, 63, 67).get());
    const auto PatternHVFlipped = flipHSurface(flipVSurface(getSubPicture(fame_pic, 0, 1, 63, 67).get()).get());

    const auto width  = 2 * PatternNormal->w;
    const auto height = 2 * PatternNormal->h;

    auto surface = sdl2::surface_ptr{SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0)};
    if (surface == nullptr) {
        THROW(std::runtime_error, "PictureFactory::PictureFactory: Cannot create new background Picture!");
    }
    dune::globals::palette.applyToSurface(surface.get());

    SDL_Rect dest_normal{0, 0, PatternNormal->w, PatternNormal->w};
    SDL_BlitSurface(PatternNormal.get(), nullptr, surface.get(), &dest_normal);

    SDL_Rect dest_flipH{PatternNormal->w, 0, PatternHFlipped->w, PatternHFlipped->w};
    SDL_BlitSurface(PatternHFlipped.get(), nullptr, surface.get(), &dest_flipH);

    SDL_Rect dest_flipV{0, PatternNormal->h, PatternVFlipped->w, PatternVFlipped->w};
    SDL_BlitSurface(PatternVFlipped.get(), nullptr, surface.get(), &dest_flipV);

    SDL_Rect dest_flipHV{PatternNormal->w, PatternNormal->h, PatternHVFlipped->w, PatternHVFlipped->w};
    SDL_BlitSurface(PatternHVFlipped.get(), nullptr, surface.get(), &dest_flipHV);

    return surface;
}

sdl2::surface_ptr PictureFactory::createBackground(const int width, const int height) const {
    return createTiledSurface(backgroundTile.get(), width, height);
}

sdl2::surface_ptr PictureFactory::createGameStatsBackground(HOUSETYPE House) const {
    auto pSurface = copySurface(gameStatsBackground.get());

    sdl2::surface_ptr pLogo;
    switch (House) {
        case HOUSETYPE::HOUSE_HARKONNEN:
        case HOUSETYPE::HOUSE_SARDAUKAR: {
            pLogo = copySurface(harkonnenLogo.get());
        } break;

        case HOUSETYPE::HOUSE_ATREIDES:
        case HOUSETYPE::HOUSE_FREMEN: {
            pLogo = copySurface(atreidesLogo.get());
        } break;

        case HOUSETYPE::HOUSE_ORDOS:
        case HOUSETYPE::HOUSE_MERCENARY: {
            pLogo = copySurface(ordosLogo.get());
        } break;

        default:
            THROW(std::invalid_argument, "PictureFactory::createGameStatsBackground(): Unknown house %d!",
                  static_cast<int>(House));
    }

    pLogo = Scaler::defaultDoubleSurface(pLogo.get());

    auto dest1 = calcDrawingRect(pLogo.get(), getWidth(gameStatsBackground.get()) / 2 - 320 + 2,
                                 getHeight(gameStatsBackground.get()) / 2 - 200 + 16);
    SDL_BlitSurface(pLogo.get(), nullptr, pSurface.get(), &dest1);
    auto dest2 = calcDrawingRect(pLogo.get(), getWidth(gameStatsBackground.get()) / 2 + 320 - 3,
                                 getHeight(gameStatsBackground.get()) / 2 - 200 + 16, HAlign::Right, VAlign::Top);
    SDL_BlitSurface(pLogo.get(), nullptr, pSurface.get(), &dest2);

    return pSurface;
}

sdl2::surface_ptr PictureFactory::createMenu(int x, int y) const {
    auto Pic = getSubPicture(background.get(), 0, 0, x, y);

    SDL_Rect dest1{0, 0, getWidth(Pic.get()), 27};

    const auto grey = SDL2RGB(dune::globals::palette[PALCOLOR_GREY]);

    SDL_FillRect(Pic.get(), &dest1, grey);

    drawFrame(Pic.get(), DecorationFrame::SimpleFrame, &dest1);

    SDL_Rect dest2 = calcDrawingRect(Pic.get(), 0, dest1.h);
    drawFrame(Pic.get(), DecorationFrame::DecorationFrame1, &dest2);

    return Pic;
}

sdl2::surface_ptr PictureFactory::createMenu(SDL_Surface* CaptionPic, int y) const {
    if (CaptionPic == nullptr)
        return nullptr;

    auto Pic = getSubPicture(background.get(), 0, 0, CaptionPic->w, y);

    SDL_Rect dest1 = calcDrawingRect(CaptionPic, 0, 0);
    SDL_BlitSurface(CaptionPic, nullptr, Pic.get(), &dest1);

    drawFrame(Pic.get(), DecorationFrame::SimpleFrame, &dest1);

    SDL_Rect dest2 = calcDrawingRect(Pic.get(), 0, dest1.h);
    drawFrame(Pic.get(), DecorationFrame::DecorationFrame1, &dest2);

    return Pic;
}

sdl2::surface_ptr PictureFactory::createOptionsMenu() {
    auto tmp = LoadPNG_RW(dune::globals::pFileManager->openFile("UI_OptionsMenu.png").get());
    if (tmp == nullptr) {
        THROW(std::runtime_error, "Cannot load 'UI_OptionsMenu.png'!");
    }
    SDL_SetColorKey(tmp.get(), SDL_TRUE, 0);

    auto Pic = getSubPicture(background.get(), 0, 0, tmp->w, tmp->h);
    SDL_BlitSurface(tmp.get(), nullptr, Pic.get(), nullptr);

    tmp.reset();

    SDL_Rect dest1{0, 0, getWidth(Pic.get()), 27};
    drawFrame(Pic.get(), DecorationFrame::SimpleFrame, &dest1);

    SDL_Rect dest2 = calcDrawingRect(Pic.get(), 0, dest1.h);
    drawFrame(Pic.get(), DecorationFrame::DecorationFrame1, &dest2);

    return Pic;
}

sdl2::surface_ptr PictureFactory::createMessageBoxBorder() const {
    return copySurface(messageBoxBorder.get());
}

sdl2::surface_ptr PictureFactory::createHouseSelect(SDL_Surface* HouseChoice) const {
    auto Pic = copySurface(HouseChoice);

    const SDL_Rect dest{0, 50, getWidth(Pic.get()), getHeight(Pic.get()) - 50};
    assert(Pic->format->BitsPerPixel == 8);
    SDL_FillRect(Pic.get(), &dest, PALCOLOR_BLACK);

    drawFrame(Pic.get(), DecorationFrame::SimpleFrame, nullptr);

    return Pic;
}

sdl2::surface_ptr PictureFactory::createGreyHouseChoice(SDL_Surface* HouseChoice) {
    static constexpr auto index2greyindex = std::to_array<unsigned char>(
        {0,   0,   0,   13,  233, 127, 0,   131, 0,   0,   0,   0,   0,   13, 14,  15,  15,  127, 127, 14, 14,  14,
         14,  130, 24,  131, 131, 13,  13,  29,  30,  31,  0,   128, 128, 14, 14,  14,  14,  130, 130, 24, 24,  14,
         13,  13,  0,   29,  0,   0,   30,  0,   0,   183, 0,   0,   0,   0,  0,   0,   14,  30,  30,  30, 126, 0,
         0,   126, 128, 0,   0,   14,  14,  14,  0,   14,  14,  0,   0,   0,  14,  14,  0,   0,   0,   0,  0,   14,
         0,   0,   130, 13,  131, 13,  13,  29,  30,  30,  183, 175, 175, 0,  0,   0,   0,   0,   0,   0,  14,  233,
         14,  14,  14,  14,  14,  130, 24,  0,   0,   0,   131, 0,   122, 0,  24,  0,   0,   0,   0,   14, 130, 131,
         29,  133, 134, 127, 233, 14,  14,  24,  131, 13,  29,  183, 30,  30, 183, 183, 175, 175, 150, 0,  0,   0,
         0,   0,   0,   0,   0,   0,   24,  13,  29,  183, 175, 0,   0,   30, 0,   0,   13,  0,   0,   30, 174, 175,
         14,  24,  131, 13,  30,  183, 175, 122, 0,   0,   0,   0,   0,   0,  0,   0,   14,  24,  131, 13, 30,  122,
         175, 0,   0,   0,   0,   13,  0,   0,   0,   0,   14,  24,  131, 13, 30,  122, 175, 24,  14,  0,  0,   29,
         0,   0,   0,   0,   14,  24,  131, 13,  30,  122, 175, 0,   0,   0,  0,   0,   0,   0,   0,   0,  0,   0,
         0,   0,   13,  0,   30,  30,  183, 250, 250, 0,   0,   0,   0,   0});

    auto pic = copySurface(HouseChoice);

    const sdl2::surface_lock lock{pic.get()};

    auto* const pixels = static_cast<unsigned char*>(lock.pixels());
    const auto pitch   = static_cast<ptrdiff_t>(lock.pitch());

    for (auto y = 0; y < pic->h; ++y) {
        auto* const RESTRICT line = pixels + y * pitch;

        for (auto x = 0; x < pic->w; ++x) {
            auto& pixel = line[x];

            const auto inputIndex  = pixel;
            const auto outputIndex = index2greyindex[inputIndex];

            pixel = outputIndex;
        }
    }

    return pic;
}

sdl2::surface_ptr PictureFactory::createMapChoiceScreen(HOUSETYPE House) const {
    sdl2::surface_ptr pMapChoiceScreen{LoadCPS_RW(dune::globals::pFileManager->openFile("MAPMACH.CPS").get())};
    if (pMapChoiceScreen == nullptr) {
        THROW(std::runtime_error, "Cannot load 'MAPMACH.CPS'!");
    }

    auto LeftLogo  = calcDrawingRect(harkonnenLogo.get(), 2, 145);
    auto RightLogo = calcDrawingRect(harkonnenLogo.get(), 266, 145);

    switch (House) {
        case HOUSETYPE::HOUSE_HARKONNEN:
        case HOUSETYPE::HOUSE_SARDAUKAR: {
            SDL_BlitSurface(harkonnenLogo.get(), nullptr, pMapChoiceScreen.get(), &LeftLogo);
            SDL_BlitSurface(harkonnenLogo.get(), nullptr, pMapChoiceScreen.get(), &RightLogo);
        } break;

        case HOUSETYPE::HOUSE_ATREIDES:
        case HOUSETYPE::HOUSE_FREMEN: {
            SDL_BlitSurface(atreidesLogo.get(), nullptr, pMapChoiceScreen.get(), &LeftLogo);
            SDL_BlitSurface(atreidesLogo.get(), nullptr, pMapChoiceScreen.get(), &RightLogo);
        } break;

        case HOUSETYPE::HOUSE_ORDOS:
        case HOUSETYPE::HOUSE_MERCENARY: {
            SDL_BlitSurface(ordosLogo.get(), nullptr, pMapChoiceScreen.get(), &LeftLogo);
            SDL_BlitSurface(ordosLogo.get(), nullptr, pMapChoiceScreen.get(), &RightLogo);
        } break;

        default: {

        } break;
    }

    const auto& language = dune::globals::settings.general.language;

    if (language == "de") {
        auto tmp = getSubPicture(pMapChoiceScreen.get(), 8, 120, 303, 23);
        tmp      = copySurface(
                 tmp.get()); // Workaround: SDL2 leaks memory when blitting from A to B and afterwards from B to A
        SDL_Rect dest{8, 0, 303, 23};
        SDL_BlitSurface(tmp.get(), nullptr, pMapChoiceScreen.get(), &dest);
    } else if (language == "fr") {
        auto tmp = getSubPicture(pMapChoiceScreen.get(), 8, 96, 303, 23);
        tmp      = copySurface(
                 tmp.get()); // Workaround: SDL2 leaks memory when blitting from A to B and afterwards from B to A
        SDL_Rect dest{8, 0, 303, 23};
        SDL_BlitSurface(tmp.get(), nullptr, pMapChoiceScreen.get(), &dest);
    } else {
        // Nothing to do (use English)
    }

    // clear everything in the middle
    static constexpr SDL_Rect clearRect{8, 24, 304, 119};
    assert(pMapChoiceScreen->format->BitsPerPixel == 8);
    SDL_FillRect(pMapChoiceScreen.get(), &clearRect, PALCOLOR_TRANSPARENT);

    pMapChoiceScreen =
        Scaler::defaultDoubleSurface(mapSurfaceColorRange(pMapChoiceScreen.get(), PALCOLOR_HARKONNEN,
                                                          dune::globals::houseToPaletteIndex[static_cast<int>(House)])
                                         .get());
    auto pFullMapChoiceScreen = copySurface(background.get());

    SDL_Rect dest = calcAlignedDrawingRect(pMapChoiceScreen.get(), pFullMapChoiceScreen.get());
    SDL_BlitSurface(pMapChoiceScreen.get(), nullptr, pFullMapChoiceScreen.get(), &dest);

    return pFullMapChoiceScreen;
}

sdl2::surface_ptr PictureFactory::createMentatHouseChoiceQuestion(HOUSETYPE House, Palette& benePalette) const {
    sdl2::surface_ptr pSurface{SDL_CreateRGBSurface(0, 416 + 208, 48, 8, 0, 0, 0, 0)};
    if (pSurface == nullptr) {
        THROW(sdl_error, "Cannot create new surface: %s!", SDL_GetError());
    }

    benePalette.applyToSurface(pSurface.get());
    SDL_SetColorKey(pSurface.get(), SDL_TRUE, 0);

    const auto pQuestionPart1 = getSubPicture(mentatHouseChoiceQuestionSurface.get(), 0, 0, 416, 48);

    sdl2::surface_ptr pQuestionPart2 = nullptr;

    const auto* const file_manager = dune::globals::pFileManager.get();

    // clang-format off
    switch(House) {
        case HOUSETYPE::HOUSE_HARKONNEN:   pQuestionPart2 = getSubPicture(mentatHouseChoiceQuestionSurface.get(),0, 48, 208, 48);   break;
        case HOUSETYPE::HOUSE_ATREIDES:    pQuestionPart2 = getSubPicture(mentatHouseChoiceQuestionSurface.get(),0, 96, 208, 48);   break;
        case HOUSETYPE::HOUSE_ORDOS:       pQuestionPart2 = getSubPicture(mentatHouseChoiceQuestionSurface.get(),0, 144, 208, 48);  break;
        case HOUSETYPE::HOUSE_FREMEN:      pQuestionPart2 = Scaler::defaultDoubleSurface(LoadPNG_RW(file_manager->openFile("Fremen.png").get()).get());      break;
        case HOUSETYPE::HOUSE_SARDAUKAR:   pQuestionPart2 = Scaler::defaultDoubleSurface(LoadPNG_RW(file_manager->openFile("Sardaukar.png").get()).get());   break;
        case HOUSETYPE::HOUSE_MERCENARY:   pQuestionPart2 = Scaler::defaultDoubleSurface(LoadPNG_RW(file_manager->openFile("Mercenary.png").get()).get());   break;
        default:    break;
    }
    // clang-format on

    SDL_SetColorKey(pQuestionPart2.get(), SDL_TRUE, 0);

    SDL_Rect dest1 = calcDrawingRect(pQuestionPart1.get(), 0, 0);
    SDL_BlitSurface(pQuestionPart1.get(), nullptr, pSurface.get(), &dest1);

    SDL_Rect dest2 = calcDrawingRect(pQuestionPart2.get(), getWidth(pQuestionPart1.get()), 0);
    SDL_BlitSurface(pQuestionPart2.get(), nullptr, pSurface.get(), &dest2);

    return pSurface;
}

sdl2::surface_ptr PictureFactory::createBuilderListUpperCap() const {
    return copySurface(builderListUpperCap.get());
}

sdl2::surface_ptr PictureFactory::createBuilderListLowerCap() const {
    return copySurface(builderListLowerCap.get());
}

sdl2::surface_ptr PictureFactory::createHarkonnenLogo() const {
    return copySurface(harkonnenLogo.get());
}

sdl2::surface_ptr PictureFactory::createAtreidesLogo() const {
    return copySurface(atreidesLogo.get());
}

sdl2::surface_ptr PictureFactory::createOrdosLogo() const {
    return copySurface(ordosLogo.get());
}

sdl2::surface_ptr PictureFactory::createHeraldFre(SDL_Surface* heraldHark) {
    assert(heraldHark->format->BitsPerPixel == 8);

    auto pRedReplaced = mapSurfaceColorRange(heraldHark, PALCOLOR_HARKONNEN, PALCOLOR_FREMEN);

    assert(pRedReplaced->format->BitsPerPixel == 8);
    const auto pBlueReplaced = mapSurfaceColorRange(pRedReplaced.get(), PALCOLOR_ATREIDES, PALCOLOR_FREMEN + 1);
    pRedReplaced.reset();

    replaceColor(pBlueReplaced.get(), 170, 194);
    replaceColor(pBlueReplaced.get(), 173, 195);

    const auto* const file_manager = dune::globals::pFileManager.get();

    auto pTmp1     = scaleSurface(Wsafile(file_manager->openFile("WORM.WSA").get()).getPicture(0).get(), 0.5);
    auto pSandworm = getSubPicture(pTmp1.get(), 40 - 18, 6 - 12, 83, 91);
    pTmp1.reset();

    auto pMask = LoadPNG_RW(file_manager->openFile("HeraldFreMask.png").get());
    SDL_SetColorKey(pMask.get(), SDL_TRUE, 0);

    SDL_BlitSurface(pMask.get(), nullptr, pBlueReplaced.get(), nullptr);
    pMask.reset();

    SDL_SetColorKey(pBlueReplaced.get(), SDL_TRUE, 223);

    SDL_BlitSurface(pBlueReplaced.get(), nullptr, pSandworm.get(), nullptr);

    return pSandworm;
}

sdl2::surface_ptr PictureFactory::createHeraldSard(SDL_Surface* heraldOrd, SDL_Surface* heraldAtre) {
    assert(heraldOrd->format->BitsPerPixel == 8);
    const auto pGreenReplaced = mapSurfaceColorRange(heraldOrd, PALCOLOR_ORDOS, PALCOLOR_SARDAUKAR - 1);

    replaceColor(pGreenReplaced.get(), 3, 209);

    auto pCurtain = mapSurfaceColorRange(heraldAtre, PALCOLOR_ATREIDES, PALCOLOR_SARDAUKAR);
    pCurtain      = getSubPicture(pCurtain.get(), 7, 7, 69, 49);

    auto pFrameAndCurtain = combinePictures(pGreenReplaced.get(), pCurtain.get(), 7, 7);

    const auto pMask = sdl2::surface_ptr{LoadPNG_RW(dune::globals::pFileManager->openFile("HeraldSardMask.png").get())};
    SDL_SetColorKey(pMask.get(), SDL_TRUE, 0);

    SDL_BlitSurface(pMask.get(), nullptr, pFrameAndCurtain.get(), nullptr);

    return pFrameAndCurtain;
}

sdl2::surface_ptr PictureFactory::createHeraldMerc(SDL_Surface* heraldAtre, SDL_Surface* heraldOrd) {
    assert(heraldAtre->format->BitsPerPixel == 8);
    auto pBlueReplaced = mapSurfaceColorRange(heraldAtre, PALCOLOR_ATREIDES, PALCOLOR_MERCENARY);

    const auto pRedReplaced = mapSurfaceColorRange(pBlueReplaced.get(), PALCOLOR_HARKONNEN, PALCOLOR_ATREIDES);
    pBlueReplaced.reset();

    auto pCurtain = mapSurfaceColorRange(heraldOrd, PALCOLOR_ORDOS, PALCOLOR_MERCENARY);
    pCurtain      = getSubPicture(pCurtain.get(), 7, 7, 69, 49);

    const auto pFrameAndCurtain = combinePictures(pRedReplaced.get(), pCurtain.get(), 7, 7);

    const auto* const file_manager = dune::globals::pFileManager.get();

    auto pSoldier = Wsafile(file_manager->openFile("INFANTRY.WSA").get()).getPicture(0);
    pSoldier      = getSubPicture(pSoldier.get(), 49, 17, 83, 91);

    auto pMask = LoadPNG_RW(file_manager->openFile("HeraldMercMask.png").get());
    SDL_SetColorKey(pMask.get(), SDL_TRUE, 0);

    SDL_BlitSurface(pMask.get(), nullptr, pFrameAndCurtain.get(), nullptr);
    pMask.reset();

    SDL_SetColorKey(pFrameAndCurtain.get(), SDL_TRUE, 223);

    SDL_BlitSurface(pFrameAndCurtain.get(), nullptr, pSoldier.get(), nullptr);

    return pSoldier;
}

std::unique_ptr<Animation> PictureFactory::createFremenPlanet(SDL_Surface* heraldFre) {
    auto newAnimation = std::make_unique<Animation>();

    auto newFrame = sdl2::surface_ptr{LoadCPS_RW(dune::globals::pFileManager->openFile("BIGPLAN.CPS").get())};
    newFrame      = getSubPicture(newFrame.get(), -68, -34, 368, 224);

    const SDL_Rect src{0, 0, getWidth(heraldFre) - 2, 126};
    SDL_Rect dest{12, 66, getWidth(heraldFre) - 2, getHeight(heraldFre)};
    SDL_BlitSurface(heraldFre, &src, newFrame.get(), &dest);

    assert(newFrame->format->BitsPerPixel == 8);
    drawRect(newFrame.get(), 0, 0, newFrame->w - 1, newFrame->h - 1, PALCOLOR_WHITE);

    newAnimation->addFrame(std::move(newFrame));

    return newAnimation;
}

namespace {

constexpr auto create_color_map() {
    std::array<uint8_t, 256> color_map{};

    for (auto i = 0U; i < color_map.size(); i++)
        color_map[i] = static_cast<uint8_t>(i);

    return color_map;
}

} // namespace

std::unique_ptr<Animation>
PictureFactory::createSardaukarPlanet(Animation* ordosPlanetAnimation, SDL_Surface* heraldSard) {

    const sdl2::surface_ptr maskSurface{
        Scaler::defaultDoubleSurface(LoadPNG_RW(dune::globals::pFileManager->openFile("PlanetMask.png").get()).get())};
    SDL_SetColorKey(maskSurface.get(), SDL_TRUE, 0);

    auto newAnimation = std::make_unique<Animation>();

    static constexpr auto colorMap{[] {
        auto map = create_color_map();

        map[15]  = 165;
        map[154] = 13;
        map[155] = 29;
        map[156] = 30;
        map[157] = 31;
        map[158] = 164;
        map[159] = 165;
        map[160] = 24;
        map[161] = 22;
        map[162] = 13;
        map[163] = 29;
        map[164] = 31;

        return map;
    }()};

    for (const auto& pSurface : ordosPlanetAnimation->getFrames()) {
        auto newFrame = copySurface(pSurface.get());

        mapColor(newFrame.get(), colorMap.data());

        auto newFrameWithoutPlanet = copySurface(pSurface.get());

        SDL_BlitSurface(maskSurface.get(), nullptr, newFrameWithoutPlanet.get(), nullptr);
        SDL_SetColorKey(newFrameWithoutPlanet.get(), SDL_TRUE, 223);
        SDL_BlitSurface(newFrameWithoutPlanet.get(), nullptr, newFrame.get(), nullptr);

        SDL_Rect src{0, 0, getWidth(heraldSard), 126};
        SDL_Rect dest = calcDrawingRect(heraldSard, 12, 66);
        SDL_BlitSurface(heraldSard, &src, newFrame.get(), &dest);

        newAnimation->addFrame(std::move(newFrame));
    }

    return newAnimation;
}

std::unique_ptr<Animation>
PictureFactory::createMercenaryPlanet(Animation* atreidesPlanetAnimation, SDL_Surface* heraldMerc) {

    auto newAnimation = std::make_unique<Animation>();

    static constexpr auto colorMap{[] {
        auto map = create_color_map();

        map[3]   = 93;
        map[4]   = 90;
        map[68]  = 87;
        map[69]  = 88;
        map[70]  = 89;
        map[71]  = 90;
        map[72]  = 91;
        map[73]  = 92;
        map[74]  = 93;
        map[75]  = 94;
        map[76]  = 95;
        map[176] = 91;
        map[177] = 92;
        map[178] = 94;
        map[179] = 95;

        return map;
    }()};

    for (const auto& pSurface : atreidesPlanetAnimation->getFrames()) {
        auto newFrame = copySurface(pSurface.get());

        mapColor(newFrame.get(), colorMap.data());

        SDL_Rect src{0, 0, getWidth(heraldMerc), 126};
        SDL_Rect dest = calcDrawingRect(heraldMerc, 12, 66);
        SDL_BlitSurface(heraldMerc, &src, newFrame.get(), &dest);

        newAnimation->addFrame(std::move(newFrame));
    }

    return newAnimation;
}

sdl2::surface_ptr PictureFactory::mapMentatSurfaceToMercenary(SDL_Surface* ordosMentat) {
    assert(ordosMentat->format->BitsPerPixel == 8);
    auto mappedSurface = mapSurfaceColorRange(ordosMentat, PALCOLOR_ORDOS, PALCOLOR_MERCENARY);

    static constexpr auto colorMap{[] {
        auto map = create_color_map();

        map[186] = 245;
        map[187] = 250;

        return map;
    }()};

    mapColor(mappedSurface.get(), colorMap.data());

    return mappedSurface;
}

std::unique_ptr<Animation> PictureFactory::mapMentatAnimationToFremen(Animation* fremenAnimation) {
    auto newAnimation = std::make_unique<Animation>();

    for (const auto& pSurface : fremenAnimation->getFrames()) {
        newAnimation->addFrame(mapMentatSurfaceToFremen(pSurface.get()));
    }

    newAnimation->setFrameDurationTime(fremenAnimation->getFrameDurationTime());
    newAnimation->setNumLoops(fremenAnimation->getLoopsLeft());

    return newAnimation;
}

sdl2::surface_ptr PictureFactory::mapMentatSurfaceToSardaukar(SDL_Surface* harkonnenMentat) {
    assert(harkonnenMentat->format->BitsPerPixel == 8);
    auto mappedSurface = mapSurfaceColorRange(harkonnenMentat, PALCOLOR_HARKONNEN, PALCOLOR_SARDAUKAR);

    static constexpr auto colorMap{[] {
        auto map = create_color_map();

        map[54]  = 212;
        map[56]  = 212;
        map[57]  = 213;
        map[58]  = 213;
        map[121] = 211;
        map[199] = 210;
        map[200] = 211;
        map[201] = 211;
        map[202] = 213;

        return map;
    }()};

    mapColor(mappedSurface.get(), colorMap.data());

    return mappedSurface;
}

std::unique_ptr<Animation> PictureFactory::mapMentatAnimationToSardaukar(Animation* harkonnenAnimation) {
    auto newAnimation = std::make_unique<Animation>();

    for (const auto& pSurface : harkonnenAnimation->getFrames()) {
        newAnimation->addFrame(mapMentatSurfaceToSardaukar(pSurface.get()));
    }

    newAnimation->setFrameDurationTime(harkonnenAnimation->getFrameDurationTime());
    newAnimation->setNumLoops(harkonnenAnimation->getLoopsLeft());

    return newAnimation;
}

std::unique_ptr<Animation> PictureFactory::mapMentatAnimationToMercenary(Animation* ordosAnimation) {
    auto newAnimation = std::make_unique<Animation>();

    for (const auto& pSurface : ordosAnimation->getFrames()) {
        newAnimation->addFrame(mapMentatSurfaceToMercenary(pSurface.get()));
    }

    newAnimation->setFrameDurationTime(ordosAnimation->getFrameDurationTime());
    newAnimation->setNumLoops(ordosAnimation->getLoopsLeft());

    return newAnimation;
}

sdl2::surface_ptr PictureFactory::mapMentatSurfaceToFremen(SDL_Surface* fremenMentat) {
    assert(fremenMentat->format->BitsPerPixel == 8);
    sdl2::surface_ptr mappedSurface{mapSurfaceColorRange(fremenMentat, PALCOLOR_ATREIDES, PALCOLOR_FREMEN)};

    static constexpr auto colorMap{[] {
        auto map = create_color_map();

        map[179] = 12;
        map[180] = 12;
        map[181] = 12;
        map[182] = 12;

        return map;
    }()};

    mapColor(mappedSurface.get(), colorMap.data());

    return mappedSurface;
}
