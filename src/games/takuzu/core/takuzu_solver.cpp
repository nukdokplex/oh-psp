#include "takuzu_solver.hpp"
#include <vector>

namespace ohpsp::takuzu {

bool Solver::find_hint(const Board& board, Hint& hint) {
    int n = board.size();
    int half = n / 2;

    // 1. Trio Adjacent Check (e.g. BB_ -> R, or _BB -> R)
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (board.get_color(x, y) != Color::None) continue;

            // Horizontal adjacent to the left: [x-2, x-1]
            if (x >= 2) {
                Color c1 = board.get_color(x - 1, y);
                Color c2 = board.get_color(x - 2, y);
                if (c1 != Color::None && c1 == c2) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = opposite_color(c1);
                    hint.type = HintType::TrioAdjacent;
                    hint.description = "Cannot place three identical colors in a row.";
                    return true;
                }
            }
            // Horizontal adjacent to the right: [x+1, x+2]
            if (x + 2 < n) {
                Color c1 = board.get_color(x + 1, y);
                Color c2 = board.get_color(x + 2, y);
                if (c1 != Color::None && c1 == c2) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = opposite_color(c1);
                    hint.type = HintType::TrioAdjacent;
                    hint.description = "Cannot place three identical colors in a row.";
                    return true;
                }
            }
            // Vertical adjacent above: [y-2, y-1]
            if (y >= 2) {
                Color c1 = board.get_color(x, y - 1);
                Color c2 = board.get_color(x, y - 2);
                if (c1 != Color::None && c1 == c2) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = opposite_color(c1);
                    hint.type = HintType::TrioAdjacent;
                    hint.description = "Cannot place three identical colors in a column.";
                    return true;
                }
            }
            // Vertical adjacent below: [y+1, y+2]
            if (y + 2 < n) {
                Color c1 = board.get_color(x, y + 1);
                Color c2 = board.get_color(x, y + 2);
                if (c1 != Color::None && c1 == c2) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = opposite_color(c1);
                    hint.type = HintType::TrioAdjacent;
                    hint.description = "Cannot place three identical colors in a column.";
                    return true;
                }
            }
        }
    }

    // 2. Trio Middle Check (e.g. B_B -> R)
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (board.get_color(x, y) != Color::None) continue;

            // Horizontal gap: [x-1, x, x+1]
            if (x >= 1 && x + 1 < n) {
                Color cl = board.get_color(x - 1, y);
                Color cr = board.get_color(x + 1, y);
                if (cl != Color::None && cl == cr) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = opposite_color(cl);
                    hint.type = HintType::TrioMiddle;
                    hint.description = "Prevent a trio between two matching colors.";
                    return true;
                }
            }
            // Vertical gap: [y-1, y, y+1]
            if (y >= 1 && y + 1 < n) {
                Color cu = board.get_color(x, y - 1);
                Color cd = board.get_color(x, y + 1);
                if (cu != Color::None && cu == cd) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = opposite_color(cu);
                    hint.type = HintType::TrioMiddle;
                    hint.description = "Prevent a trio between two matching colors.";
                    return true;
                }
            }
        }
    }

    // 3. Balance Check (Row)
    for (int y = 0; y < n; ++y) {
        int blue = board.count_row_color(y, Color::Blue);
        int red = board.count_row_color(y, Color::Red);
        if (blue == half && red < half) {
            for (int x = 0; x < n; ++x) {
                if (board.get_color(x, y) == Color::None) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = Color::Red;
                    hint.type = HintType::RowBalance;
                    hint.description = "This row already has half blue cells; rest must be red.";
                    return true;
                }
            }
        } else if (red == half && blue < half) {
            for (int x = 0; x < n; ++x) {
                if (board.get_color(x, y) == Color::None) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = Color::Blue;
                    hint.type = HintType::RowBalance;
                    hint.description = "This row already has half red cells; rest must be blue.";
                    return true;
                }
            }
        }
    }

    // 4. Balance Check (Column)
    for (int x = 0; x < n; ++x) {
        int blue = board.count_col_color(x, Color::Blue);
        int red = board.count_col_color(x, Color::Red);
        if (blue == half && red < half) {
            for (int y = 0; y < n; ++y) {
                if (board.get_color(x, y) == Color::None) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = Color::Red;
                    hint.type = HintType::ColBalance;
                    hint.description = "This column already has half blue cells; rest must be red.";
                    return true;
                }
            }
        } else if (red == half && blue < half) {
            for (int y = 0; y < n; ++y) {
                if (board.get_color(x, y) == Color::None) {
                    hint.x = x;
                    hint.y = y;
                    hint.color = Color::Blue;
                    hint.type = HintType::ColBalance;
                    hint.description = "This column already has half red cells; rest must be blue.";
                    return true;
                }
            }
        }
    }

    // 5. Uniqueness deduction (Hypothetical testing for rows with 2 empty spaces)
    for (int y = 0; y < n; ++y) {
        std::vector<int> empty_cols;
        for (int x = 0; x < n; ++x) {
            if (board.get_color(x, y) == Color::None) {
                empty_cols.push_back(x);
            }
        }
        if (empty_cols.size() == 2) {
            int x1 = empty_cols[0];
            int x2 = empty_cols[1];
            Board test_b = board;
            test_b.set_fixed(x1, y, Color::Blue, false);
            test_b.set_fixed(x2, y, Color::Red, false);
            if (!test_b.is_valid()) {
                hint.x = x1;
                hint.y = y;
                hint.color = Color::Red;
                hint.type = HintType::RowUniqueness;
                hint.description = "Setting this to Blue would duplicate another row or cause a violation.";
                return true;
            }
            test_b.set_fixed(x1, y, Color::Red, false);
            test_b.set_fixed(x2, y, Color::Blue, false);
            if (!test_b.is_valid()) {
                hint.x = x1;
                hint.y = y;
                hint.color = Color::Blue;
                hint.type = HintType::RowUniqueness;
                hint.description = "Setting this to Red would duplicate another row or cause a violation.";
                return true;
            }
        }
    }

    return false;
}

