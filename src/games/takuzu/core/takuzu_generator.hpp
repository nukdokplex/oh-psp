#pragma once

#include "takuzu_board.hpp"
#include <cstdint>

namespace ohpsp::takuzu {

class Generator {
public:
    static Board generate_full_board(int size, uint32_t seed);
    static Board generate_puzzle(int size, uint32_t seed, bool pure_logic_only = true);
};

} // namespace ohpsp::takuzu
