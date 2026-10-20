#pragma once

#include <cstdint>
#include <vector>

namespace ohpsp::vision {

enum class TileType : uint8_t {
    Unknown = 0,
    Wall = 1,        // Red
    Dot = 2,         // Blue (un-numbered)
    NumberedDot = 3  // Blue with sight number
};

struct Cell {
    TileType type = TileType::Unknown;
    int number = 0;   // Clue value (only if type == NumberedDot)
    bool fixed = false; // Fixed initial clues cannot be toggled
};

enum class Direction : uint8_t {
    Up = 0,
    Down = 1,
    Left = 2,
    Right = 3
};

struct RayInfo {
    int confirmed_dots = 0;
    int unknowns = 0;
    int max_possible = 0;
    int first_unknown_x = -1;
    int first_unknown_y = -1;
};

struct TileSightInfo {
    int total_confirmed = 0;
    int total_unknowns = 0;
    int total_max_possible = 0;
    RayInfo rays[4];
};

class Board {
public:
    explicit Board(int size = 5);

    int size() const { return size_; }
    void resize(int size);

    Cell get(int x, int y) const;
    TileType get_type(int x, int y) const;
    int get_number(int x, int y) const;
    bool is_fixed(int x, int y) const;

    bool set_type(int x, int y, TileType type);
    void set_fixed(int x, int y, TileType type, int number = 0, bool fixed = true);
    void cycle_cell(int x, int y); // Unknown -> Dot -> Wall -> Unknown
    void clear();

    int empty_count() const;
    TileSightInfo compute_sight(int x, int y) const;

    bool is_tile_satisfied(int x, int y) const;
    bool has_violations() const;
    bool is_complete() const;

private:
    int size_;
    std::vector<Cell> cells_;

    int index(int x, int y) const { return y * size_ + x; }
};

} // namespace ohpsp::vision
