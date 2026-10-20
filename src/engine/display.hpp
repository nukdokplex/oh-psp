#pragma once

#include <SDL.h>

namespace ohpsp::engine {

constexpr int VIRTUAL_SCREEN_WIDTH = 480;
constexpr int VIRTUAL_SCREEN_HEIGHT = 272;

class Display {
public:
    Display();
    ~Display();

    bool init(const char* title = "Oh PSP", bool fullscreen = false, int scale = 2);
    void shutdown();

    void clear(uint8_t r = 34, uint8_t g = 37, uint8_t b = 44);
    void present();

    SDL_Renderer* renderer() const { return renderer_; }
    SDL_Window* window() const { return window_; }

private:
    bool initialized_ = false;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace ohpsp::engine
