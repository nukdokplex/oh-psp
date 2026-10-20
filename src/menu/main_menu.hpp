#pragma once

#include "engine/registry.hpp"
#include <memory>

namespace ohpsp::menu {

enum class MenuState {
    GameSelect,
    PresetSelect,
    AboutScreen,
    InGame
};

class MainMenu {
public:
    MainMenu();

    void init();
    void handle_input(const engine::InputState& input);
    void update(float dt);
    void render(SDL_Renderer* renderer);

    bool wants_quit() const { return wants_quit_; }

private:
    MenuState state_ = MenuState::GameSelect;
    int selected_game_index_ = 0;
    int selected_preset_index_ = 0;
    bool wants_quit_ = false;
    float pulse_timer_ = 0.0f;

    // About screen scroll state
    float about_scroll_y_ = 0.0f;
    float target_scroll_y_ = 0.0f;

    std::shared_ptr<engine::IGameModule> active_game_ = nullptr;

    void render_about_screen(SDL_Renderer* renderer);
};

} // namespace ohpsp::menu
