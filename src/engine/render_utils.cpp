#include "render_utils.hpp"
#include <algorithm>

namespace ohpsp::engine {

void RenderUtils::fill_rect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void RenderUtils::draw_rect_outline(SDL_Renderer* renderer, const SDL_Rect& rect, int thickness, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; ++i) {
        SDL_Rect r{rect.x + i, rect.y + i, rect.w - 2 * i, rect.h - 2 * i};
        SDL_RenderDrawRect(renderer, &r);
    }
}

void RenderUtils::draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color) {
    if (radius <= 0) return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = static_cast<int>(std::sqrt(r2 - dy * dy));
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void RenderUtils::draw_circle_outline(SDL_Renderer* renderer, int cx, int cy, int radius, int thickness, SDL_Color color) {
    if (radius <= 0) return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int t = 0; t < thickness; ++t) {
        int r = radius - t;
        if (r < 0) break;
        int x = r;
        int y = 0;
        int err = 0;

        while (x >= y) {
            SDL_RenderDrawPoint(renderer, cx + x, cy + y);
            SDL_RenderDrawPoint(renderer, cx + y, cy + x);
            SDL_RenderDrawPoint(renderer, cx - y, cy + x);
            SDL_RenderDrawPoint(renderer, cx - x, cy + y);
            SDL_RenderDrawPoint(renderer, cx - x, cy - y);
            SDL_RenderDrawPoint(renderer, cx - y, cy - x);
            SDL_RenderDrawPoint(renderer, cx + y, cy - x);
            SDL_RenderDrawPoint(renderer, cx + x, cy - y);

            if (err <= 0) {
                y += 1;
                err += 2 * y + 1;
            }
            if (err > 0) {
                x -= 1;
                err -= 2 * x + 1;
            }
        }
    }
}

void RenderUtils::draw_rounded_rect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    if (radius <= 0) {
        fill_rect(renderer, rect, color);
        return;
    }

    radius = std::min({radius, rect.w / 2, rect.h / 2});

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // 1. Center vertical strip (full height)
    SDL_Rect center_rect{rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_RenderFillRect(renderer, &center_rect);

    // 2. Left and right vertical strips (between corner arcs)
    int inner_h = rect.h - 2 * radius;
    if (inner_h > 0) {
        SDL_Rect left_rect{rect.x, rect.y + radius, radius, inner_h};
        SDL_RenderFillRect(renderer, &left_rect);

        SDL_Rect right_rect{rect.x + rect.w - radius, rect.y + radius, radius, inner_h};
        SDL_RenderFillRect(renderer, &right_rect);
    }

    // 3. Four corner quarter-circles
    int r2 = radius * radius;
    for (int dy = 1; dy <= radius; ++dy) {
        int dx = static_cast<int>(std::round(std::sqrt(r2 - dy * dy)));
        if (dx <= 0) continue;

        int row_top = rect.y + radius - dy;
        int row_bot = rect.y + rect.h - 1 - radius + dy;

        // Top-left
        SDL_RenderDrawLine(renderer, rect.x + radius - dx, row_top, rect.x + radius - 1, row_top);
        // Top-right
        SDL_RenderDrawLine(renderer, rect.x + rect.w - radius, row_top, rect.x + rect.w - radius + dx - 1, row_top);
        // Bottom-left
        SDL_RenderDrawLine(renderer, rect.x + radius - dx, row_bot, rect.x + radius - 1, row_bot);
        // Bottom-right
        SDL_RenderDrawLine(renderer, rect.x + rect.w - radius, row_bot, rect.x + rect.w - radius + dx - 1, row_bot);
    }
}

