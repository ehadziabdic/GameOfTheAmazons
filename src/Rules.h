// Implement rules here - Muhammed

#ifndef RULES_H
#define RULES_H

#include <vector>
#include <array>
#include <optional>
#include <algorithm>
#include <cassert>
#include <cstdint>

namespace amazons {

    
    // Basic enums and small types

    enum class Tile : uint8_t {
        Empty = 0,
        WhiteQueen = 1,
        BlackQueen = 2,
        Arrow = 3
    };

    enum class Player : uint8_t {
        White = 0,
        Black = 1
    };

    inline Player opponent(Player p) {
        return (p == Player::White) ? Player::Black : Player::White;
    }

    struct Pos {
        int r;
        int c;
        Pos() : r(-1), c(-1) {}
        Pos(int rr, int cc) : r(rr), c(cc) {}
        bool operator==(const Pos& o) const { return r == o.r && c == o.c; }
        bool operator!=(const Pos& o) const { return !(*this == o); }
    };

    struct Move {
        Pos from;
        Pos to;
        Pos arrow;
        Move() = default;
        Move(Pos f, Pos t, Pos a) : from(f), to(t), arrow(a) {}
    };

    class Board {
    public:
        int rows;
        int cols;
        std::vector<std::vector<Tile>> grid; 

        Board() : rows(0), cols(0) {}
        Board(int r, int c) : rows(r), cols(c), grid(r, std::vector<Tile>(c, Tile::Empty)) {}

        inline bool isInside(const Pos& p) const {
            return p.r >= 0 && p.r < rows && p.c >= 0 && p.c < cols;
        }

        inline Tile get(const Pos& p) const {
            assert(isInside(p));
            return grid[p.r][p.c];
        }

        inline void set(const Pos& p, Tile t) {
            assert(isInside(p));
            grid[p.r][p.c] = t;
        }

        inline Board clone() const {
            Board b(rows, cols);
            b.grid = grid;
            return b;
        }
    };

    struct GameState {
        Board board;
        Player currentPlayer;
        std::vector<Pos> whiteQueens;
        std::vector<Pos> blackQueens;
        std::vector<Move> moveHistory;
        bool finished;                    
        std::optional<Player> winner;     // winner when finished

        GameState() : board(), currentPlayer(Player::White), finished(false), winner(std::nullopt) {}
        GameState(int rows, int cols) : board(rows, cols), currentPlayer(Player::White), finished(false), winner(std::nullopt) {}

        // convenience access to queen lists
        inline std::vector<Pos>& queensOf(Player p) {
            return (p == Player::White) ? whiteQueens : blackQueens;
        }
        inline const std::vector<Pos>& queensOf(Player p) const {
            return (p == Player::White) ? whiteQueens : blackQueens;
        }

        // Rebuild queen lists from the board content (useful if external code modifies board)
        inline void rebuildQueenLists() {
            whiteQueens.clear();
            blackQueens.clear();
            for (int r = 0; r < board.rows; ++r) {
                for (int c = 0; c < board.cols; ++c) {
                    Tile t = board.grid[r][c];
                    if (t == Tile::WhiteQueen) whiteQueens.emplace_back(r, c);
                    else if (t == Tile::BlackQueen) blackQueens.emplace_back(r, c);
                }
            }
        }
    };

    
    // Ray-casting and directions

    // Ray-cast: from a starting square (exclusive) step along direction (dr,dc)
    // collecting reachable empty tiles until a blocker (queen or arrow) or edge is hit.
    inline std::vector<Pos> rayCast(const Board& board, const Pos& start, int dr, int dc) {
        std::vector<Pos> out;
        Pos cur(start.r + dr, start.c + dc);
        while (board.isInside(cur)) {
            Tile t = board.get(cur);
            if (t != Tile::Empty) break;
            out.push_back(cur);
            cur.r += dr;
            cur.c += dc;
        }
        return out;
    }

    // Eight queen-like directions
    static const std::array<std::pair<int, int>, 8> DIRECTIONS = { {
        { -1,  0 }, 
        { -1,  1 }, 
        {  0,  1 }, 
        {  1,  1 }, 
        {  1,  0 }, 
        {  1, -1 }, 
        {  0, -1 }, 
        { -1, -1 }  
    } };

    
    // Generate legal queen destinations from a queen position using rayCast in 8 directions.
    // Returns all reachable (empty) positions the queen may move to.
    
    inline std::vector<Pos> generateQueenDestinations(const Board& board, const Pos& queenPos) {
        std::vector<Pos> results;
        for (const auto& d : DIRECTIONS) {
            auto part = rayCast(board, queenPos, d.first, d.second);
            results.insert(results.end(), part.begin(), part.end());
        }
        return results;
    }

    
    // For a candidate queen move (from -> to), simulate the queen at `to`
    // (treat `from` as empty and `to` as occupied) and run rayCast from `to`
    // to find legal arrow targets. Return all reachable arrow target positions.
    
