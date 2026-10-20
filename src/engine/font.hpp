#pragma once

#include <SDL.h>
#include <string>

namespace ohpsp::engine {

class FontRenderer {
public:
    static void draw_text(SDL_Renderer* renderer, const std::string& text, int x, int y,
                          SDL_Color color, int scale = 1, bool shadow = false);

    static void draw_text_centered(SDL_Renderer* renderer, const std::string& text,
                                   int center_x, int y, SDL_Color color, int scale = 1, bool shadow = false);

    static int get_text_width(const std::string& text, int scale = 1);
    static int get_text_height(int scale = 1);
};

} // namespace ohpsp::engine
