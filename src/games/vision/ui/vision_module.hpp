#pragma once

#include "engine/registry.hpp"
#include "games/vision/core/vision_board.hpp"
#include "games/vision/core/vision_solver.hpp"

namespace ohpsp::vision {

class VisionModule : public engine::IGameModule {
public:
    VisionModule();
    ~VisionModule() override = default;

    const char* get_id() const override { return "vision"; }
    const char* get_title() const override { return "Vision"; }
    const char* get_tagline() const override { return "Line of sight: blue dots see along rays, walls block"; }
    std::vector<engine::GamePreset> get_presets() const override;

    void start_game(const engine::GamePreset& preset) override;
    void restart_game() override;
    void handle_input(const engine::InputState& input) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    bool is_completed() const override { return completed_; }
    bool wants_exit_to_menu() const override { return exit_to_menu_; }

private:
    engine::GamePreset current_preset_;
    Board board_;
    Board solution_;
    int cursor_x_ = 0;
    int cursor_y_ = 0;
    bool completed_ = false;
    bool exit_to_menu_ = false;

    float pulse_timer_ = 0.0f;
    float elapsed_time_ = 0.0f;
    int victory_choice_ = 0; // 0: Next Puzzle, 1: Back to Menu

    Hint current_hint_;
    bool hint_active_ = false;
    float hint_timer_ = 0.0f;

    void cycle_current_cell(bool forward);
    void request_hint();
    void next_puzzle();
    void render_victory_modal(SDL_Renderer* renderer);
};

} // namespace ohpsp::vision
