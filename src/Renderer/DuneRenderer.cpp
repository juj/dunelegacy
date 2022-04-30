#include <Renderer/DuneRenderer.h>

#include <cmath>

void DuneDrawSelectionBox(SDL_Renderer* renderer, float x, float y, float w, float h, uint32_t color) {
    setRenderDrawColor(renderer, color);

    const auto zoom = dune::globals::currentZoomlevel;

    // now draw the box with parts at all corners
    for (auto i = 0; i <= zoom; i++) {
        const auto offset = static_cast<float>(zoom + 1) * 3.f;
        const auto fi     = static_cast<float>(i);

        // top left bit
        DuneDrawLines(renderer, {{x + fi, y + offset}, {x + fi, y + fi}, {x + offset, y + fi}});

        // top right bit
        DuneDrawLines(renderer, {{x + w - 1 - fi, y + offset}, {x + w - 1 - fi, y + fi}, {x + w - 1 - offset, y + fi}});

        // bottom left bit
        DuneDrawLines(renderer, {{x + fi, y + h - 1 - offset}, {x + fi, y + h - fi}, {x + offset, y + h - fi}});

        // bottom right bit
        DuneDrawLines(renderer, {{x + w - 1 - offset, y + h - 1 - fi},
                                 {x + w - 1 - fi, y + h - 1 - fi},
                                 {x + w - 1 - fi, y + h - 1 - offset}});
    }
}

int Dune_RenderCopyEx(SDL_Renderer* renderer, const DuneTexture* texture, const SDL_Rect* srcrect,
                      const SDL_Rect* dstrect, const double angle, const SDL_Point* center,
                      const SDL_RendererFlip flip) {
    assert(texture && texture->texture_);
    assert(texture->source_.x >= 0 && texture->source_.y >= 0 && texture->source_.w > 0 && texture->source_.h > 0);

    DuneRendererImplementation::countRenderCopy(texture->texture_);

    if (srcrect) {
        assert(srcrect->x >= 0 && srcrect->y >= 0 && srcrect->w > 0 && srcrect->h > 0);
        assert(srcrect->x + srcrect->w <= texture->source_.w);
        assert(srcrect->y + srcrect->h <= texture->source_.h);

        const SDL_Rect offset{texture->source_.x + srcrect->x, texture->source_.y + srcrect->y, srcrect->w, srcrect->h};

        return SDL_RenderCopyEx(renderer, texture->texture_, &offset, dstrect, angle, center, flip);
    }

    const auto source = texture->source_rect();
    return SDL_RenderCopyEx(renderer, texture->texture_, &source, dstrect, angle, center, flip);
}

int Dune_RenderCopyExF(SDL_Renderer* renderer, const DuneTexture* texture, const SDL_Rect* srcrect,
                       const SDL_FRect* dstrect, const double angle, const SDL_FPoint* center,
                       const SDL_RendererFlip flip) {
    assert(texture && texture->texture_);
    assert(texture->source_.x >= 0 && texture->source_.y >= 0 && texture->source_.w > 0 && texture->source_.h > 0);

    DuneRendererImplementation::countRenderCopy(texture->texture_);

    if (srcrect) {
        assert(srcrect->x >= 0 && srcrect->y >= 0 && srcrect->w > 0 && srcrect->h > 0);
        assert(srcrect->x + srcrect->w <= texture->source_.w);
        assert(srcrect->y + srcrect->h <= texture->source_.h);

        const SDL_Rect offset{texture->source_.x + srcrect->x, texture->source_.y + srcrect->y, srcrect->w, srcrect->h};

        return SDL_RenderCopyExF(renderer, texture->texture_, &offset, dstrect, angle, center, flip);
    }

    const auto source = texture->source_rect();
    return SDL_RenderCopyExF(renderer, texture->texture_, &source, dstrect, angle, center, flip);
}

