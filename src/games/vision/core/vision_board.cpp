#include "vision_board.hpp"

namespace ohpsp::vision {

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

TileType Board::get_type(int x, int y) const {
    return get(x, y).type;
}

int Board::get_number(int x, int y) const {
    return get(x, y).number;
}

bool Board::is_fixed(int x, int y) const {
    return get(x, y).fixed;
}

bool Board::set_type(int x, int y, TileType type) {
    if (x < 0 || x >= size_ || y < 0 || y >= size_) return false;
    if (cells_[index(x, y)].fixed) return false;
    cells_[index(x, y)].type = type;
    return true;
}

void Board::set_fixed(int x, int y, TileType type, int number, bool fixed) {
    if (x < 0 || x >= size_ || y < 0 || y >= size_) return;
    cells_[index(x, y)] = Cell{type, number, fixed};
}

void Board::cycle_cell(int x, int y) {
    if (x < 0 || x >= size_ || y < 0 || y >= size_) return;
    if (cells_[index(x, y)].fixed) return;

    TileType cur = cells_[index(x, y)].type;
    TileType next = TileType::Unknown;
    if (cur == TileType::Unknown) next = TileType::Dot;
    else if (cur == TileType::Dot) next = TileType::Wall;
    else if (cur == TileType::Wall) next = TileType::Unknown;

    cells_[index(x, y)].type = next;
}

void Board::clear() {
    for (auto& cell : cells_) {
        if (!cell.fixed) {
            cell.type = TileType::Unknown;
        }
    }
}

int Board::empty_count() const {
    int count = 0;
    for (const auto& c : cells_) {
        if (c.type == TileType::Unknown) {
            count++;
        }
    }
    return count;
}

TileSightInfo Board::compute_sight(int x, int y) const {
    TileSightInfo info{};
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};

    for (int d = 0; d < 4; ++d) {
        RayInfo& ray = info.rays[d];
        ray.first_unknown_x = -1;
        ray.first_unknown_y = -1;
        bool hit_unknown = false;

        int cx = x + dx[d];
        int cy = y + dy[d];

        while (cx >= 0 && cx < size_ && cy >= 0 && cy < size_) {
            TileType t = get_type(cx, cy);
            if (t == TileType::Wall) {
                break; // Wall blocks sight
            }
            if (t == TileType::Unknown) {
                if (!hit_unknown) {
                    hit_unknown = true;
                    ray.first_unknown_x = cx;
                    ray.first_unknown_y = cy;
                }
                ray.unknowns++;
                ray.max_possible++;
            } else if (t == TileType::Dot || t == TileType::NumberedDot) {
                if (!hit_unknown) {
                    ray.confirmed_dots++;
                }
                ray.max_possible++;
            }
            cx += dx[d];
            cy += dy[d];
        }

        info.total_confirmed += ray.confirmed_dots;
        info.total_unknowns += ray.unknowns;
        info.total_max_possible += ray.max_possible;
    }

    return info;
}

bool Board::is_tile_satisfied(int x, int y) const {
    Cell cell = get(x, y);
    if (cell.type == TileType::NumberedDot) {
        TileSightInfo info = compute_sight(x, y);
        // Only satisfied if exact count reached and no unknowns in open rays
        if (info.total_confirmed != cell.number) return false;
        // Check that rays with confirmed count cannot accidentally expand
        for (int d = 0; d < 4; ++d) {
            if (info.rays[d].unknowns > 0) return false;
        }
        return true;
    }
    if (cell.type == TileType::Dot) {
        TileSightInfo info = compute_sight(x, y);
        return info.total_confirmed >= 1;
    }
    return false;
}

bool Board::has_violations() const {
    for (int y = 0; y < size_; ++y) {
        for (int x = 0; x < size_; ++x) {
            Cell cell = get(x, y);
            if (cell.type == TileType::NumberedDot) {
                TileSightInfo info = compute_sight(x, y);
                if (info.total_confirmed > cell.number) return true;
                if (info.total_max_possible < cell.number) return true;
            } else if (cell.type == TileType::Dot) {
                TileSightInfo info = compute_sight(x, y);
                if (info.total_max_possible == 0) return true; // Isolated dot
            }
        }
    }
    return false;
}

bool Board::is_complete() const {
    if (empty_count() > 0) return false;
    for (int y = 0; y < size_; ++y) {
        for (int x = 0; x < size_; ++x) {
            Cell cell = get(x, y);
            if (cell.type == TileType::NumberedDot) {
                TileSightInfo info = compute_sight(x, y);
                if (info.total_confirmed != cell.number) return false;
            } else if (cell.type == TileType::Dot) {
                TileSightInfo info = compute_sight(x, y);
                if (info.total_confirmed < 1) return false;
            }
        }
    }
    return true;
}

} // namespace ohpsp::vision
