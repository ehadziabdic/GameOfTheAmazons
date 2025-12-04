#pragma once

#include "GameState.h"
#include "Rules.h"
#include <vector>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <cmath>
#include <atomic>
#include <stdexcept>

class SearchCanceled : public std::exception {
public:
    const char* what() const noexcept override {
        return "AI search was cancelled by user";
    }
};

class Algorithm {
public:
    Algorithm() = default;
    ~Algorithm() = default;

    [[nodiscard]] Move getBestMove(const GameState& state, Difficulty difficulty,
        std::atomic_bool* cancelToken = nullptr);

private:
    [[nodiscard]] double calculateMobility(const GameState& state, Player player,
        std::atomic_bool* cancelToken = nullptr);

    [[nodiscard]] double calculateTerritory(const GameState& state, Player player,
        std::atomic_bool* cancelToken = nullptr);

    [[nodiscard]] int floodFillFromQueen(const GameState& state, const Position& queenPos);

    [[nodiscard]] double calculateSpatialInfluence(const GameState& state, Player player,
        std::atomic_bool* cancelToken = nullptr);

    [[nodiscard]] double evaluate(const GameState& state, Player player,
        std::atomic_bool* cancelToken = nullptr);

    [[nodiscard]] double minimax(const GameState& state, int depth, int maxDepth,
        double alpha, double beta, bool maximizingPlayer,
        Player aiPlayer, std::atomic_bool* cancelToken = nullptr);

    [[nodiscard]] bool isTerminal(const GameState& state) const;

    [[nodiscard]] static int getSearchDepth(Difficulty difficulty);

    inline void checkCancellation(std::atomic_bool* cancelToken) const {
        if (cancelToken && cancelToken->load()) {
            throw SearchCanceled();
        }
    }

    static constexpr double MOBILITY_WEIGHT = 1.0;
    static constexpr double TERRITORY_WEIGHT = 2.0;
    static constexpr double SPATIAL_WEIGHT = 0.5;

    static constexpr double CENTER_BONUS = 5.0;
    static constexpr double REACHABLE_BONUS_FACTOR = 0.1;
    static constexpr double EDGE_TRAPPED_PENALTY = 3.0;

    static constexpr double WIN_SCORE = 100000.0;
    static constexpr double LOSE_SCORE = -100000.0;

    static constexpr int DIR_ROW[8] = { -1, -1, -1,  0, 0, 1, 1, 1 };
    static constexpr int DIR_COL[8] = { -1,  0,  1, -1, 1,-1, 0, 1 };
};

