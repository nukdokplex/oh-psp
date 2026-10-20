#include <cassert>
#include <iostream>
#include "vision_board.hpp"
#include "vision_solver.hpp"
#include "vision_generator.hpp"

using namespace ohpsp::vision;

void test_board_basics() {
    Board board(4);
    assert(board.size() == 4);
    assert(board.empty_count() == 16);

    board.set_fixed(0, 0, TileType::NumberedDot, 3, true);
    assert(board.get_type(0, 0) == TileType::NumberedDot);
    assert(board.get_number(0, 0) == 3);
    assert(board.is_fixed(0, 0));

    // Trying to set fixed should fail
    assert(!board.set_type(0, 0, TileType::Wall));

    // Cycle non-fixed
    board.cycle_cell(1, 0); // Unknown -> Dot
    assert(board.get_type(1, 0) == TileType::Dot);
    board.cycle_cell(1, 0); // Dot -> Wall
    assert(board.get_type(1, 0) == TileType::Wall);
    board.cycle_cell(1, 0); // Wall -> Unknown
    assert(board.get_type(1, 0) == TileType::Unknown);

    std::cout << "[PASS] test_board_basics" << std::endl;
}

void test_sight_calculation() {
    Board board(4);
    // Row 0: [Dot(3), Dot, Dot, Wall]
    board.set_fixed(0, 0, TileType::NumberedDot, 3, true);
    board.set_fixed(1, 0, TileType::Dot, 0, false);
    board.set_fixed(2, 0, TileType::Dot, 0, false);
    board.set_fixed(3, 0, TileType::Wall, 0, false);

    // Col 0: [Dot(3), Dot, Wall, Wall]
    board.set_fixed(0, 1, TileType::Dot, 0, false);
    board.set_fixed(0, 2, TileType::Wall, 0, false);
    board.set_fixed(0, 3, TileType::Wall, 0, false);

    TileSightInfo info = board.compute_sight(0, 0);
    // Right ray: sees (1,0) and (2,0) before wall at (3,0) -> 2 dots
    // Down ray: sees (0,1) before wall at (0,2) -> 1 dot
    // Up & Left: 0 dots
    // Total confirmed = 3 dots!
    assert(info.total_confirmed == 3);
    assert(board.is_tile_satisfied(0, 0));

    std::cout << "[PASS] test_sight_calculation" << std::endl;
}

void test_hints() {
    Board board(4);
    board.set_fixed(0, 0, TileType::NumberedDot, 1, true);
    board.set_fixed(1, 0, TileType::Dot, 0, false);
    // Number 1 already sees (1, 0), so down ray at (0, 1) must be a Wall!
    Hint hint;
    assert(Solver::find_hint(board, hint));
    assert(hint.x == 0 && hint.y == 1);
    assert(hint.suggested_type == TileType::Wall);
    assert(hint.type == HintType::ValueReachedCloseWalls);

    std::cout << "[PASS] test_hints" << std::endl;
}

void test_solver_and_generator() {
    uint32_t seed = 999;
    Board full = Generator::generate_full_board(4, seed);
    assert(full.is_complete());
    assert(!full.has_violations());

    Board puzzle = Generator::generate_puzzle(4, seed, true);
    assert(puzzle.empty_count() > 0);
    assert(Solver::count_solutions(puzzle, 2) == 1);

    Board solved = puzzle;
    assert(Solver::solve(solved));
    assert(solved.is_complete());

    // 5x5 test
    Board puzzle5 = Generator::generate_puzzle(5, 54321, true);
    assert(puzzle5.empty_count() > 0);
    assert(Solver::count_solutions(puzzle5, 2) == 1);

    Board solved5 = puzzle5;
    assert(Solver::solve_pure_logic(solved5));
    assert(solved5.is_complete());

    std::cout << "[PASS] test_solver_and_generator" << std::endl;
}

int main() {
    test_board_basics();
    test_sight_calculation();
    test_hints();
    test_solver_and_generator();
    std::cout << "All Vision core tests passed!" << std::endl;
    return 0;
}
