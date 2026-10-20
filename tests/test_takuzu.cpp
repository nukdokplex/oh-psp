#include <cassert>
#include <iostream>
#include "takuzu_board.hpp"
#include "takuzu_solver.hpp"
#include "takuzu_generator.hpp"

using namespace ohpsp::takuzu;

void test_board_basics() {
    Board board(4);
    assert(board.size() == 4);
    assert(board.empty_count() == 16);

    board.set_fixed(0, 0, Color::Blue, true);
    assert(board.get_color(0, 0) == Color::Blue);
    assert(board.is_fixed(0, 0));

    // Trying to overwrite fixed cell with set() should fail
    assert(!board.set(0, 0, Color::Red));
    assert(board.get_color(0, 0) == Color::Blue);

    // Non-fixed cell
    board.set(1, 0, Color::Red);
    assert(board.get_color(1, 0) == Color::Red);
    assert(!board.is_fixed(1, 0));

    board.clear_cell(1, 0);
    assert(board.get_color(1, 0) == Color::None);

    std::cout << "[PASS] test_board_basics" << std::endl;
}

void test_validity_rules() {
    Board board(4);
    // 3 in a row
    board.set_fixed(0, 0, Color::Blue, false);
    board.set_fixed(1, 0, Color::Blue, false);
    board.set_fixed(2, 0, Color::Blue, false);
    assert(!board.is_row_valid(0));
    assert(!board.is_valid());

    board.set_fixed(2, 0, Color::Red, false);
    assert(board.is_row_valid(0));

    // Too many of one color in row (for 4x4, max is 2)
    board.set_fixed(3, 0, Color::Blue, false);
    // Now row 0 has: Blue, Blue, Red, Blue -> 3 Blues!
    assert(!board.is_row_valid(0));

    std::cout << "[PASS] test_validity_rules" << std::endl;
}

void test_hints() {
    Board board(4);
    // Trio adjacent: [Blue, Blue, None, None]
    board.set_fixed(0, 0, Color::Blue, true);
    board.set_fixed(1, 0, Color::Blue, true);

    Hint hint;
    assert(Solver::find_hint(board, hint));
    assert(hint.x == 2 && hint.y == 0);
    assert(hint.color == Color::Red);
    assert(hint.type == HintType::TrioAdjacent);

    // Trio gap: [Red, None, Red, None]
    Board board2(4);
    board2.set_fixed(0, 1, Color::Red, true);
    board2.set_fixed(2, 1, Color::Red, true);
    assert(Solver::find_hint(board2, hint));
    assert(hint.x == 1 && hint.y == 1);
    assert(hint.color == Color::Blue);
    assert(hint.type == HintType::TrioMiddle);

    // Balance: [Blue, Blue, None, None] with x=2 already Red
    Board board3(4);
    board3.set_fixed(0, 2, Color::Blue, true);
    board3.set_fixed(1, 2, Color::Blue, true);
    board3.set_fixed(2, 2, Color::Red, true);
    assert(Solver::find_hint(board3, hint));
    assert(hint.x == 3 && hint.y == 2);
    assert(hint.color == Color::Red);

    std::cout << "[PASS] test_hints" << std::endl;
}

void test_solver_and_generator() {
    uint32_t seed = 42;
    Board full = Generator::generate_full_board(4, seed);
    assert(full.is_complete());
    assert(full.is_valid());
    assert(full.rows_unique());
    assert(full.cols_unique());

    Board puzzle = Generator::generate_puzzle(4, seed, true);
    assert(puzzle.empty_count() > 0);
    assert(Solver::count_solutions(puzzle, 2) == 1);

    Board solved = puzzle;
    assert(Solver::solve(solved));
    assert(solved.is_complete());
    assert(solved.is_valid());

    // 6x6 test
    Board puzzle6 = Generator::generate_puzzle(6, 12345, true);
    assert(puzzle6.empty_count() > 0);
    assert(Solver::count_solutions(puzzle6, 2) == 1);

    Board solved6 = puzzle6;
    assert(Solver::solve_pure_logic(solved6));
    assert(solved6.is_complete());

    std::cout << "[PASS] test_solver_and_generator" << std::endl;
}

int main() {
    test_board_basics();
    test_validity_rules();
    test_hints();
    test_solver_and_generator();
    std::cout << "All Takuzu core tests passed!" << std::endl;
    return 0;
}
