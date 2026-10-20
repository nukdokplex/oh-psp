#include "vision_module.hpp"
#include "games/vision/core/vision_generator.hpp"
#include "engine/render_utils.hpp"
#include "engine/font.hpp"
#include "engine/display.hpp"
#include "engine/audio.hpp"
#include <algorithm>
#include <chrono>

namespace ohpsp::vision {

using namespace engine;

static std::string format_time(float seconds) {
    int total_sec = static_cast<int>(seconds);
    int mins = total_sec / 60;
    int secs = total_sec % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    return std::string(buf);
}

VisionModule::VisionModule() {
    current_preset_ = {"5x5", 5};
}

std::vector<GamePreset> VisionModule::get_presets() const {
    return {
        {"4x4", 4},
        {"5x5", 5},
        {"6x6", 6},
        {"7x7", 7},
        {"8x8", 8},
        {"9x9", 9}
    };
}

void VisionModule::start_game(const GamePreset& preset) {
    current_preset_ = preset;
    exit_to_menu_ = false;
    cursor_x_ = 0;
    cursor_y_ = 0;
    hint_active_ = false;
    elapsed_time_ = 0.0f;
    victory_choice_ = 0;
    next_puzzle();
}

void VisionModule::restart_game() {
    board_.clear();
    completed_ = false;
    hint_active_ = false;
    elapsed_time_ = 0.0f;
    victory_choice_ = 0;
    AudioManager::instance().play_sfx(SoundEffect::TileClear);
}

void VisionModule::next_puzzle() {
    uint32_t seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    board_ = Generator::generate_puzzle(current_preset_.size, seed, true);
    solution_ = board_;
    Solver::solve(solution_);
    completed_ = false;
    hint_active_ = false;
    elapsed_time_ = 0.0f;
    victory_choice_ = 0;
    AudioManager::instance().play_sfx(SoundEffect::MenuSelect);
}

void VisionModule::cycle_current_cell(bool forward) {
    if (completed_) {
        return;
    }
    if (board_.is_fixed(cursor_x_, cursor_y_)) {
        AudioManager::instance().play_sfx(SoundEffect::ActionBlocked);
        return;
    }

    hint_active_ = false;
    TileType cur = board_.get_type(cursor_x_, cursor_y_);
    TileType next = TileType::Unknown;

    if (forward) {
        if (cur == TileType::Unknown) next = TileType::Dot;
        else if (cur == TileType::Dot) next = TileType::Wall;
        else next = TileType::Unknown;
    } else {
        if (cur == TileType::Unknown) next = TileType::Wall;
        else if (cur == TileType::Wall) next = TileType::Dot;
        else next = TileType::Unknown;
    }

    board_.set_type(cursor_x_, cursor_y_, next);

    if (next == TileType::Unknown) {
        AudioManager::instance().play_sfx(SoundEffect::TileClear);
    } else if (next == TileType::Dot) {
        AudioManager::instance().play_sfx(SoundEffect::TilePlaceA);
    } else if (next == TileType::Wall) {
        AudioManager::instance().play_sfx(SoundEffect::TilePlaceB);
    }

    if (board_.is_complete()) {
        completed_ = true;
        victory_choice_ = 0;
        AudioManager::instance().play_sfx(SoundEffect::PuzzleSolved);
    }
}

void VisionModule::request_hint() {
    if (completed_) return;

    if (Solver::find_hint(board_, current_hint_)) {
        hint_active_ = true;
        hint_timer_ = 5.0f;
        cursor_x_ = current_hint_.x;
        cursor_y_ = current_hint_.y;
        AudioManager::instance().play_sfx(SoundEffect::HintRequested);
    } else {
        AudioManager::instance().play_sfx(SoundEffect::ActionBlocked);
    }
}

void VisionModule::handle_input(const InputState& input) {
    int n = board_.size();

    // Victory modal controls
    if (completed_) {
        int modal_w = 280;
        int modal_h = 144;
        int modal_y = (VIRTUAL_SCREEN_HEIGHT - modal_h) / 2;
        int btn_w = 220;
        int btn_h = 24;
        int btn_x = (VIRTUAL_SCREEN_WIDTH - btn_w) / 2;
        int btn0_y = modal_y + 60;
        int btn1_y = modal_y + 90;

        if (input.pointer_moved) {
            int px = static_cast<int>(input.pointer_x);
            int py = static_cast<int>(input.pointer_y);

            if (px >= btn_x && px < btn_x + btn_w) {
                if (py >= btn0_y && py < btn0_y + btn_h) {
                    if (victory_choice_ != 0) {
                        victory_choice_ = 0;
                        AudioManager::instance().play_sfx(SoundEffect::CursorMove);
                    }
                } else if (py >= btn1_y && py < btn1_y + btn_h) {
                    if (victory_choice_ != 1) {
                        victory_choice_ = 1;
                        AudioManager::instance().play_sfx(SoundEffect::CursorMove);
                    }
                }
            }
        }

        if (input.pressed_up || input.pressed_down || input.pressed_left || input.pressed_right) {
            victory_choice_ = 1 - victory_choice_;
            AudioManager::instance().play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_a) {
            AudioManager::instance().play_sfx(SoundEffect::MenuSelect);
            if (victory_choice_ == 0) {
                next_puzzle();
            } else {
                exit_to_menu_ = true;
            }
        }
        if (input.pressed_b) {
            AudioManager::instance().play_sfx(SoundEffect::MenuBack);
            exit_to_menu_ = true;
        }
        return;
    }

    if (input.pressed_b) {
        AudioManager::instance().play_sfx(SoundEffect::MenuBack);
        exit_to_menu_ = true;
        return;
    }

    // Hover check: Pointer moves grid selection Focus (Селектор) ONLY when pointer actually moves
    if (input.pointer_moved) {
        int max_grid_h = 196;
        int gap = (n >= 8) ? 2 : 4;
        int cell_size = (max_grid_h - (n - 1) * gap) / n;
        if (cell_size > 36) cell_size = 36;
        if (cell_size < 12) cell_size = 12;

        int grid_w = n * cell_size + (n - 1) * gap;
        int grid_h = n * cell_size + (n - 1) * gap;
        int grid_x = (VIRTUAL_SCREEN_WIDTH - grid_w) / 2;
        int grid_y = 38 + (max_grid_h - grid_h) / 2;

        int px = static_cast<int>(input.pointer_x);
        int py = static_cast<int>(input.pointer_y);

        if (px >= grid_x && px < grid_x + grid_w && py >= grid_y && py < grid_y + grid_h) {
            int cell_step = cell_size + gap;
            int rel_x = px - grid_x;
            int rel_y = py - grid_y;
            int col = rel_x / cell_step;
            int row = rel_y / cell_step;
            if (col >= 0 && col < n && row >= 0 && row < n) {
                int off_x = rel_x % cell_step;
                int off_y = rel_y % cell_step;
                if (off_x < cell_size && off_y < cell_size) {
                    if (cursor_x_ != col || cursor_y_ != row) {
                        cursor_x_ = col;
                        cursor_y_ = row;
                        AudioManager::instance().play_sfx(SoundEffect::CursorMove);
                    }
                }
            }
        }
    }

    bool cursor_moved = false;
    if (input.pressed_up) {
        cursor_y_ = (cursor_y_ - 1 + n) % n;
        cursor_moved = true;
    }
    if (input.pressed_down) {
        cursor_y_ = (cursor_y_ + 1) % n;
        cursor_moved = true;
    }
    if (input.pressed_left) {
        cursor_x_ = (cursor_x_ - 1 + n) % n;
        cursor_moved = true;
    }
    if (input.pressed_right) {
        cursor_x_ = (cursor_x_ + 1) % n;
        cursor_moved = true;
    }

    if (cursor_moved) {
        AudioManager::instance().play_sfx(SoundEffect::CursorMove);
    }

    if (input.pressed_a) {
        cycle_current_cell(true);
    }
    if (input.pressed_x) {
        cycle_current_cell(false);
    }
    if (input.pressed_y) {
        request_hint();
    }
    if (input.pressed_select) {
        restart_game();
    }
    if (input.pressed_start) {
        next_puzzle();
    }
}

void VisionModule::update(float dt) {
    pulse_timer_ += dt;
    if (pulse_timer_ > 6.283185f) {
        pulse_timer_ -= 6.283185f;
    }

    if (!completed_) {
        elapsed_time_ += dt;
    }

    if (hint_active_) {
        hint_timer_ -= dt;
        if (hint_timer_ <= 0.0f) {
            hint_active_ = false;
        }
    }
}

void VisionModule::render(SDL_Renderer* renderer) {
    int n = board_.size();

    // 1. Title bar & Status
    std::string title_str = "VISION " + current_preset_.name;
    FontRenderer::draw_text(renderer, title_str, 20, 12, Colors::TextWhite, 2, true);

    int remaining = board_.empty_count();
    std::string time_str = "Time: " + format_time(elapsed_time_);
    FontRenderer::draw_text(renderer, time_str, 240, 15, Colors::TextWhite, 1, false);

    std::string rem_str = "Empty: " + std::to_string(remaining);
    FontRenderer::draw_text(renderer, rem_str, 380, 15, Colors::TextDim, 1, false);

    // 2. Grid Geometry Calculation
    int max_grid_h = 196;
    int gap = (n >= 8) ? 2 : 4;
    int cell_size = (max_grid_h - (n - 1) * gap) / n;
    if (cell_size > 36) cell_size = 36;
    if (cell_size < 12) cell_size = 12;

    int grid_w = n * cell_size + (n - 1) * gap;
    int grid_h = n * cell_size + (n - 1) * gap;

    int grid_x = (VIRTUAL_SCREEN_WIDTH - grid_w) / 2;
    int grid_y = 38 + (max_grid_h - grid_h) / 2;

    // Grid panel background
    SDL_Rect board_panel{grid_x - 6, grid_y - 6, grid_w + 12, grid_h + 12};
    RenderUtils::draw_rounded_rect(renderer, board_panel, 8, Colors::PanelBackground);

    // 3. Render Cells
    int circle_radius = cell_size / 2 - 1;
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            int cx = grid_x + x * (cell_size + gap);
            int cy = grid_y + y * (cell_size + gap);
            int center_x = cx + cell_size / 2;
            int center_y = cy + cell_size / 2;

            Cell cell = board_.get(x, y);

            if (cell.type == TileType::Unknown) {
                RenderUtils::draw_filled_circle(renderer, center_x, center_y, circle_radius, Colors::EmptyCell);
            } else if (cell.type == TileType::Dot) {
                RenderUtils::draw_filled_circle(renderer, center_x, center_y, circle_radius, Colors::Blue);
            } else if (cell.type == TileType::Wall) {
                RenderUtils::draw_filled_circle(renderer, center_x, center_y, circle_radius, Colors::Red);
            } else if (cell.type == TileType::NumberedDot) {
                bool satisfied = board_.is_tile_satisfied(x, y);
                SDL_Color dot_color = satisfied ? SDL_Color{30, 144, 255, 255} : Colors::Blue;
                RenderUtils::draw_filled_circle(renderer, center_x, center_y, circle_radius, dot_color);

                std::string num_str = std::to_string(cell.number);
                int font_scale = (cell_size >= 28) ? 2 : 1;
                int text_w = FontRenderer::get_text_width(num_str, font_scale);
                int text_h = FontRenderer::get_text_height(font_scale);
                int text_x = center_x - text_w / 2;
                int text_y = center_y - text_h / 2;

                FontRenderer::draw_text(renderer, num_str, text_x, text_y, Colors::TextWhite, font_scale, false);
            }
        }
    }

    // 4. Cursor / Selection
    int cur_screen_x = grid_x + cursor_x_ * (cell_size + gap);
    int cur_screen_y = grid_y + cursor_y_ * (cell_size + gap);
    int cur_center_x = cur_screen_x + cell_size / 2;
    int cur_center_y = cur_screen_y + cell_size / 2;

    float pulse = 0.5f + 0.5f * std::sin(pulse_timer_ * 4.0f);
    uint8_t alpha = static_cast<uint8_t>(180 + 75 * pulse);
    SDL_Color cursor_color = Colors::CursorHighlight;
    cursor_color.a = alpha;

    RenderUtils::draw_circle_outline(renderer, cur_center_x, cur_center_y, circle_radius + 2, 2, cursor_color);

    // 5. Hint Display
    if (hint_active_) {
        int hx = grid_x + current_hint_.x * (cell_size + gap) + cell_size / 2;
        int hy = grid_y + current_hint_.y * (cell_size + gap) + cell_size / 2;
        RenderUtils::draw_circle_outline(renderer, hx, hy, circle_radius + 3, 2, Colors::AccentGold);

        RenderUtils::draw_rounded_rect(renderer, SDL_Rect{20, 240, 440, 22}, 4, Colors::PanelBackground);
        FontRenderer::draw_text_centered(renderer, current_hint_.description, 240, 246, Colors::AccentGold, 1, false);
    } else {
        // 6. Navigation Controls (clean [Tri] prompt)
        std::string controls = "[X] Dot->Wall  [Sq] Wall->Dot  [Tri] Hint  [Select] Clear  [O] Menu";
        FontRenderer::draw_text_centered(renderer, controls, 240, 250, Colors::TextDim, 1, false);
    }

    // 7. Victory Modal Dialog
    if (completed_) {
        render_victory_modal(renderer);
    }
}