bool Solver::apply_logical_step(Board& board, Hint* out_hint) {
    Hint hint;
    if (find_hint(board, hint)) {
        if (out_hint) {
            *out_hint = hint;
        }
        board.set(hint.x, hint.y, hint.color);
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
            if (board.get_color(x, y) == Color::None) {
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

    for (Color color : {Color::Blue, Color::Red}) {
        board.set_fixed(best_x, best_y, color, false);
        if (board.is_row_valid(best_y) && board.is_col_valid(best_x)) {
            // Also if row or col becomes complete, check uniqueness
            bool can_proceed = true;
            if (board.row_to_string(best_y).find('.') == std::string::npos && !board.rows_unique()) {
                can_proceed = false;
            }
            if (can_proceed && board.col_to_string(best_x).find('.') == std::string::npos && !board.cols_unique()) {
                can_proceed = false;
            }

            if (can_proceed) {
                if (backtrack(board, count, limit)) {
                    if (count >= limit) {
                        board.set_fixed(best_x, best_y, Color::None, false);
                        return true;
                    }
                }
            }
        }
        board.set_fixed(best_x, best_y, Color::None, false);
    }

    return false;
}

int Solver::count_solutions(const Board& board, int limit) {
    if (!board.is_valid()) return 0;
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
            if (board.get_color(x, y) == Color::None) {
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

    for (Color color : {Color::Blue, Color::Red}) {
        board.set_fixed(best_x, best_y, color, false);
        if (board.is_row_valid(best_y) && board.is_col_valid(best_x)) {
            bool can_proceed = true;
            if (board.row_to_string(best_y).find('.') == std::string::npos && !board.rows_unique()) {
                can_proceed = false;
            }
            if (can_proceed && board.col_to_string(best_x).find('.') == std::string::npos && !board.cols_unique()) {
                can_proceed = false;
            }

            if (can_proceed && solve_first(board)) {
                return true;
            }
        }
        board.set_fixed(best_x, best_y, Color::None, false);
    }

    return false;
}

bool Solver::solve(Board& board) {
    if (!board.is_valid()) return false;
    return solve_first(board);
}

} // namespace ohpsp::takuzu
