#pragma once

#include "vision_board.hpp"
#include <cstdint>

namespace ohpsp::vision {

class Generator {
public:
    static Board generate_full_board(int size, uint32_t seed);
    static Board generate_puzzle(int size, uint32_t seed, bool pure_logic_only = true);
};

} // namespace ohpsp::vision