    inline std::vector<Pos> generateArrowTargetsAfterMove(const Board& board, const Pos& from, const Pos& to) {
        Board tmp = board.clone();
        // set from empty
        tmp.set(from, Tile::Empty);
        // preserve queen color from original 'from' tile
        Tile fromTile = board.get(from);
        Tile newTile = (fromTile == Tile::WhiteQueen) ? Tile::WhiteQueen : Tile::BlackQueen;
        tmp.set(to, newTile);

        std::vector<Pos> results;
        for (const auto& d : DIRECTIONS) {
            auto part = rayCast(tmp, to, d.first, d.second);
            results.insert(results.end(), part.begin(), part.end());
        }
        return results;
    }

    
    // Generate all legal (queen + arrow) moves for a player in the current state.
    // For each queen: generate queen destinations; for each destination, generate arrow targets.
    
    inline std::vector<Move> generateAllMoves(const GameState& state, Player p) {
        std::vector<Move> out;
        const Board& board = state.board;
        const auto& queens = state.queensOf(p);

        for (const Pos& qpos : queens) {
            auto qdests = generateQueenDestinations(board, qpos);
            for (const Pos& dest : qdests) {
                auto arrows = generateArrowTargetsAfterMove(board, qpos, dest);
                for (const Pos& a : arrows) {
                    out.emplace_back(qpos, dest, a);
                }
            }
        }
        return out;
    }

    
    // Validate a move for the given player within the given state.
    // Checks:
    //  - positions inside board
    //  - selected from square contains player's queen
    //  - destination is empty and reachable via rayCast
    //  - arrow is empty and reachable after the queen moves
    
    inline bool validateMove(const GameState& state, const Move& m, Player player) {
        if (!state.board.isInside(m.from) || !state.board.isInside(m.to) || !state.board.isInside(m.arrow)) return false;

        Tile fromT = state.board.get(m.from);
        Tile toT = state.board.get(m.to);
        Tile arrowT = state.board.get(m.arrow);

        Tile expected = (player == Player::White) ? Tile::WhiteQueen : Tile::BlackQueen;
        if (fromT != expected) return false;
        if (toT != Tile::Empty) return false;
        if (arrowT != Tile::Empty) return false;

        // check 'to' reachable by rayCast
        bool toReachable = false;
        for (const auto& d : DIRECTIONS) {
            auto part = rayCast(state.board, m.from, d.first, d.second);
            if (std::find(part.begin(), part.end(), m.to) != part.end()) { toReachable = true; break; }
        }
        if (!toReachable) return false;

        // check arrow reachable after simulating move
        auto arrowTargets = generateArrowTargetsAfterMove(state.board, m.from, m.to);
        if (std::find(arrowTargets.begin(), arrowTargets.end(), m.arrow) == arrowTargets.end()) return false;

        return true;
    }

    
    // Apply a move to the GameState. Caller should ensure move is valid first.
    // Effects:
    //  - move the queen tile
    //  - place an arrow tile
    //  - update queen lists (tries replacement, falls back to rebuild)
    //  - push move into history
    //  - switch current player
    //  - update finished/winner by checking next player's moves
    
    inline void applyMove(GameState& state, const Move& m) {
        assert(state.board.isInside(m.from) && state.board.isInside(m.to) && state.board.isInside(m.arrow));
        Tile fromT = state.board.get(m.from);
        Tile toT = state.board.get(m.to);
        Tile arrowT = state.board.get(m.arrow);

        // sanity checks
        assert(fromT == Tile::WhiteQueen || fromT == Tile::BlackQueen);
        assert(toT == Tile::Empty);
        assert(arrowT == Tile::Empty);

        // move queen on board
        state.board.set(m.from, Tile::Empty);
        state.board.set(m.to, fromT);

        // update queen list for current player
        auto& qlist = state.queensOf(state.currentPlayer);
        bool replaced = false;
        for (auto& p : qlist) {
            if (p == m.from) { p = m.to; replaced = true; break; }
        }
        if (!replaced) {
            // safety: rebuild if replacement failed
            state.rebuildQueenLists();
        }

        // place arrow
        state.board.set(m.arrow, Tile::Arrow);

        // record and switch player
        state.moveHistory.push_back(m);
        state.currentPlayer = opponent(state.currentPlayer);

        // detect terminal: if next player has no legal moves -> previous player wins
        auto nextMoves = generateAllMoves(state, state.currentPlayer);
        if (nextMoves.empty()) {
            state.finished = true;
            state.winner = opponent(state.currentPlayer); // the player who just moved
        }
        else {
            state.finished = false;
            state.winner.reset();
        }
    }

    
    // Quick check whether a player has any legal moves (early exit when found).
    
    inline bool hasLegalMoves(const GameState& state, Player p) {
        const Board& board = state.board;
        const auto& queens = state.queensOf(p);

        for (const Pos& qpos : queens) {
            for (const auto& d : DIRECTIONS) {
                auto qdests = rayCast(board, qpos, d.first, d.second);
                if (qdests.empty()) continue;
                for (const Pos& dest : qdests) {
                    auto arrows = generateArrowTargetsAfterMove(board, qpos, dest);
                    if (!arrows.empty()) return true;
                }
            }
        }
        return false;
    }

    
    // checkWin: examine the current state; if game is terminal, set finished/winner and return it.
    // Returns std::nullopt if game not finished, otherwise returns the winning Player.
    
    inline std::optional<Player> checkWin(GameState& state) {
        if (state.finished) return state.winner;
        Player p = state.currentPlayer;
        if (!hasLegalMoves(state, p)) {
            state.finished = true;
            state.winner = opponent(p);
            return state.winner;
        }
        return std::nullopt;
    }

}

#endif