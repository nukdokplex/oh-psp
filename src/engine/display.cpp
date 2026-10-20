#include "display.hpp"
#include <iostream>

namespace ohpsp::engine {

Display::Display() = default;

Display::~Display() {
    shutdown();
}

bool Display::init(const char* title, bool fullscreen, int scale) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

#if defined(PSP) || defined(__PSP__)
    window_ = SDL_CreateWindow(title,
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               VIRTUAL_SCREEN_WIDTH,
                               VIRTUAL_SCREEN_HEIGHT,
                               0);
#else
    uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }

    int win_w = VIRTUAL_SCREEN_WIDTH * scale;
    int win_h = VIRTUAL_SCREEN_HEIGHT * scale;

    window_ = SDL_CreateWindow(title,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               win_w,
                               win_h,
                               window_flags);
#endif

    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        // Fallback to software renderer if accelerated fails
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set logical size so rendering coordinates are always 480x272
    SDL_RenderSetLogicalSize(renderer_, VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    initialized_ = true;
    return true;
}

void Display::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (initialized_) {
        SDL_Quit();
        initialized_ = false;
    }
}

void Display::clear(uint8_t r, uint8_t g, uint8_t b) {
    if (renderer_) {
        SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
        SDL_RenderClear(renderer_);
    }
}

void Display::present() {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

} // namespace ohpsp::engine
