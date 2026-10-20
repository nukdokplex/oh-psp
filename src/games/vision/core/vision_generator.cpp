#include "vision_generator.hpp"
#include "vision_solver.hpp"
#include <random>
#include <algorithm>
#include <vector>

namespace ohpsp::vision {

Board Generator::generate_full_board(int size, uint32_t seed) {
    std::mt19937 rng(seed);
    int num_walls = size + (size >= 6 ? (size / 3) : 0);

    for (int attempt = 0; attempt < 200; ++attempt) {
        Board board(size);

        std::vector<std::pair<int, int>> all_cells;
        all_cells.reserve(size * size);
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                all_cells.emplace_back(x, y);
            }
        }
        std::shuffle(all_cells.begin(), all_cells.end(), rng);

        // Place walls
        for (int i = 0; i < num_walls; ++i) {
            board.set_fixed(all_cells[i].first, all_cells[i].second, TileType::Wall, 0, false);
        }
        // Fill rest with dots
        for (int i = num_walls; i < size * size; ++i) {
            board.set_fixed(all_cells[i].first, all_cells[i].second, TileType::Dot, 0, false);
        }

        // Validate: every dot must see at least 1 other dot
        bool valid = true;
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                if (board.get_type(x, y) == TileType::Dot) {
                    TileSightInfo info = board.compute_sight(x, y);
                    if (info.total_confirmed < 1) {
                        valid = false;
                        break;
                    }
                }
            }
            if (!valid) break;
        }

        if (valid) {
            // Assign sight numbers
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    if (board.get_type(x, y) == TileType::Dot) {
                        TileSightInfo info = board.compute_sight(x, y);
                        board.set_fixed(x, y, TileType::NumberedDot, info.total_confirmed, true);
                    }
                }
            }
            return board;
        }
    }

    // Fallback simple board if attempts exceeded
    Board fallback(size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (x == y && x < num_walls) {
                fallback.set_fixed(x, y, TileType::Wall, 0, false);
            } else {
                fallback.set_fixed(x, y, TileType::Dot, 0, false);
            }
        }
    }
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (fallback.get_type(x, y) == TileType::Dot) {
                TileSightInfo info = fallback.compute_sight(x, y);
                fallback.set_fixed(x, y, TileType::NumberedDot, info.total_confirmed, true);
            }
        }
    }
    return fallback;
}

Board Generator::generate_puzzle(int size, uint32_t seed, bool pure_logic_only) {
    std::mt19937 rng(seed);
    Board full = generate_full_board(size, seed);
    Board puzzle(size);

    // Initial setup: numbered dots are set, walls become Unknown
    std::vector<std::pair<int, int>> numbered_positions;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            Cell c = full.get(x, y);
            if (c.type == TileType::NumberedDot) {
                puzzle.set_fixed(x, y, TileType::NumberedDot, c.number, true);
                numbered_positions.emplace_back(x, y);
            } else {
                puzzle.set_fixed(x, y, TileType::Unknown, 0, false);
            }
        }
    }

    // Try removing numbers from some dots to leave them as Unknown dots
    std::shuffle(numbered_positions.begin(), numbered_positions.end(), rng);

    for (const auto& [x, y] : numbered_positions) {
        Cell original = puzzle.get(x, y);
        puzzle.set_fixed(x, y, TileType::Unknown, 0, false);

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
            puzzle.set_fixed(x, y, original.type, original.number, true);
        }
    }

    return puzzle;
}

} // namespace ohpsp::vision
