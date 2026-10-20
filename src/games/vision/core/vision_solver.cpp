#include "vision_solver.hpp"
#include <vector>

namespace ohpsp::vision {

bool Solver::find_hint(const Board& board, Hint& hint) {
    int n = board.size();

    // 1. Clue reached: all open rays from this NumberedDot must be closed with Walls
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            Cell cell = board.get(x, y);
            if (cell.type == TileType::NumberedDot) {
                TileSightInfo info = board.compute_sight(x, y);
                if (info.total_confirmed == cell.number && info.total_unknowns > 0) {
                    for (int d = 0; d < 4; ++d) {
                        if (info.rays[d].unknowns > 0) {
                            hint.x = info.rays[d].first_unknown_x;
                            hint.y = info.rays[d].first_unknown_y;
                            hint.suggested_type = TileType::Wall;
                            hint.type = HintType::ValueReachedCloseWalls;
                            hint.description = "Number already sees all its dots; close remaining rays with walls.";
                            return true;
                        }
                    }
                }
            }
        }
    }

    // 2. All unknowns required: total_max_possible == number -> all unknowns must be Dots!
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            Cell cell = board.get(x, y);
            if (cell.type == TileType::NumberedDot) {
                TileSightInfo info = board.compute_sight(x, y);
                if (info.total_max_possible == cell.number && info.total_unknowns > 0) {
                    for (int d = 0; d < 4; ++d) {
                        if (info.rays[d].unknowns > 0) {
                            hint.x = info.rays[d].first_unknown_x;
                            hint.y = info.rays[d].first_unknown_y;
                            hint.suggested_type = TileType::Dot;
                            hint.type = HintType::AllUnknownsRequiredDots;
                            hint.description = "Number needs every possible remaining space to reach its count.";
                            return true;
                        }
                    }
                }
            }
        }
    }

    // 3. Single direction expand: only one ray has room to expand and number not reached
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            Cell cell = board.get(x, y);
            if (cell.type == TileType::NumberedDot) {
                TileSightInfo info = board.compute_sight(x, y);
                if (info.total_confirmed < cell.number) {
                    int expandable_dirs = 0;
                    int last_dir = -1;
                    for (int d = 0; d < 4; ++d) {
                        if (info.rays[d].unknowns > 0) {
                            expandable_dirs++;
                            last_dir = d;
                        }
                    }
                    if (expandable_dirs == 1 && last_dir != -1) {
                        hint.x = info.rays[last_dir].first_unknown_x;
                        hint.y = info.rays[last_dir].first_unknown_y;
                        hint.suggested_type = TileType::Dot;
                        hint.type = HintType::SingleDirectionExpand;
                        hint.description = "Only one direction can expand to satisfy this number.";
                        return true;
                    }
                }
            }
        }
    }

    // 4. Would exceed: test placing a Dot at an unknown; if it causes any NumberedDot to exceed, it must be Wall
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (board.get_type(x, y) == TileType::Unknown) {
                Board test = board;
                test.set_fixed(x, y, TileType::Dot, 0, false);
                if (test.has_violations()) {
                    hint.x = x;
                    hint.y = y;
                    hint.suggested_type = TileType::Wall;
                    hint.type = HintType::WouldExceedBlockWall;
                    hint.description = "Placing a dot here would exceed a visible count.";
                    return true;
                }
            }
        }
    }

    // 5. Isolated check: test placing a Wall at an unknown; if it causes violation, must be Dot
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (board.get_type(x, y) == TileType::Unknown) {
                Board test = board;
                test.set_fixed(x, y, TileType::Wall, 0, false);
                if (test.has_violations()) {
                    hint.x = x;
                    hint.y = y;
                    hint.suggested_type = TileType::Dot;
                    hint.type = HintType::LogicalDeduction;
                    hint.description = "Placing a wall here would make a number impossible to satisfy.";
                    return true;
                }
            }
        }
    }

    return false;
}

bool Solver::apply_logical_step(Board& board, Hint* out_hint) {
    Hint hint;
    if (find_hint(board, hint)) {
        if (out_hint) *out_hint = hint;
        board.set_type(hint.x, hint.y, hint.suggested_type);
        return true;
    }
    return false;
}

bool Solver::solve_pure_logic(Board& board) {
    while (board.empty_count() > 0) {
        if (!apply_logical_step(board)) {
            return false;
        }
    }
    return board.is_complete();
}

bool Solver::backtrack(Board& board, int& count, int limit) {
    if (count >= limit) return true;

    int n = board.size();
    int best_x = -1;
    int best_y = -1;

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (board.get_type(x, y) == TileType::Unknown) {
                best_x = x;
                best_y = y;
                break;
            }
        }
        if (best_x != -1) break;
    }

    if (best_x == -1) {
        if (board.is_complete()) {
            count++;
        }
        return count >= limit;
    }

    // Try Dot first, then Wall
    for (TileType type : {TileType::Dot, TileType::Wall}) {
        board.set_fixed(best_x, best_y, type, 0, false);
        if (!board.has_violations()) {
            if (backtrack(board, count, limit)) {
                if (count >= limit) {
                    board.set_fixed(best_x, best_y, TileType::Unknown, 0, false);
                    return true;
                }
            }
        }
        board.set_fixed(best_x, best_y, TileType::Unknown, 0, false);
    }

    return false;
}

int Solver::count_solutions(const Board& board, int limit) {
    if (board.has_violations()) return 0;
    Board copy = board;
    int count = 0;
    backtrack(copy, count, limit);
    return count;
}

static bool solve_first(Board& board) {
    int n = board.size();
    int best_x = -1;
    int best_y = -1;

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (board.get_type(x, y) == TileType::Unknown) {
                best_x = x;
                best_y = y;
                break;
            }
        }
        if (best_x != -1) break;
    }

    if (best_x == -1) {
        return board.is_complete();
    }

    for (TileType type : {TileType::Dot, TileType::Wall}) {
        board.set_fixed(best_x, best_y, type, 0, false);
        if (!board.has_violations()) {
            if (solve_first(board)) {
                return true;
            }
        }
        board.set_fixed(best_x, best_y, TileType::Unknown, 0, false);
    }

    return false;
}

bool Solver::solve(Board& board) {
    if (board.has_violations()) return false;
    return solve_first(board);
}

} // namespace ohpsp::vision
