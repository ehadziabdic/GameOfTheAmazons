#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class Player : std::uint8_t {
    None = 0,
    White,
    Black
};

enum class TileContent : std::uint8_t {
    Empty = 0,
    WhiteQueen,
    BlackQueen,
    Arrow
};

enum class BoardDimension : std::uint8_t {
    Six = 6,
    Eight = 8,
    Ten = 10
};

struct BoardSizeConfig {
    BoardDimension id;
    const char* name;
    int dimension;
};

inline constexpr std::array<BoardSizeConfig, 3> kBoardSizeConfigs = {
    BoardSizeConfig{BoardDimension::Six, "6x6", 6},
    BoardSizeConfig{BoardDimension::Eight, "8x8", 8},
    BoardSizeConfig{BoardDimension::Ten, "10x10", 10}
};

constexpr Player opponentOf(Player player) {
    return player == Player::White ? Player::Black : (player == Player::Black ? Player::White : Player::None);
}

constexpr bool isQueen(TileContent tile) {
    return tile == TileContent::WhiteQueen || tile == TileContent::BlackQueen;
}

class Board {
public:
    explicit Board(BoardDimension dimension = BoardDimension::Ten) {
        resize(dimension);
    }

    void resize(BoardDimension dimension) {
        _dimension = static_cast<int>(dimension);
        _tiles.assign(static_cast<std::size_t>(_dimension * _dimension), TileContent::Empty);
    }

    [[nodiscard]] int dimension() const { return _dimension; }

    void clear(TileContent fill = TileContent::Empty) {
        std::fill(_tiles.begin(), _tiles.end(), fill);
    }

    [[nodiscard]] bool isInsideBoard(int row, int col) const {
        return row >= 0 && col >= 0 && row < _dimension && col < _dimension;
    }

    [[nodiscard]] TileContent getTile(int row, int col) const {
        assert(isInsideBoard(row, col));
        return _tiles[index(row, col)];
    }

    void setTile(int row, int col, TileContent value) {
        assert(isInsideBoard(row, col));
        _tiles[index(row, col)] = value;
    }

    [[nodiscard]] const std::vector<TileContent>& tiles() const { return _tiles; }
    [[nodiscard]] std::vector<TileContent>& tiles() { return _tiles; }

private:
    int _dimension = 0;
    std::vector<TileContent> _tiles;

    [[nodiscard]] std::size_t index(int row, int col) const {
        assert(isInsideBoard(row, col));
        return static_cast<std::size_t>(row * _dimension + col);
    }
};
