#pragma once

#include "Board.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

struct Position {
    int row = -1;
    int col = -1;

    [[nodiscard]] bool isValid() const { return row >= 0 && col >= 0; }

    bool operator==(const Position& other) const { return row == other.row && col == other.col; }
    bool operator!=(const Position& other) const { return !(*this == other); }
};

enum class Difficulty : std::uint8_t {
    Easy = 0,
    Medium,
    Hard
};

enum class BoardStyle : std::uint8_t {
    Wooden = 0,
    BlackWhite,
    IceTheme,
    StoneTheme,
    DiamondTheme,
    TournamentTheme,
    BubblegumTheme,
    CustomTheme
};

enum class PlayerType : std::uint8_t {
    Human = 0,
    AI
};

struct Move {
    Player player = Player::None;
    Position queenFrom;
    Position queenTo;
    Position arrow;
};

struct BoardLayout {
    BoardDimension size;
    std::vector<Position> whiteQueens;
    std::vector<Position> blackQueens;
};

inline const BoardLayout& layoutFor(BoardDimension dimension) {
    static const std::vector<BoardLayout> layouts = {
        // White (player) queens at bottom, Black (AI) queens at top
        {BoardDimension::Six,
         {{5, 1}, {5, 4}, {4, 0}, {4, 5}},
         {{0, 1}, {0, 4}, {1, 0}, {1, 5}}},
        {BoardDimension::Eight,
         {{7, 2}, {7, 5}, {5, 0}, {5, 7}},
         {{0, 2}, {0, 5}, {2, 0}, {2, 7}}},
        {BoardDimension::Ten,
         {{9, 3}, {9, 6}, {6, 0}, {6, 9}},
         {{0, 3}, {0, 6}, {3, 0}, {3, 9}}}
    };

    auto it = std::find_if(layouts.begin(), layouts.end(), [dimension](const BoardLayout& layout) {
        return layout.size == dimension;
    });
    if (it == layouts.end()) {
        throw std::runtime_error("Unsupported board layout requested");
    }
    return *it;
}

class GameState {
public:
    GameState() = default;

    void startNewGame(BoardDimension boardSize, Difficulty difficulty) {
        _board.resize(boardSize);
        _board.clear();
        _boardSize = boardSize;
        _difficulty = difficulty;
        _currentPlayer = Player::White;
        _arrows.clear();
        _moveHistory.clear();
        _isFinished = false;
        _winner = Player::None;
        initializeQueens(layoutFor(boardSize));
    }

    [[nodiscard]] GameState clone() const { return *this; }

    [[nodiscard]] Board& board() { return _board; }
    [[nodiscard]] const Board& board() const { return _board; }

    [[nodiscard]] Player currentPlayer() const { return _currentPlayer; }
    void setCurrentPlayer(Player player) { _currentPlayer = player; }

    [[nodiscard]] Difficulty difficulty() const { return _difficulty; }
    void setDifficulty(Difficulty difficulty) { _difficulty = difficulty; }

    [[nodiscard]] BoardDimension boardSize() const { return _boardSize; }

    [[nodiscard]] const std::vector<Position>& queenPositions(Player player) const {
        return _queens[playerIndex(player)];
    }

    [[nodiscard]] std::vector<Position>& queenPositions(Player player) {
        return _queens[playerIndex(player)];
    }

    [[nodiscard]] const std::vector<Position>& arrowPositions() const { return _arrows; }

    void addArrow(const Position& pos) {
        if (!_board.isInsideBoard(pos.row, pos.col)) {
            throw std::out_of_range("Arrow position outside of board");
        }
        _arrows.push_back(pos);
        _board.setTile(pos.row, pos.col, TileContent::Arrow);
    }

    void updateQueenPosition(Player player, const Position& from, const Position& to) {
        auto& positions = _queens[playerIndex(player)];
        auto it = std::find(positions.begin(), positions.end(), from);
        if (it == positions.end()) {
            throw std::runtime_error("Trying to move a queen that does not exist");
        }

        _board.setTile(from.row, from.col, TileContent::Empty);
        _board.setTile(to.row, to.col, player == Player::White ? TileContent::WhiteQueen : TileContent::BlackQueen);
        *it = to;
    }

    void recordMove(const Move& move) { _moveHistory.push_back(move); }
    [[nodiscard]] const std::vector<Move>& moveHistory() const { return _moveHistory; }

    void markFinished(Player winner) {
        _isFinished = true;
        _winner = winner;
    }

    void clearFinishedState() {
        _isFinished = false;
        _winner = Player::None;
    }

    [[nodiscard]] bool isFinished() const { return _isFinished; }
    [[nodiscard]] Player winner() const { return _winner; }

private:
    Board _board{};
    Player _currentPlayer = Player::White;
    Difficulty _difficulty = Difficulty::Medium;
    BoardDimension _boardSize = BoardDimension::Ten;
    std::array<std::vector<Position>, 2> _queens{}; // 0 -> White, 1 -> Black
    std::vector<Position> _arrows;
    std::vector<Move> _moveHistory;
    bool _isFinished = false;
    Player _winner = Player::None;

    void initializeQueens(const BoardLayout& layout) {
        for (auto& vec : _queens) {
            vec.clear();
        }

        _queens[0] = layout.whiteQueens;
        _queens[1] = layout.blackQueens;

        for (const auto& pos : layout.whiteQueens) {
            _board.setTile(pos.row, pos.col, TileContent::WhiteQueen);
        }
        for (const auto& pos : layout.blackQueens) {
            _board.setTile(pos.row, pos.col, TileContent::BlackQueen);
        }
    }

    [[nodiscard]] static std::size_t playerIndex(Player player) {
        switch (player) {
            case Player::White:
                return 0;
            case Player::Black:
                return 1;
            default:
                throw std::runtime_error("Player None does not have queen positions");
        }
    }
};
