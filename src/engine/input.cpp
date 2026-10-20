#include "input.hpp"
#include <cmath>
#include <algorithm>

namespace ohpsp::engine {

InputManager::InputManager() = default;

InputManager::~InputManager() {
    if (controller_) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }
    if (joystick_) {
        SDL_JoystickClose(joystick_);
        joystick_ = nullptr;
    }
}

void InputManager::init() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller_ = SDL_GameControllerOpen(i);
            if (controller_) break;
        } else if (!joystick_) {
            joystick_ = SDL_JoystickOpen(i);
        }
    }
    if (!controller_ && !joystick_ && SDL_NumJoysticks() > 0) {
        joystick_ = SDL_JoystickOpen(0);
    }
}

void InputManager::update(float dt) {
    prev_state_ = current_state_;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        process_event(event);
    }

    // Integrate analog stick to desktop pointer
    float mag = std::sqrt(stick_raw_x_ * stick_raw_x_ + stick_raw_y_ * stick_raw_y_);
    constexpr float DEADZONE = 0.18f;
    if (mag > DEADZONE) {
        float norm_mag = std::min(1.0f, (mag - DEADZONE) / (1.0f - DEADZONE));
        float dir_x = stick_raw_x_ / mag;
        float dir_y = stick_raw_y_ / mag;
        constexpr float SPEED = 280.0f; // pixels per second across 480x272
        pointer_x_ += dir_x * norm_mag * SPEED * dt;
        pointer_y_ += dir_y * norm_mag * SPEED * dt;
    }
    pointer_x_ = std::clamp(pointer_x_, 0.0f, 479.0f);
    pointer_y_ = std::clamp(pointer_y_, 0.0f, 271.0f);

    float dx = pointer_x_ - prev_pointer_x_;
    float dy = pointer_y_ - prev_pointer_y_;
    current_state_.pointer_moved = (dx * dx + dy * dy) > 0.04f;
    prev_pointer_x_ = pointer_x_;
    prev_pointer_y_ = pointer_y_;

    current_state_.stick_x = stick_raw_x_;
    current_state_.stick_y = stick_raw_y_;
    current_state_.pointer_x = pointer_x_;
    current_state_.pointer_y = pointer_y_;

    update_edges();
}