void VisionModule::render_victory_modal(SDL_Renderer* renderer) {
    // Dim background overlay
    RenderUtils::draw_rounded_rect(renderer, SDL_Rect{0, 0, VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT}, 0, SDL_Color{15, 17, 23, 200});

    // Modal Card
    int modal_w = 280;
    int modal_h = 144;
    int modal_x = (VIRTUAL_SCREEN_WIDTH - modal_w) / 2;
    int modal_y = (VIRTUAL_SCREEN_HEIGHT - modal_h) / 2;

    SDL_Rect modal_rect{modal_x, modal_y, modal_w, modal_h};
    RenderUtils::draw_rounded_rect(renderer, modal_rect, 8, Colors::PanelBackground);
    RenderUtils::draw_rounded_rect_outline(renderer, modal_rect, 8, 2, Colors::SuccessGreen);

    // Header Title
    FontRenderer::draw_text_centered(renderer, "PUZZLE SOLVED!", 240, modal_y + 14, Colors::SuccessGreen, 2, true);

    // Elapsed Time
    std::string time_text = "Time: " + format_time(elapsed_time_);
    FontRenderer::draw_text_centered(renderer, time_text, 240, modal_y + 40, Colors::TextWhite, 1, false);

    // Buttons: Next Puzzle / Back to Menu
    int btn_w = 220;
    int btn_h = 24;
    int btn_x = (VIRTUAL_SCREEN_WIDTH - btn_w) / 2;

    int btn0_y = modal_y + 60;
    int btn1_y = modal_y + 90;

    SDL_Rect btn0_rect{btn_x, btn0_y, btn_w, btn_h};
    SDL_Rect btn1_rect{btn_x, btn1_y, btn_w, btn_h};

    bool sel0 = (victory_choice_ == 0);
    bool sel1 = (victory_choice_ == 1);

    RenderUtils::draw_rounded_rect(renderer, btn0_rect, 4, sel0 ? SDL_Color{45, 52, 68, 255} : SDL_Color{32, 35, 43, 255});
    RenderUtils::draw_rounded_rect(renderer, btn1_rect, 4, sel1 ? SDL_Color{45, 52, 68, 255} : SDL_Color{32, 35, 43, 255});

    if (sel0) {
        RenderUtils::draw_rounded_rect_outline(renderer, btn0_rect, 4, 2, Colors::Blue);
    }
    if (sel1) {
        RenderUtils::draw_rounded_rect_outline(renderer, btn1_rect, 4, 2, Colors::Blue);
    }

    FontRenderer::draw_text_centered(renderer, "Next Puzzle", 240, btn0_y + 8, sel0 ? Colors::TextWhite : Colors::TextDim, 1, false);
    FontRenderer::draw_text_centered(renderer, "Back to Menu", 240, btn1_y + 8, sel1 ? Colors::TextWhite : Colors::TextDim, 1, false);

    // Footer Hint
    FontRenderer::draw_text_centered(renderer, "[Up/Down] Select   [X] Confirm   [O] Menu", 240, modal_y + 124, Colors::TextDim, 1, false);
}

} // namespace ohpsp::vision
