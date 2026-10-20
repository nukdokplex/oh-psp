#pragma once

#include <SDL.h>

namespace ohpsp::engine {

struct InputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    bool action_a = false; // Cross / Enter / Space
    bool action_b = false; // Circle / Esc / Backspace
    bool action_x = false; // Square / C
    bool action_y = false; // Triangle / V / H (Hint)

    bool start = false;
    bool select = false;
    bool l_trigger = false;
    bool r_trigger = false;

    bool quit = false;

    // Pressed this frame (edge-triggered)
    bool pressed_up = false;
    bool pressed_down = false;
    bool pressed_left = false;
    bool pressed_right = false;

    bool pressed_a = false;
    bool pressed_b = false;
    bool pressed_x = false;
    bool pressed_y = false;

    bool pressed_start = false;
    bool pressed_select = false;
    bool pressed_l = false;
    bool pressed_r = false;

    // Analog stick & desktop-style mouse pointer
    float stick_x = 0.0f;
    float stick_y = 0.0f;
    float pointer_x = 240.0f;
    float pointer_y = 136.0f;
    bool pointer_moved = false;
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    void init();
    void set_window_scale(float scale_x, float scale_y) {
        scale_x_ = scale_x > 0.0f ? scale_x : 1.0f;
        scale_y_ = scale_y > 0.0f ? scale_y : 1.0f;
    }
    void update(float dt);
    const InputState& state() const { return current_state_; }

private:
    InputState current_state_{};
    InputState prev_state_{};

    float stick_raw_x_ = 0.0f;
    float stick_raw_y_ = 0.0f;
    float pointer_x_ = 240.0f;
    float pointer_y_ = 136.0f;
    float prev_pointer_x_ = 240.0f;
    float prev_pointer_y_ = 136.0f;
    float scale_x_ = 1.0f;
    float scale_y_ = 1.0f;

    SDL_GameController* controller_ = nullptr;
    SDL_Joystick* joystick_ = nullptr;

    void process_event(const SDL_Event& event);
    void update_edges();
};

} // namespace ohpsp::engine