void InputManager::process_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        current_state_.quit = true;
        return;
    }

    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        if (!controller_) {
            controller_ = SDL_GameControllerOpen(event.cdevice.which);
        }
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (controller_ && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))) {
            SDL_GameControllerClose(controller_);
            controller_ = nullptr;
        }
    }

    // Keyboard support
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool down = (event.type == SDL_KEYDOWN);
        switch (event.key.keysym.sym) {
            case SDLK_UP:
            case SDLK_w:
                current_state_.up = down;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                current_state_.down = down;
                break;
            case SDLK_LEFT:
            case SDLK_a:
                current_state_.left = down;
                break;
            case SDLK_RIGHT:
            case SDLK_d:
                current_state_.right = down;
                break;
            case SDLK_RETURN:
            case SDLK_SPACE:
            case SDLK_z:
                current_state_.action_a = down;
                break;
            case SDLK_ESCAPE:
            case SDLK_BACKSPACE:
            case SDLK_x:
                current_state_.action_b = down;
                break;
            case SDLK_c:
                current_state_.action_x = down;
                break;
            case SDLK_v:
            case SDLK_h:
                current_state_.action_y = down;
                break;
            case SDLK_TAB:
            case SDLK_p:
                current_state_.start = down;
                break;
            case SDLK_BACKQUOTE:
                current_state_.select = down;
                break;
            case SDLK_q:
            case SDLK_LEFTBRACKET:
                current_state_.l_trigger = down;
                break;
            case SDLK_e:
            case SDLK_RIGHTBRACKET:
                current_state_.r_trigger = down;
                break;
            default:
                break;
        }
    }

    // GameController support
    if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
        switch (event.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                current_state_.up = down;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                current_state_.down = down;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                current_state_.left = down;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                current_state_.right = down;
                break;
            case SDL_CONTROLLER_BUTTON_A: // PSP Cross
                current_state_.action_a = down;
                break;
            case SDL_CONTROLLER_BUTTON_B: // PSP Circle
                current_state_.action_b = down;
                break;
            case SDL_CONTROLLER_BUTTON_X: // PSP Square
                current_state_.action_x = down;
                break;
            case SDL_CONTROLLER_BUTTON_Y: // PSP Triangle
                current_state_.action_y = down;
                break;
            case SDL_CONTROLLER_BUTTON_START:
                current_state_.start = down;
                break;
            case SDL_CONTROLLER_BUTTON_BACK:
                current_state_.select = down;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                current_state_.l_trigger = down;
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                current_state_.r_trigger = down;
                break;
            default:
                break;
        }
    }

    // Native PSP Joystick / Button support
    // In psp-sdl2 (SDL2 for PSP), D-Pad buttons are mapped as buttons 6..9, and Select/Start as 10/11:
    // 0: Triangle, 1: Circle, 2: Cross, 3: Square, 4: L, 5: R
    // 6: Down, 7: Left, 8: Up, 9: Right, 10: Select, 11: Start
    if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP) {
        bool down = (event.type == SDL_JOYBUTTONDOWN);
        switch (event.jbutton.button) {
            case 0: // PSP Triangle
                current_state_.action_y = down;
                break;
            case 1: // PSP Circle
                current_state_.action_b = down;
                break;
            case 2: // PSP Cross
                current_state_.action_a = down;
                break;
            case 3: // PSP Square
                current_state_.action_x = down;
                break;
            case 4: // PSP L-Trigger
                current_state_.l_trigger = down;
                break;
            case 5: // PSP R-Trigger
                current_state_.r_trigger = down;
                break;
            case 6: // PSP D-Pad Down
                current_state_.down = down;
                break;
            case 7: // PSP D-Pad Left
                current_state_.left = down;
                break;
            case 8: // PSP D-Pad Up
                current_state_.up = down;
                break;
            case 9: // PSP D-Pad Right
                current_state_.right = down;
                break;
            case 10: // PSP Select
                current_state_.select = down;
                break;
            case 11: // PSP Start
                current_state_.start = down;
                break;
            default:
                break;
        }
    }

    if (event.type == SDL_JOYHATMOTION) {
        current_state_.up = (event.jhat.value & SDL_HAT_UP) != 0;
        current_state_.down = (event.jhat.value & SDL_HAT_DOWN) != 0;
        current_state_.left = (event.jhat.value & SDL_HAT_LEFT) != 0;
        current_state_.right = (event.jhat.value & SDL_HAT_RIGHT) != 0;
    }

    // Host PC Mouse support
    if (event.type == SDL_MOUSEMOTION) {
        pointer_x_ = static_cast<float>(event.motion.x) / scale_x_;
        pointer_y_ = static_cast<float>(event.motion.y) / scale_y_;
    } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        bool down = (event.type == SDL_MOUSEBUTTONDOWN);
        if (event.button.button == SDL_BUTTON_LEFT) {
            current_state_.action_a = down;
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            current_state_.action_x = down;
        }
    }

    // PSP Analog Stick & Generic Joystick Axis motion
    if (event.type == SDL_JOYAXISMOTION) {
        if (event.jaxis.axis == 0) {
            stick_raw_x_ = static_cast<float>(event.jaxis.value) / 32767.0f;
        } else if (event.jaxis.axis == 1) {
            stick_raw_y_ = static_cast<float>(event.jaxis.value) / 32767.0f;
        }
    }

    // GameController Left Stick Axis
    if (event.type == SDL_CONTROLLERAXISMOTION) {
        if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
            stick_raw_x_ = static_cast<float>(event.caxis.value) / 32767.0f;
        } else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            stick_raw_y_ = static_cast<float>(event.caxis.value) / 32767.0f;
        }
    }
}

void InputManager::update_edges() {
    current_state_.pressed_up = current_state_.up && !prev_state_.up;
    current_state_.pressed_down = current_state_.down && !prev_state_.down;
    current_state_.pressed_left = current_state_.left && !prev_state_.left;
    current_state_.pressed_right = current_state_.right && !prev_state_.right;

    current_state_.pressed_a = current_state_.action_a && !prev_state_.action_a;
    current_state_.pressed_b = current_state_.action_b && !prev_state_.action_b;
    current_state_.pressed_x = current_state_.action_x && !prev_state_.action_x;
    current_state_.pressed_y = current_state_.action_y && !prev_state_.action_y;

    current_state_.pressed_start = current_state_.start && !prev_state_.start;
    current_state_.pressed_select = current_state_.select && !prev_state_.select;
    current_state_.pressed_l = current_state_.l_trigger && !prev_state_.l_trigger;
    current_state_.pressed_r = current_state_.r_trigger && !prev_state_.r_trigger;
}

} // namespace ohpsp::engine
