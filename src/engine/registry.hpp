#pragma once

#include <vector>
#include <string>
#include <memory>
#include <SDL.h>
#include "input.hpp"

namespace ohpsp::engine {

struct GamePreset {
    std::string name;
    int size = 4;
};

class IGameModule {
public:
    virtual ~IGameModule() = default;

    virtual const char* get_id() const = 0;
    virtual const char* get_title() const = 0;
    virtual const char* get_tagline() const = 0;
    virtual std::vector<GamePreset> get_presets() const = 0;

    virtual void start_game(const GamePreset& preset) = 0;
    virtual void restart_game() = 0;
    virtual void handle_input(const InputState& input) = 0;
    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual bool is_completed() const = 0;
    virtual bool wants_exit_to_menu() const = 0;
};

class GameRegistry {
public:
    static GameRegistry& instance();

    void register_module(std::shared_ptr<IGameModule> module);
    const std::vector<std::shared_ptr<IGameModule>>& get_modules() const;
    std::shared_ptr<IGameModule> get_module_by_id(const std::string& id) const;

private:
    GameRegistry() = default;
    std::vector<std::shared_ptr<IGameModule>> modules_;
};

} // namespace ohpsp::engine
