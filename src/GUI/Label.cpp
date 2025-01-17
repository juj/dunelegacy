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

#include "GUI/Label.h"

#include "misc/DrawingRectHelper.h"
#include <misc/string_util.h>

#include <algorithm>

Label::Label() {
    Label::enableResizing(true, true);
}
Label::~Label() = default;

void Label::resize(uint32_t width, uint32_t height) {
    invalidateTextures();
    parent::resize(width, height);
}

Point Label::getMinimumSize() const {
    Point p(0, 0);

    // split text into single lines at every '\n'
    size_t startpos = 0;
    size_t nextpos  = 0;
    std::vector<std::string_view> hardLines;
    do {
        nextpos = text_.find('\n', startpos);
        if (nextpos == std::string::npos) {
            hardLines.emplace_back(text_.data() + startpos, text_.length() - startpos);
        } else {
            hardLines.emplace_back(text_.data() + startpos, nextpos - startpos);
            startpos = nextpos + 1;
        }
    } while (nextpos != std::string::npos);

    const auto& style = GUIStyle::getInstance();

    for (const auto& hardLine : hardLines) {
        const auto minLabelSize = style.getMinimumLabelSize(hardLine, fontSize_);

        p.x = std::max(p.x, minLabelSize.x);
        p.y += minLabelSize.y;
    }

    return p;
}

void Label::draw(Point position) {
    if (!isEnabled() || !isVisible())
        return;

    updateTextures();

    if (!pTexture_)
        return;

    const auto size = getSize();

    const auto x = static_cast<float>(position.x) + (static_cast<float>(size.x) - pTexture_.width_) / 2;
    const auto y = static_cast<float>(position.y) + (static_cast<float>(size.y) - pTexture_.height_) / 2;

    pTexture_.draw(dune::globals::renderer.get(), x, y);
}

std::unique_ptr<Label>
Label::create(const std::string& text, Uint32 textcolor, Uint32 textshadowcolor, Uint32 backgroundcolor) {
    auto label = std::make_unique<Label>();
    label->setText(text);
    label->setTextColor(textcolor, textshadowcolor, backgroundcolor);
    label->pAllocated_ = true;
    return label;
}

void Label::updateTextures() {
    parent::updateTextures();

    if (pTexture_)
        return;

    const auto& gui      = GUIStyle::getInstance();
    auto* const renderer = dune::globals::renderer.get();

    const auto size = getSize();

    const auto textLines =
        greedyWordWrap(text_, static_cast<float>(size.x), [&gui, font = fontSize_](std::string_view tmp) {
            return static_cast<float>(gui.getMinimumLabelSize(tmp, font).x) - 4.f;
        });

    pTexture_ = gui.createLabel(renderer, size.x, size.y, textLines, fontSize_, alignment_, text_color_,
                                text_shadow_color_, background_color_);
}

void Label::invalidateTextures() {
    pTexture_ = DuneTextureOwned();

    parent::invalidateTextures();
}
