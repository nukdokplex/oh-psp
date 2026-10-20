#pragma once

#include <SDL.h>

namespace ohpsp::engine {

namespace Colors {
    constexpr SDL_Color Background      { 34,  36,  43, 255 }; // #22242b
    constexpr SDL_Color PanelBackground { 42,  45,  54, 255 }; // #2a2d36
    constexpr SDL_Color EmptyCell       { 52,  56,  68, 255 }; // #343844
    constexpr SDL_Color EmptyCellHover  { 65,  70,  84, 255 }; // #414654
    constexpr SDL_Color Blue            { 41, 173, 255, 255 }; // #29adff
    constexpr SDL_Color Red             { 234,  85,  72, 255 }; // #ea5548
    constexpr SDL_Color TextWhite       { 245, 247, 250, 255 }; // #f5f7fa
    constexpr SDL_Color TextDim         { 140, 148, 163, 255 }; // #8c94a3
    constexpr SDL_Color CursorHighlight { 255, 255, 255, 220 };
    constexpr SDL_Color AccentGold      { 245, 197,  66, 255 }; // #f5c542
    constexpr SDL_Color SuccessGreen    {  46, 204, 113, 255 }; // #2ecc71
}

class RenderUtils {
public:
    static void fill_rect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color);
    static void draw_rect_outline(SDL_Renderer* renderer, const SDL_Rect& rect, int thickness, SDL_Color color);

    static void draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color);
    static void draw_circle_outline(SDL_Renderer* renderer, int cx, int cy, int radius, int thickness, SDL_Color color);

    static void draw_rounded_rect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);
    static void draw_rounded_rect_outline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int thickness, SDL_Color color);

    // Desktop-style pointer arrow (Указатель)
    static void draw_pointer(SDL_Renderer* renderer, int px, int py);
};

} // namespace ohpsp::engine