void Dune_RenderCopy(SDL_Renderer* renderer, const DuneTexture* texture, const SDL_Rect* srcrect,
                     const SDL_Rect* dstrect) {
    assert(texture && texture->texture_);
    assert(texture->source_.x >= 0 && texture->source_.y >= 0 && texture->source_.w > 0 && texture->source_.h > 0);

    DuneRendererImplementation::countRenderCopy(texture->texture_);

    if (srcrect) {
        assert(srcrect->x >= 0 && srcrect->y >= 0 && srcrect->w > 0 && srcrect->h > 0);
        assert(srcrect->x + srcrect->w <= texture->source_.w);
        assert(srcrect->y + srcrect->h <= texture->source_.h);

        const SDL_Rect offset{texture->source_.x + srcrect->x, texture->source_.y + srcrect->y, srcrect->w, srcrect->h};

        SDL_RenderCopy(renderer, texture->texture_, &offset, dstrect);
    } else {
        const auto src = texture->source_.as_sdl();
        SDL_RenderCopy(renderer, texture->texture_, &src, dstrect);
    }
}

void Dune_RenderCopyF(SDL_Renderer* renderer, const DuneTexture* texture, const SDL_Rect* srcrect,
                      const SDL_FRect* dstrect) {
    assert(texture && texture->texture_);
    assert(texture->source_.x >= 0 && texture->source_.y >= 0 && texture->source_.w > 0 && texture->source_.h > 0);

    DuneRendererImplementation::countRenderCopy(texture->texture_);

    if (srcrect) {
        assert(srcrect->x >= 0 && srcrect->y >= 0 && srcrect->w > 0 && srcrect->h > 0);
        assert(srcrect->x + srcrect->w <= texture->source_.w);
        assert(srcrect->y + srcrect->h <= texture->source_.h);

        const SDL_Rect offset{texture->source_.x + srcrect->x, texture->source_.y + srcrect->y, srcrect->w, srcrect->h};

        SDL_RenderCopyF(renderer, texture->texture_, &offset, dstrect);
    } else {
        const auto src = texture->source_.as_sdl();
        SDL_RenderCopyF(renderer, texture->texture_, &src, dstrect);
    }
}

void Dune_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, int x, int y) {
    DuneRendererImplementation::countRenderCopy(texture);

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    const SDL_FRect dest{static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)};

    SDL_RenderCopyF(renderer, texture, nullptr, &dest);
}

#if _DEBUG

void DuneRendererImplementation::countRenderCopy(SDL_Texture* texture) {
    if (render_texture != texture) {
        render_texture = texture;
        ++render_texture_changes;
    }
    ++render_copies;
    ++render_textures[texture];
}

void Dune_RenderDump() {
    using namespace DuneRendererImplementation;

    sdl2::log_info("present calls: %d, copy calls: %d, texture changes: %d", render_presents, render_copies,
                   render_texture_changes);

    auto max_w = 0, max_h = 0;
    auto pixels = 0;

    for (const auto& it : render_textures) {

        int h, w;
        if (SDL_QueryTexture(it.first, nullptr, nullptr, &w, &h))
            continue;

        sdl2::log_info("texture %x of size %dx%d rendered %d times", reinterpret_cast<intptr_t>(it.first), w, h,
                       it.second);

        if (w > max_w)
            max_w = w;
        if (h > max_h)
            max_h = h;

        pixels += h * w;
    }

    const auto square = static_cast<int>(std::ceil(std::sqrt(pixels)));

    sdl2::log_info("%ld textures max_w=%d max_h=%d pixels=%d (%dx%d)", render_textures.size(), max_w, max_h, pixels,
                   square, square);
}

namespace DuneRendererImplementation {
int render_copies;
int render_presents;
bool render_dump;

SDL_Texture* render_texture;
int render_texture_changes;

std::map<SDL_Texture*, int> render_textures;
} // namespace DuneRendererImplementation

#endif // _DEBUG
