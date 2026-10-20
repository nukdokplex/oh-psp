#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace ohpsp::takuzu {

enum class Color : uint8_t {
    None = 0,
    Blue = 1,
    Red = 2
};

inline Color opposite_color(Color c) {
    if (c == Color::Blue) return Color::Red;
    if (c == Color::Red) return Color::Blue;
    return Color::None;
}

struct Cell {
    Color color = Color::None;
    bool fixed = false;
};

class Board {
public:
    explicit Board(int size = 6);

    int size() const { return size_; }
    void resize(int size);

    Cell get(int x, int y) const;
    Color get_color(int x, int y) const;
    bool is_fixed(int x, int y) const;

    bool set(int x, int y, Color color);
    void set_fixed(int x, int y, Color color, bool fixed = true);
    void clear_cell(int x, int y);
    void clear();

    int count_row_color(int y, Color color) const;
    int count_col_color(int x, Color color) const;
    int empty_count() const;

    bool has_three_in_a_row(int x, int y) const;
    bool is_row_valid(int y) const;
    bool is_col_valid(int x) const;
    bool is_valid() const;
    bool is_complete() const;

    // Check row/column equality against others
    bool rows_unique() const;
    bool cols_unique() const;

    std::string row_to_string(int y) const;
    std::string col_to_string(int x) const;

private:
    int size_;
    std::vector<Cell> cells_;

    int index(int x, int y) const { return y * size_ + x; }
};

} // namespace ohpsp::takuzu
