#include "takuzu_generator.hpp"
#include "takuzu_solver.hpp"
#include <random>
#include <algorithm>
#include <numeric>

namespace ohpsp::takuzu {

static void generate_row_combos(int n, int idx, int blue_count, int red_count,
                                std::vector<Color>& current,
                                std::vector<std::vector<Color>>& results) {
    int half = n / 2;
    if (idx == n) {
        if (blue_count == half && red_count == half) {
            results.push_back(current);
        }
        return;
    }

    // Try Blue
    if (blue_count < half) {
        if (idx < 2 || !(current[idx - 1] == Color::Blue && current[idx - 2] == Color::Blue)) {
            current[idx] = Color::Blue;
            generate_row_combos(n, idx + 1, blue_count + 1, red_count, current, results);
        }
    }

    // Try Red
    if (red_count < half) {
        if (idx < 2 || !(current[idx - 1] == Color::Red && current[idx - 2] == Color::Red)) {
            current[idx] = Color::Red;
            generate_row_combos(n, idx + 1, blue_count, red_count + 1, current, results);
        }
    }
}

static bool fill_rows(Board& board, int y, const std::vector<std::vector<Color>>& combos,
                      std::vector<bool>& used, std::mt19937& rng) {
    int n = board.size();
    if (y == n) {
        return board.is_complete();
    }

    std::vector<int> indices(combos.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (int idx : indices) {
        if (used[idx]) continue;

        const auto& row = combos[idx];
        for (int x = 0; x < n; ++x) {
            board.set_fixed(x, y, row[x], false);
        }

        bool valid = true;
        for (int x = 0; x < n; ++x) {
            if (!board.is_col_valid(x)) {
                valid = false;
                break;
            }
        }

        if (valid) {
            used[idx] = true;
            if (fill_rows(board, y + 1, combos, used, rng)) {
                return true;
            }
            used[idx] = false;
        }

        for (int x = 0; x < n; ++x) {
            board.set_fixed(x, y, Color::None, false);
        }
    }

    return false;
}

Board Generator::generate_full_board(int size, uint32_t seed) {
    std::mt19937 rng(seed);
    std::vector<std::vector<Color>> combos;
    std::vector<Color> current(size);
    generate_row_combos(size, 0, 0, 0, current, combos);

    Board board(size);
    std::vector<bool> used(combos.size(), false);
    fill_rows(board, 0, combos, used, rng);
    return board;
}

Board Generator::generate_puzzle(int size, uint32_t seed, bool pure_logic_only) {
    std::mt19937 rng(seed);
    Board full = generate_full_board(size, seed);
    Board puzzle = full;

    std::vector<std::pair<int, int>> positions;
    positions.reserve(size * size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            positions.emplace_back(x, y);
        }
    }
    std::shuffle(positions.begin(), positions.end(), rng);

    for (const auto& [x, y] : positions) {
        Color original = puzzle.get_color(x, y);
        puzzle.set_fixed(x, y, Color::None, false);

        bool can_remove = false;
        if (pure_logic_only) {
            Board test = puzzle;
            if (Solver::solve_pure_logic(test) && test.is_complete()) {
                can_remove = true;
            }
        } else {
            if (Solver::count_solutions(puzzle, 2) == 1) {
                can_remove = true;
            }
        }

        if (!can_remove) {
            puzzle.set_fixed(x, y, original, false);
        }
    }

    // Set remaining clues as fixed
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            Color c = puzzle.get_color(x, y);
            if (c != Color::None) {
                puzzle.set_fixed(x, y, c, true);
            }
        }
    }

    return puzzle;
}

} // namespace ohpsp::takuzu
