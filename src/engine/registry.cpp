#include "registry.hpp"

namespace ohpsp::engine {

GameRegistry& GameRegistry::instance() {
    static GameRegistry reg;
    return reg;
}

void GameRegistry::register_module(std::shared_ptr<IGameModule> module) {
    if (!module) return;
    for (const auto& existing : modules_) {
        if (std::string(existing->get_id()) == module->get_id()) {
            return;
        }
    }
    modules_.push_back(module);
}

const std::vector<std::shared_ptr<IGameModule>>& GameRegistry::get_modules() const {
    return modules_;
}

std::shared_ptr<IGameModule> GameRegistry::get_module_by_id(const std::string& id) const {
    for (const auto& module : modules_) {
        if (std::string(module->get_id()) == id) {
            return module;
        }
    }
    return nullptr;
}

} // namespace ohpsp::engine
