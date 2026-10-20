#include "takuzu_board.hpp"
#include <algorithm>

namespace ohpsp::takuzu {

Board::Board(int size)
    : size_(size), cells_(size * size) {}

void Board::resize(int size) {
    size_ = size;
    cells_.assign(size * size, Cell{});
}

Cell Board::get(int x, int y) const {
    if (x < 0 || x >= size_ || y < 0 || y >= size_) {
        return Cell{};
    }
    return cells_[index(x, y)];
}

Color Board::get_color(int x, int y) const {
    return get(x, y).color;
}

bool Board::is_fixed(int x, int y) const {
    return get(x, y).fixed;
}

bool Board::set(int x, int y, Color color) {
    if (x < 0 || x >= size_ || y < 0 || y >= size_) {
        return false;
    }
    if (cells_[index(x, y)].fixed) {
        return false;
    }
    cells_[index(x, y)].color = color;
    return true;
}

void Board::set_fixed(int x, int y, Color color, bool fixed) {
    if (x < 0 || x >= size_ || y < 0 || y >= size_) {
        return;
    }
    cells_[index(x, y)] = Cell{color, fixed};
}

void Board::clear_cell(int x, int y) {
    if (x >= 0 && x < size_ && y >= 0 && y < size_) {
        if (!cells_[index(x, y)].fixed) {
            cells_[index(x, y)].color = Color::None;
        }
    }
}

void Board::clear() {
    for (auto& cell : cells_) {
        if (!cell.fixed) {
            cell.color = Color::None;
        }
    }
}

int Board::count_row_color(int y, Color color) const {
    if (y < 0 || y >= size_) return 0;
    int count = 0;
    for (int x = 0; x < size_; ++x) {
        if (cells_[index(x, y)].color == color) {
            count++;
        }
    }
    return count;
}

int Board::count_col_color(int x, Color color) const {
    if (x < 0 || x >= size_) return 0;
    int count = 0;
    for (int y = 0; y < size_; ++y) {
        if (cells_[index(x, y)].color == color) {
            count++;
        }
    }
    return count;
}

int Board::empty_count() const {
    int count = 0;
    for (const auto& c : cells_) {
        if (c.color == Color::None) {
            count++;
        }
    }
    return count;
}

bool Board::has_three_in_a_row(int x, int y) const {
    Color c = get_color(x, y);
    if (c == Color::None) return false;

    // Horizontal checks
    if (x >= 2 && get_color(x - 1, y) == c && get_color(x - 2, y) == c) return true;
    if (x >= 1 && x + 1 < size_ && get_color(x - 1, y) == c && get_color(x + 1, y) == c) return true;
    if (x + 2 < size_ && get_color(x + 1, y) == c && get_color(x + 2, y) == c) return true;

    // Vertical checks
    if (y >= 2 && get_color(x, y - 1) == c && get_color(x, y - 2) == c) return true;
    if (y >= 1 && y + 1 < size_ && get_color(x, y - 1) == c && get_color(x, y + 1) == c) return true;
    if (y + 2 < size_ && get_color(x, y + 1) == c && get_color(x, y + 2) == c) return true;

    return false;
}

bool Board::is_row_valid(int y) const {
    int half = size_ / 2;
    int blue = 0;
    int red = 0;
    for (int x = 0; x < size_; ++x) {
        Color c = get_color(x, y);
        if (c == Color::Blue) blue++;
        else if (c == Color::Red) red++;

        if (x >= 2) {
            Color c0 = get_color(x - 2, y);
            Color c1 = get_color(x - 1, y);
            Color c2 = get_color(x, y);
            if (c0 != Color::None && c0 == c1 && c1 == c2) {
                return false;
            }
        }
    }
    if (blue > half || red > half) return false;
    return true;
}

bool Board::is_col_valid(int x) const {
    int half = size_ / 2;
    int blue = 0;
    int red = 0;
    for (int y = 0; y < size_; ++y) {
        Color c = get_color(x, y);
        if (c == Color::Blue) blue++;
        else if (c == Color::Red) red++;

        if (y >= 2) {
            Color c0 = get_color(x, y - 2);
            Color c1 = get_color(x, y - 1);
            Color c2 = get_color(x, y);
            if (c0 != Color::None && c0 == c1 && c1 == c2) {
                return false;
            }
        }
    }
    if (blue > half || red > half) return false;
    return true;
}

std::string Board::row_to_string(int y) const {
    std::string s;
    s.reserve(size_);
    for (int x = 0; x < size_; ++x) {
        Color c = get_color(x, y);
        if (c == Color::None) s += '.';
        else if (c == Color::Blue) s += '1';
        else s += '2';
    }
    return s;
}

std::string Board::col_to_string(int x) const {
    std::string s;
    s.reserve(size_);
    for (int y = 0; y < size_; ++y) {
        Color c = get_color(x, y);
        if (c == Color::None) s += '.';
        else if (c == Color::Blue) s += '1';
        else s += '2';
    }
    return s;
}

bool Board::rows_unique() const {
    for (int y1 = 0; y1 < size_; ++y1) {
        std::string s1 = row_to_string(y1);
        if (s1.find('.') != std::string::npos) continue; // Skip incomplete
        for (int y2 = y1 + 1; y2 < size_; ++y2) {
            std::string s2 = row_to_string(y2);
            if (s2.find('.') != std::string::npos) continue;
            if (s1 == s2) return false;
        }
    }
    return true;
}

bool Board::cols_unique() const {
    for (int x1 = 0; x1 < size_; ++x1) {
        std::string s1 = col_to_string(x1);
        if (s1.find('.') != std::string::npos) continue; // Skip incomplete
        for (int x2 = x1 + 1; x2 < size_; ++x2) {
            std::string s2 = col_to_string(x2);
            if (s2.find('.') != std::string::npos) continue;
            if (s1 == s2) return false;
        }
    }
    return true;
}

bool Board::is_valid() const {
    for (int y = 0; y < size_; ++y) {
        if (!is_row_valid(y)) return false;
    }
    for (int x = 0; x < size_; ++x) {
        if (!is_col_valid(x)) return false;
    }
    if (!rows_unique()) return false;
    if (!cols_unique()) return false;
    return true;
}

bool Board::is_complete() const {
    if (empty_count() > 0) return false;
    return is_valid();
}

} // namespace ohpsp::takuzu