static void draw_quarter_circle(SDL_Renderer* renderer, int cx, int cy, int radius, int quadrant, SDL_Color color) {
    if (radius <= 0) return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        switch (quadrant) {
            case 0: // Top-Left
                SDL_RenderDrawPoint(renderer, cx - x, cy - y);
                SDL_RenderDrawPoint(renderer, cx - y, cy - x);
                break;
            case 1: // Top-Right
                SDL_RenderDrawPoint(renderer, cx + x, cy - y);
                SDL_RenderDrawPoint(renderer, cx + y, cy - x);
                break;
            case 2: // Bottom-Left
                SDL_RenderDrawPoint(renderer, cx - x, cy + y);
                SDL_RenderDrawPoint(renderer, cx - y, cy + x);
                break;
            case 3: // Bottom-Right
                SDL_RenderDrawPoint(renderer, cx + x, cy + y);
                SDL_RenderDrawPoint(renderer, cx + y, cy + x);
                break;
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void RenderUtils::draw_rounded_rect_outline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int thickness, SDL_Color color) {
    for (int i = 0; i < thickness; ++i) {
        SDL_Rect r{rect.x + i, rect.y + i, rect.w - 2 * i, rect.h - 2 * i};
        if (r.w <= 0 || r.h <= 0) break;

        int rad = std::max(0, radius - i);
        int max_rad = std::min((r.w - 1) / 2, (r.h - 1) / 2);
        rad = std::min(rad, max_rad);

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        int left = r.x;
        int right = r.x + r.w - 1;
        int top = r.y;
        int bottom = r.y + r.h - 1;

        // Draw 4 straight edge segments between corners
        if (right - rad >= left + rad) {
            SDL_RenderDrawLine(renderer, left + rad, top, right - rad, top);
            SDL_RenderDrawLine(renderer, left + rad, bottom, right - rad, bottom);
        }
        if (bottom - rad >= top + rad) {
            SDL_RenderDrawLine(renderer, left, top + rad, left, bottom - rad);
            SDL_RenderDrawLine(renderer, right, top + rad, right, bottom - rad);
        }

        // Draw 4 rounded corner arcs (quadrants)
        if (rad > 0) {
            draw_quarter_circle(renderer, left + rad, top + rad, rad, 0, color);     // Top-Left
            draw_quarter_circle(renderer, right - rad, top + rad, rad, 1, color);    // Top-Right
            draw_quarter_circle(renderer, left + rad, bottom - rad, rad, 2, color);  // Bottom-Left
            draw_quarter_circle(renderer, right - rad, bottom - rad, rad, 3, color); // Bottom-Right
        }
    }
}

void RenderUtils::draw_pointer(SDL_Renderer* renderer, int px, int py) {
    static const uint8_t SHAPE[18][12] = {
        {1,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,0,0},
        {1,2,2,2,2,2,1,0,0,0,0,0},
        {1,2,2,2,2,2,2,1,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,2,2,2,1,0,0},
        {1,2,2,2,2,2,1,1,1,1,1,0},
        {1,2,2,1,2,2,1,0,0,0,0,0},
        {1,2,1,0,1,2,2,1,0,0,0,0},
        {1,1,0,0,1,2,2,1,0,0,0,0},
        {1,0,0,0,0,1,2,2,1,0,0,0},
        {0,0,0,0,0,1,2,2,1,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    };

    // 1. Semi-transparent drop shadow (+1, +1)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
    for (int y = 0; y < 18; ++y) {
        for (int x = 0; x < 12; ++x) {
            if (SHAPE[y][x] != 0) {
                SDL_RenderDrawPoint(renderer, px + x + 1, py + y + 1);
            }
        }
    }

    // 2. Dark crisp border
    SDL_SetRenderDrawColor(renderer, 15, 17, 23, 255);
    for (int y = 0; y < 18; ++y) {
        for (int x = 0; x < 12; ++x) {
            if (SHAPE[y][x] == 1) {
                SDL_RenderDrawPoint(renderer, px + x, py + y);
            }
        }
    }

    // 3. Bright white fill
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < 18; ++y) {
        for (int x = 0; x < 12; ++x) {
            if (SHAPE[y][x] == 2) {
                SDL_RenderDrawPoint(renderer, px + x, py + y);
            }
        }
    }
}

} // namespace ohpsp::engine