inline Move Algorithm::getBestMove(const GameState& state, Difficulty difficulty,
    std::atomic_bool* cancelToken) {
    int maxDepth = getSearchDepth(difficulty);
    Player aiPlayer = state.currentPlayer();

    std::vector<Move> legalMoves = amazons::generateMovesForPlayer(state, aiPlayer);
    if (legalMoves.empty()) {
        throw std::runtime_error("getBestMove called but no legal moves available");
    }

    Move bestMove = legalMoves[0];
    double bestScore = LOSE_SCORE;

    for (const auto& move : legalMoves) {
        checkCancellation(cancelToken);

        GameState nextState = state.clone();
        amazons::applyMove(nextState, move);

        double score = minimax(nextState, 1, maxDepth,
            LOSE_SCORE, WIN_SCORE,
            false, aiPlayer, cancelToken);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    return bestMove;
}

inline double Algorithm::calculateMobility(const GameState& state, Player player,
    std::atomic_bool* cancelToken) {
    checkCancellation(cancelToken);

    GameState temp = state.clone();

    temp.setCurrentPlayer(player);
    int playerMoves = static_cast<int>(amazons::generateMovesForPlayer(temp, player).size());

    temp.setCurrentPlayer(opponentOf(player));
    int opponentMoves = static_cast<int>(amazons::generateMovesForPlayer(temp, opponentOf(player)).size());

    return static_cast<double>(playerMoves - opponentMoves);
}

inline int Algorithm::floodFillFromQueen(const GameState& state, const Position& queenPos) {
    const Board& board = state.board();
    const int dim = board.dimension();

    std::unordered_set<int> visited;
    std::queue<Position> q;

    visited.insert(queenPos.row * dim + queenPos.col);

    for (int d = 0; d < 8; ++d) {
        int r = queenPos.row + DIR_ROW[d];
        int c = queenPos.col + DIR_COL[d];

        while (board.isInsideBoard(r, c)) {
            TileContent tile = board.getTile(r, c);

            if (tile == TileContent::Arrow ||
                tile == TileContent::WhiteQueen ||
                tile == TileContent::BlackQueen) {
                break;
            }

            int idx = r * dim + c;
            if (visited.find(idx) == visited.end()) {
                visited.insert(idx);
                q.push({ r, c });
            }

            r += DIR_ROW[d];
            c += DIR_COL[d];
        }
    }

    int reachableCount = 0;

    while (!q.empty()) {
        Position cur = q.front();
        q.pop();

        ++reachableCount;

        for (int d = 0; d < 8; ++d) {
            int r = cur.row + DIR_ROW[d];
            int c = cur.col + DIR_COL[d];

            while (board.isInsideBoard(r, c)) {
                TileContent tile = board.getTile(r, c);

                if (tile == TileContent::Arrow ||
                    tile == TileContent::WhiteQueen ||
                    tile == TileContent::BlackQueen) {
                    break;
                }

                int idx = r * dim + c;
                if (visited.find(idx) == visited.end()) {
                    visited.insert(idx);
                    q.push({ r, c });
                }

                r += DIR_ROW[d];
                c += DIR_COL[d];
            }
        }
    }

    return reachableCount;
}

inline double Algorithm::calculateTerritory(const GameState& state, Player player,
    std::atomic_bool* cancelToken) {
    checkCancellation(cancelToken);

    int playerTerritory = 0;
    int opponentTerritory = 0;

    for (const auto& pos : state.queenPositions(player)) {
        playerTerritory += floodFillFromQueen(state, pos);
    }
    for (const auto& pos : state.queenPositions(opponentOf(player))) {
        opponentTerritory += floodFillFromQueen(state, pos);
    }

    return static_cast<double>(playerTerritory - opponentTerritory);
}

inline double Algorithm::calculateSpatialInfluence(const GameState& state, Player player,
    std::atomic_bool* cancelToken) {
    checkCancellation(cancelToken);

    const Board& board = state.board();
    const int dim = board.dimension();

    double centerRow = (dim - 1) / 2.0;
    double centerCol = (dim - 1) / 2.0;

    auto evalSide = [&](Player p) -> double {
        double sum = 0.0;
        const auto& queens = state.queenPositions(p);

        for (const auto& pos : queens) {
            double dist = std::abs(pos.row - centerRow) +
                std::abs(pos.col - centerCol);
            if (dist <= 2.0) {
                sum += CENTER_BONUS;
            }

            int reachable = floodFillFromQueen(state, pos);
            sum += reachable * REACHABLE_BONUS_FACTOR;

            bool onEdge = (pos.row == 0 || pos.row == dim - 1 ||
                pos.col == 0 || pos.col == dim - 1);
            if (onEdge && reachable < 5) {
                sum -= EDGE_TRAPPED_PENALTY;
            }
        }

        return sum;
        };

    double playerInf = evalSide(player);
    double opponentInf = evalSide(opponentOf(player));

    return playerInf - opponentInf;
}

inline double Algorithm::evaluate(const GameState& state, Player player,
    std::atomic_bool* cancelToken) {
    checkCancellation(cancelToken);

    if (!amazons::hasAnyLegalMove(state, player)) {
        return LOSE_SCORE;
    }
    if (!amazons::hasAnyLegalMove(state, opponentOf(player))) {
        return WIN_SCORE;
    }

    double mobility = calculateMobility(state, player, cancelToken);
    double territory = calculateTerritory(state, player, cancelToken);
    double spatial = calculateSpatialInfluence(state, player, cancelToken);

    return MOBILITY_WEIGHT * mobility +
        TERRITORY_WEIGHT * territory +
        SPATIAL_WEIGHT * spatial;
}

inline bool Algorithm::isTerminal(const GameState& state) const {
    return !amazons::hasAnyLegalMove(state, state.currentPlayer());
}

inline double Algorithm::minimax(const GameState& state, int depth, int maxDepth,
    double alpha, double beta, bool maximizingPlayer,
    Player aiPlayer, std::atomic_bool* cancelToken) {
    checkCancellation(cancelToken);

    if (depth >= maxDepth || isTerminal(state)) {
        return evaluate(state, aiPlayer, cancelToken);
    }

    std::vector<Move> legalMoves = amazons::generateMovesForPlayer(state, state.currentPlayer());

    if (maximizingPlayer) {
        double best = LOSE_SCORE;

        for (const auto& move : legalMoves) {
            checkCancellation(cancelToken);

            GameState nextState = state.clone();
            amazons::applyMove(nextState, move);

            double val = minimax(nextState, depth + 1, maxDepth,
                alpha, beta, false, aiPlayer, cancelToken);

            best = std::max(best, val);
            alpha = std::max(alpha, val);

            if (alpha >= beta) {
                break;
            }
        }

        return best;
    }
    else {
        double best = WIN_SCORE;

        for (const auto& move : legalMoves) {
            checkCancellation(cancelToken);

            GameState nextState = state.clone();
            amazons::applyMove(nextState, move);

            double val = minimax(nextState, depth + 1, maxDepth,
                alpha, beta, true, aiPlayer, cancelToken);

            best = std::min(best, val);
            beta = std::min(beta, val);

            if (alpha >= beta) {
                break;
            }
        }

        return best;
    }
}

inline int Algorithm::getSearchDepth(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:   return 2;
    case Difficulty::Medium: return 4;
    case Difficulty::Hard:   return 6;
    default:                 return 4;
    }
}
