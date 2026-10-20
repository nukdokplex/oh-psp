#include "engine/display.hpp"
#include "engine/input.hpp"
#include "engine/audio.hpp"
#include "engine/registry.hpp"
#include "engine/render_utils.hpp"
#include "games/takuzu/ui/takuzu_module.hpp"
#include "games/vision/ui/vision_module.hpp"
#include "menu/main_menu.hpp"
#include <cstring>

#if defined(PSP) || defined(__PSP__)
#include <unistd.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#define EMULATOR_DEVCTL__GET_HAS_DISPLAY 1
#define EMULATOR_DEVCTL__SEND_OUTPUT     2
#define EMULATOR_DEVCTL__IS_EMULATOR     3
#define EMULATOR_DEVCTL__EMIT_SCREENSHOT 0x20
#endif

int main(int argc, char* argv[]) {
    bool is_headless_test = false;

#if defined(PSP) || defined(__PSP__)
    // Change working directory to the directory where EBOOT is located
    if (argc > 0 && argv[0] != nullptr) {
        char dir[256];
        std::strncpy(dir, argv[0], sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = '\0';
        char* last_slash = std::strrchr(dir, '/');
        if (last_slash != nullptr) {
            *last_slash = '\0';
            chdir(dir);
            ohpsp::engine::AudioManager::instance().set_base_path(dir);
        }
    }

    uint32_t is_emu = 0;
    if (sceIoDevctl("emulator:", EMULATOR_DEVCTL__IS_EMULATOR, nullptr, 0, &is_emu, sizeof(is_emu)) == 0 && is_emu == 1) {
        uint32_t has_display = 1;
        if (sceIoDevctl("emulator:", EMULATOR_DEVCTL__GET_HAS_DISPLAY, nullptr, 0, &has_display, sizeof(has_display)) == 0) {
            if (has_display == 0) {
                is_headless_test = true;
            }
        }
    }
#endif

    ohpsp::engine::Display display;
    if (!display.init("Oh PSP", false, 2)) {
        return 1;
    }

    ohpsp::engine::InputManager input;
    input.init();
#if !defined(PSP) && !defined(__PSP__)
    input.set_window_scale(2.0f, 2.0f);
#endif

    // Initialize audio system (SDL2 Audio + dr_mp3 streaming)
    ohpsp::engine::AudioManager::instance().init();

    // Register all modular games in the suite
    auto& registry = ohpsp::engine::GameRegistry::instance();
    registry.register_module(std::make_shared<ohpsp::takuzu::TakuzuModule>());
    registry.register_module(std::make_shared<ohpsp::vision::VisionModule>());

    ohpsp::menu::MainMenu menu;
    menu.init();

    uint32_t last_time = SDL_GetTicks();
    bool running = true;
    int frame_count = 0;

    while (running) {
        uint32_t current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f; // Clamp delta time
        last_time = current_time;

        input.update(dt);

        if (input.state().quit || menu.wants_quit()) {
            running = false;
            break;
        }

        menu.handle_input(input.state());
        menu.update(dt);

        display.clear(34, 36, 43); // #22242b
        menu.render(display.renderer());
        ohpsp::engine::RenderUtils::draw_pointer(display.renderer(),
                                                 static_cast<int>(input.state().pointer_x),
                                                 static_cast<int>(input.state().pointer_y));
        display.present();

        frame_count++;

#if defined(PSP) || defined(__PSP__)
        // When running in headless automated test harness (PPSSPPHeadless without display)
        if (is_headless_test && frame_count >= 30) {
            sceIoDevctl("emulator:", EMULATOR_DEVCTL__EMIT_SCREENSHOT, nullptr, 0, nullptr, 0);
            running = false;
        }
#endif
    }

    ohpsp::engine::AudioManager::instance().shutdown();
    display.shutdown();

#if defined(PSP) || defined(__PSP__)
    if (is_headless_test) {
        sceKernelExitGame();
    }
#endif

    return 0;
}
