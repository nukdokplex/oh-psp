#include "main_menu.hpp"
#include "engine/render_utils.hpp"
#include "engine/font.hpp"
#include "engine/display.hpp"
#include "engine/audio.hpp"
#include <algorithm>
#include <cmath>

namespace ohpsp::menu {

using namespace engine;

MainMenu::MainMenu() = default;

void MainMenu::init() {
    state_ = MenuState::GameSelect;
    selected_game_index_ = 0;
    selected_preset_index_ = 0;
    wants_quit_ = false;
    about_scroll_y_ = 0.0f;
    target_scroll_y_ = 0.0f;
    active_game_ = nullptr;
}

void MainMenu::handle_input(const InputState& input) {
    auto& audio = AudioManager::instance();

    if (state_ == MenuState::InGame) {
        // In-game: allow shoulder triggers L/R to switch tracks, but DO NOT intercept Triangle (reserved for Hint)
        if (input.pressed_l) {
            audio.prev_track();
            audio.play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_r) {
            audio.next_track();
            audio.play_sfx(SoundEffect::CursorMove);
        }

        if (active_game_) {
            active_game_->handle_input(input);
            if (active_game_->wants_exit_to_menu()) {
                state_ = MenuState::PresetSelect;
                audio.play_sfx(SoundEffect::MenuBack);
            }
        }
        return;
    }

    // Menu music controls (L/R trigger track switch, Triangle mute)
    if (input.pressed_l) {
        audio.prev_track();
        audio.play_sfx(SoundEffect::CursorMove);
    }
    if (input.pressed_r) {
        audio.next_track();
        audio.play_sfx(SoundEffect::CursorMove);
    }
    if (input.pressed_y) {
        audio.toggle_music();
        audio.play_sfx(SoundEffect::VolumeToggle);
    }

    const auto& modules = GameRegistry::instance().get_modules();
    int total_menu_items = static_cast<int>(modules.size()) + 1; // +1 for "About" card

    if (state_ == MenuState::GameSelect) {
        int card_w = 400;
        int card_h = 50;
        int start_y = 56;
        int gap = 12;
        int px = static_cast<int>(input.pointer_x);
        int py = static_cast<int>(input.pointer_y);

        // Hover check: Pointer moves selection Focus (Селектор) ONLY when pointer actually moves
        if (input.pointer_moved) {
            for (int i = 0; i < total_menu_items; ++i) {
                int card_y = start_y + i * (card_h + gap);
                int card_x = (VIRTUAL_SCREEN_WIDTH - card_w) / 2;
                if (px >= card_x && px < card_x + card_w && py >= card_y && py < card_y + card_h) {
                    if (selected_game_index_ != i) {
                        selected_game_index_ = i;
                        audio.play_sfx(SoundEffect::CursorMove);
                    }
                    break;
                }
            }
        }

        if (input.pressed_up) {
            selected_game_index_ = (selected_game_index_ - 1 + total_menu_items) % total_menu_items;
            audio.play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_down) {
            selected_game_index_ = (selected_game_index_ + 1) % total_menu_items;
            audio.play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_a) {
            audio.play_sfx(SoundEffect::MenuSelect);
            if (selected_game_index_ < static_cast<int>(modules.size())) {
                state_ = MenuState::PresetSelect;
                selected_preset_index_ = 0;
            } else {
                // About card selected
                state_ = MenuState::AboutScreen;
                about_scroll_y_ = 0.0f;
                target_scroll_y_ = 0.0f;
            }
        }
        if (input.pressed_b) {
            audio.play_sfx(SoundEffect::MenuBack);
            wants_quit_ = true;
        }
    } else if (state_ == MenuState::PresetSelect) {
        const auto& presets = modules[selected_game_index_]->get_presets();
        if (presets.empty()) return;
        int total_presets = static_cast<int>(presets.size());

        int button_w = 64;
        int button_h = 56;
        int gap = 12;
        int total_w = total_presets * button_w + (total_presets - 1) * gap;
        int start_x = (VIRTUAL_SCREEN_WIDTH - total_w) / 2;
        int btn_y = 110;

        int px = static_cast<int>(input.pointer_x);
        int py = static_cast<int>(input.pointer_y);

        // Hover check: Pointer moves selection Focus (Селектор) ONLY when pointer actually moves
        if (input.pointer_moved) {
            for (int i = 0; i < total_presets; ++i) {
                int bx = start_x + i * (button_w + gap);
                if (px >= bx && px < bx + button_w && py >= btn_y && py < btn_y + button_h) {
                    if (selected_preset_index_ != i) {
                        selected_preset_index_ = i;
                        audio.play_sfx(SoundEffect::CursorMove);
                    }
                    break;
                }
            }
        }

        if (input.pressed_left || input.pressed_up) {
            selected_preset_index_ = (selected_preset_index_ - 1 + static_cast<int>(presets.size())) % presets.size();
            audio.play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_right || input.pressed_down) {
            selected_preset_index_ = (selected_preset_index_ + 1) % presets.size();
            audio.play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_a) {
            audio.play_sfx(SoundEffect::MenuSelect);
            active_game_ = modules[selected_game_index_];
            active_game_->start_game(presets[selected_preset_index_]);
            state_ = MenuState::InGame;
        }
        if (input.pressed_b) {
            audio.play_sfx(SoundEffect::MenuBack);
            state_ = MenuState::GameSelect;
        }
    } else if (state_ == MenuState::AboutScreen) {
        if (input.pressed_b) {
            audio.play_sfx(SoundEffect::MenuBack);
            state_ = MenuState::GameSelect;
        }
        if (input.pressed_up || input.up) {
            target_scroll_y_ -= (input.pressed_up ? 24.0f : 4.0f);
            if (input.pressed_up) audio.play_sfx(SoundEffect::CursorMove);
        }
        if (input.pressed_down || input.down) {
            target_scroll_y_ += (input.pressed_down ? 24.0f : 4.0f);
            if (input.pressed_down) audio.play_sfx(SoundEffect::CursorMove);
        }
    }
}

void MainMenu::update(float dt) {
    pulse_timer_ += dt;
    if (pulse_timer_ > 6.283185f) {
        pulse_timer_ -= 6.283185f;
    }

    if (state_ == MenuState::AboutScreen) {
        float max_scroll = 260.0f;
        target_scroll_y_ = std::clamp(target_scroll_y_, 0.0f, max_scroll);
        about_scroll_y_ += (target_scroll_y_ - about_scroll_y_) * std::min(1.0f, dt * 14.0f);
    }

    if (state_ == MenuState::InGame && active_game_) {
        active_game_->update(dt);
    }
}

void MainMenu::render(SDL_Renderer* renderer) {
    if (state_ == MenuState::InGame && active_game_) {
        active_game_->render(renderer);
        return;
    }

    if (state_ == MenuState::AboutScreen) {
        render_about_screen(renderer);
        return;
    }

    const auto& modules = GameRegistry::instance().get_modules();
    auto& audio = AudioManager::instance();

    // 1. Header Title
    FontRenderer::draw_text_centered(renderer, "OH PSP", 240, 10, Colors::TextWhite, 3, true);

    // Music ticker in top-right
    std::string music_tag;
    if (!audio.is_music_loaded()) {
        music_tag = "[SFX Only]";
    } else if (!audio.is_music_enabled()) {
        music_tag = "[Muted]";
    } else {
        music_tag = "[M] " + audio.current_track().title;
    }
    FontRenderer::draw_text(renderer, music_tag, 20, 14, (audio.is_music_loaded() && audio.is_music_enabled()) ? Colors::Blue : Colors::TextDim, 1, false);

    FontRenderer::draw_text_centered(renderer, "MODULAR LOGIC PUZZLE SUITE", 240, 38, Colors::Blue, 1, false);

    float pulse = 0.5f + 0.5f * std::sin(pulse_timer_ * 4.0f);
    uint8_t alpha = static_cast<uint8_t>(180 + 75 * pulse);

    if (state_ == MenuState::GameSelect) {
        int card_w = 400;
        int card_h = 50;
        int start_y = 56;
        int gap = 12;

        int total_items = static_cast<int>(modules.size()) + 1;

        for (int i = 0; i < total_items; ++i) {
            int card_y = start_y + i * (card_h + gap);
            SDL_Rect card_rect{(VIRTUAL_SCREEN_WIDTH - card_w) / 2, card_y, card_w, card_h};

            bool selected = (i == selected_game_index_);

            SDL_Color bg = selected ? Colors::PanelBackground : SDL_Color{38, 41, 49, 255};
            RenderUtils::draw_rounded_rect(renderer, card_rect, 6, bg);

            if (selected) {
                SDL_Color border = Colors::Blue;
                border.a = alpha;
                RenderUtils::draw_rounded_rect_outline(renderer, card_rect, 6, 2, border);
            }

            std::string title;
            std::string tagline;

            if (i < static_cast<int>(modules.size())) {
                title = modules[i]->get_title();
                tagline = modules[i]->get_tagline();
            } else {
                title = "About & Credits";
                tagline = "Developer info, legal soundtrack & tech stack";
            }

            SDL_Color title_col = selected ? Colors::TextWhite : Colors::TextDim;
            FontRenderer::draw_text(renderer, title, card_rect.x + 18, card_rect.y + 10, title_col, 2, false);
            FontRenderer::draw_text(renderer, tagline, card_rect.x + 18, card_rect.y + 30, Colors::TextDim, 1, false);
        }

        // Footer with full controls hint
        std::string footer = "[Up/Down] Select   [X] Choose   [L/R] Track   [Tri] Mute   [O] Exit";
        FontRenderer::draw_text_centered(renderer, footer, 240, 252, Colors::TextDim, 1, false);

    } else if (state_ == MenuState::PresetSelect) {
        auto current_mod = modules[selected_game_index_];
        std::string prompt = "SELECT GRID SIZE FOR " + std::string(current_mod->get_title());
        FontRenderer::draw_text_centered(renderer, prompt, 240, 75, Colors::TextWhite, 1, false);

        const auto& presets = current_mod->get_presets();
        int total_presets = static_cast<int>(presets.size());

        int button_w = 64;
        int button_h = 56;
        int gap = 12;
        int total_w = total_presets * button_w + (total_presets - 1) * gap;
        int start_x = (VIRTUAL_SCREEN_WIDTH - total_w) / 2;
        int btn_y = 110;

        for (int i = 0; i < total_presets; ++i) {
            int bx = start_x + i * (button_w + gap);
            SDL_Rect btn_rect{bx, btn_y, button_w, button_h};

            bool selected = (i == selected_preset_index_);
            SDL_Color bg = selected ? Colors::PanelBackground : SDL_Color{38, 41, 49, 255};
            RenderUtils::draw_rounded_rect(renderer, btn_rect, 6, bg);

            if (selected) {
                SDL_Color border = Colors::Blue;
                border.a = alpha;
                RenderUtils::draw_rounded_rect_outline(renderer, btn_rect, 6, 2, border);
            }

            SDL_Color text_col = selected ? Colors::TextWhite : Colors::TextDim;
            FontRenderer::draw_text_centered(renderer, presets[i].name, bx + button_w / 2, btn_y + 20, text_col, 2, false);
        }

        FontRenderer::draw_text_centered(renderer, "[Left/Right] Select Size   [X] Start Puzzle   [O] Back", 240, 246, Colors::TextDim, 1, false);
    }
}

void MainMenu::render_about_screen(SDL_Renderer* renderer) {
    // 1. Header (fixed at top)
    RenderUtils::draw_rounded_rect(renderer, SDL_Rect{10, 8, 460, 32}, 6, Colors::PanelBackground);
    FontRenderer::draw_text_centered(renderer, "ABOUT & CREDITS", 240, 16, Colors::TextWhite, 2, true);

    // 2. Viewport & Clipping Box for Scrollable Text
    int view_x = 20;
    int view_y = 48;
    int view_w = 440;
    int view_h = 194;

    SDL_Rect view_box{view_x, view_y, view_w, view_h};
    RenderUtils::draw_rounded_rect(renderer, view_box, 6, SDL_Color{28, 30, 36, 255});

    // Set scissor clip so content outside view box is not drawn
    SDL_RenderSetClipRect(renderer, &view_box);

    int content_y = view_y + 8 - static_cast<int>(about_scroll_y_);

    // Helper lambda to draw a header line
    auto draw_section = [&](const std::string& title) {
        FontRenderer::draw_text(renderer, title, view_x + 14, content_y, Colors::AccentGold, 1, false);
        content_y += 14;
    };

    // Helper lambda to draw a normal text line
    auto draw_line = [&](const std::string& line, SDL_Color col = Colors::TextWhite) {
        FontRenderer::draw_text(renderer, line, view_x + 14, content_y, col, 1, false);
        content_y += 11;
    };

    // --- Content Section 1: Developer ---
    draw_section("--- DEVELOPER PROFILE ---");
    draw_line("Name:    Viktor Titov (@nukdokplex)");
    draw_line("Bio:     \"A quiet guy. A living matter on cliffs.\"");
    draw_line("GitHub:  https://github.com/nukdokplex");
    draw_line("Email:   nukdokplex@ndplex.win");
    content_y += 8;

    // --- Content Section 2: Technical Architecture ---
    draw_section("--- TECHNICAL STACK ---");
    draw_line("Platform:    Sony PlayStation Portable (PSP) & Host PC");
    draw_line("Language:    C++20 Standard");
    draw_line("Rendering:   SDL2 (Hardware Accelerated PSP GE / OpenGL)");
    draw_line("Audio:       dr_mp3 streaming + Hardware PCM (44.1 kHz Stereo)");
    draw_line("Packaging:   Nix Flakes (flake-parts) + pspdev-nix");
    draw_line("Compilers:   psp-gcc 15.2.0 (MIPS Allegrex) / GCC (Host)");
    draw_line("Core Design: Zero-dependency algorithmic solvers & generators");
    draw_line("Architecture:Modular Plugin Interface (IGameModule / Registry)");
    content_y += 8;

    // --- Content Section 3: Soundtrack & Licensing ---
    draw_section("--- SOUNDTRACK (100% ROYALTY-FREE / CC0) ---");
    draw_line("Music by: MintoDog (OpenGameArt.org)");
    draw_line("License:  Creative Commons Zero (CC0) / Public Domain");
    draw_line("1. Cozy Puzzle Title");
    draw_line("   Author: MintoDog | License: CC0", Colors::TextDim);
    draw_line("2. Cozy Puzzle In-Game 1");
    draw_line("   Author: MintoDog | License: CC0", Colors::TextDim);
    draw_line("3. Cozy Puzzle In-Game 2");
    draw_line("   Author: MintoDog | License: CC0", Colors::TextDim);
    draw_line("Interactive SFX: Soft-synthesizer chords & placement chimes");
    content_y += 8;

    // --- Content Section 4: License & Disclaimer ---
    draw_section("--- PROJECT LICENSE & TRADEMARKS ---");
    draw_line("License: MIT License (Free & Open Source)");
    draw_line("Copyright (c) 2026 Oh PSP Contributors");
    draw_line("This project is a clean, independent logic puzzle suite.");
    draw_line("No proprietary trademarks or third-party assets are used.");
    content_y += 14;

    // Reset scissor rect
    SDL_RenderSetClipRect(renderer, nullptr);

    // 3. Scrollbar Indicator
    float total_content_h = static_cast<float>(content_y + static_cast<int>(about_scroll_y_) - view_y);
    float max_scroll = std::max(1.0f, total_content_h - view_h);

    int bar_track_h = view_h - 8;
    int thumb_h = std::max(20, static_cast<int>(bar_track_h * (static_cast<float>(view_h) / total_content_h)));
    int thumb_y = view_y + 4 + static_cast<int>((bar_track_h - thumb_h) * (about_scroll_y_ / max_scroll));

    SDL_Rect thumb_rect{view_x + view_w - 8, thumb_y, 4, thumb_h};
    RenderUtils::draw_rounded_rect(renderer, thumb_rect, 2, Colors::Blue);

    // 4. Footer controls
    std::string footer = "[Up/Down] Scroll   [L/R] Track   [Tri] Mute   [O] Back to Menu";
    FontRenderer::draw_text_centered(renderer, footer, 240, 252, Colors::TextDim, 1, false);
}

} // namespace ohpsp::menu
