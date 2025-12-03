// Implement rules here - Muhammed

#ifndef RULES_H
#define RULES_H

#include <vector>
#include <array>
#include <optional>
#include <algorithm>
#include <cassert>
#include "GameState.h"

namespace amazons {

    // Return the tile type (WhiteQueen or BlackQueen) belonging to a player

    inline TileContent tileForPlayer(Player player) {
        return (player == Player::White) ? TileContent::WhiteQueen : TileContent::BlackQueen;
    }

    // Utility: checks if Position p exists inside PositionList list

    inline bool containsPosition(const PositionList& list, const Position& p) {
        return std::find(list.begin(), list.end(), p) != list.end();
    }

    // Gathers all tiles reachable from a starting position by sliding in the 8 directions
    // Stops when reaching a non-empty tile or board edge

    inline PositionList gatherReachableTiles(const Board& board, const Position& start) {
        PositionList results;
        // Iterate over all 8 queen directions

        for (const auto& d : DIRECTIONS) {
            PositionList partial;
            Position cur(start.row + d.first, start.col + d.second);
            // Move until blocked or off-board

            while (board.isInsideBoard(cur)) {
                if (board.getTile(cur) != TileContent::Empty) break;
                partial.push_back(cur);
                cur.row += d.first;
                cur.col += d.second;
            }
            // Append reachable tiles from this direction
            results.insert(results.end(), partial.begin(), partial.end());
        }
        return results;
    }

    inline bool hasAnyLegalMove(const GameState& state, Player p);
    inline bool evaluateWinState(GameState& state);

    // Performs ray-casting from a start tile in direction (dr, dc)
    // Collects all consecutive empty tiles

    inline PositionList rayCast(const Board& board, const Position& start, int dr, int dc) {
        PositionList out;
        Position cur(start.row + dr, start.col + dc);
        while (board.isInsideBoard(cur)) {
            if (board.getTile(cur) != TileContent::Empty) break;
            out.push_back(cur);
            cur.row += dr;
            cur.col += dc;
        }
        return out;
    }

    // After moving a queen, generate all valid arrow tiles
    // This is done by cloning the board, applying the queen move,
    // then raycasting outward from the queen's new position

    inline PositionList generateArrowTargetsAfterMove(const Board& board, const Position& queenFrom, const Position& queenTo) {
        Board tmp = board; // copy like Code 2 does (instead of board.clone())
        tmp.setTile(queenFrom, TileContent::Empty);
        TileContent queenTile = board.getTile(queenFrom);
        tmp.setTile(queenTo, queenTile);

        PositionList results;
        for (const auto& d : DIRECTIONS) {
            auto part = rayCast(tmp, queenTo, d.first, d.second);
            results.insert(results.end(), part.begin(), part.end());
        }
        return results;
    }

    // Generates all possible moves for a player
    // moveCap allows early cutoff (used for checking if any move exists)

    inline MoveList generateMovesForPlayer(
        const GameState& state,
        Player p,
        std::optional<size_t> moveCap = std::nullopt
    ) {
        MoveList out;
        const Board& board = state.board();
        const auto& queens = state.queenPositions(p);

        for (const Position& qpos : queens) {
            auto qdests = gatherReachableTiles(board, qpos);
            for (const Position& dest : qdests) {

                auto arrows = generateArrowTargetsAfterMove(board, qpos, dest);
                for (const Position& a : arrows) {
                    out.emplace_back(qpos, dest, a, p);
                    if (moveCap && out.size() >= *moveCap) return out;
                }
            }
        }
        return out;
    }

    // Checks whether a given move is legal under the Game of Amazons rules

    inline bool isMoveLegal(const GameState& state, const Move& move) {
        // Basic rule: cannot move if the game is over or wrong player tries to move
        if (state.isFinished()) return false;
        if (move.player != state.currentPlayer()) return false;

        const Board& board = state.board();
        // All positions must be on-board
        if (!board.isInsideBoard(move.queenFrom) ||
            !board.isInsideBoard(move.queenTo) ||
            !board.isInsideBoard(move.arrow)) return false;

        // The starting tile must contain the player’s queen
        if (board.getTile(move.queenFrom) != tileForPlayer(move.player)) return false;
        // The queenDest and arrow tiles must be empty
        if (board.getTile(move.queenTo) != TileContent::Empty) return false;
        if (board.getTile(move.arrow) != TileContent::Empty) return false;

        // Check queen movement is valid: the destination must be reachable via straight-line sliding
        bool reachableQueen = false;
        for (const auto& d : DIRECTIONS) {
            auto part = rayCast(board, move.queenFrom, d.first, d.second);
            if (containsPosition(part, move.queenTo)) { reachableQueen = true; break; }
        }
        if (!reachableQueen) return false;

        // Check arrow legality: arrow must be reachable after moving queen
        auto arrowTargets = generateArrowTargetsAfterMove(board, move.queenFrom, move.queenTo);
        if (!containsPosition(arrowTargets, move.arrow)) return false;

        return true;
    }

    // Returns true if the player has at least ONE legal move
    inline bool hasAnyLegalMove(const GameState& state, Player p) {
        // Generate only 1 move via moveCap to avoid heavy computation
        return !generateMovesForPlayer(state, p, 1).empty();
    }

    // Evaluates whether current player has no moves
    // If so, game ends and the opponent wins
    inline bool evaluateWinState(GameState& state) {
        if (!hasAnyLegalMove(state, state.currentPlayer())) {
            state.markFinished(opponentOf(state.currentPlayer()));
            return true;
        }
        // Otherwise the game continues
        state.setFinished(false);
        state.setWinner(std::nullopt);
        return false;
    }

    // Applies a move to the game state
    inline void applyMove(GameState& state, const Move& m) {
        assert(isMoveLegal(state, m));

        // Move queen
        state.updateQueenPosition(m.player, m.queenFrom, m.queenTo);
        // Place arrow on board
        state.addArrow(m.arrow);
        // Record move in history
        state.recordMove(m);
        // Next player's turn
        state.setCurrentPlayer(opponentOf(state.currentPlayer()));
        // Evaluate if someone wins after move
        evaluateWinState(state);
    }

}

#endif