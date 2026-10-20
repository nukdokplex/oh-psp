#pragma once

#include "vision_board.hpp"
#include <string>

namespace ohpsp::vision {

enum class HintType {
    None,
    ValueReachedCloseWalls,
    AllUnknownsRequiredDots,
    SingleDirectionExpand,
    WouldExceedBlockWall,
    IsolatedTileMustBeWall,
    LogicalDeduction
};

struct Hint {
    int x = -1;
    int y = -1;
    TileType suggested_type = TileType::Unknown;
    HintType type = HintType::None;
    std::string description;
};

class Solver {
public:
    static bool solve(Board& board);
    static int count_solutions(const Board& board, int limit = 2);
    static bool find_hint(const Board& board, Hint& hint);

    static bool apply_logical_step(Board& board, Hint* out_hint = nullptr);
    static bool solve_pure_logic(Board& board);

private:
    static bool backtrack(Board& board, int& count, int limit);
};

} // namespace ohpsp::vision
