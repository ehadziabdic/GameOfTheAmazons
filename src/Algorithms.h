#pragma once

#include "GameState.h"
#include "Rules.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>
#include <atomic>
#include <stdexcept>

struct SearchCanceled : public std::exception {
    const char* what() const noexcept override {
        return "search canceled";
    }
};

namespace detail {

    inline int mobilityCount(const GameState& state, Player player) {
        constexpr std::size_t kMobilitySample = 48;
        auto moves = generateMovesForPlayer(state, player, kMobilitySample);
        return static_cast<int>(moves.size());
    }

    inline std::size_t floodFillReachableTiles(const GameState& state, const Position& start) {
        const auto& board = state.board();
        int dim = board.dimension();

        if (!board.isInsideBoard(start.row, start.col)) {
            return 0;
        }

        Board workingBoard = board;
        workingBoard.setTile(start.row, start.col, TileContent::Empty);

        std::vector<bool> visited(static_cast<std::size_t>(dim * dim), false);
        auto indexOf = [dim](int row, int col) {
            return static_cast<std::size_t>(row * dim + col);
            };

        std::queue<Position> frontier;
        frontier.push(start);
        visited[indexOf(start.row, start.col)] = true;
        std::size_t count = 1;

        while (!frontier.empty()) {
            Position current = frontier.front();
            frontier.pop();

            auto neighbors = gatherReachableTiles(workingBoard, current);
            for (const auto& neighbor : neighbors) {
                auto idx = indexOf(neighbor.row, neighbor.col);
                if (visited[idx]) {
                    continue;
                }
                visited[idx] = true;
                frontier.push(neighbor);
                ++count;
            }
        }

        return count;
    }

    inline int territoryScore(const GameState& state, Player player) {
        Player opponent = opponentOf(player);
        std::size_t playerReachable = 0;
        std::size_t opponentReachable = 0;

        for (const auto& pos : state.queenPositions(player)) {
            playerReachable += floodFillReachableTiles(state, pos);
        }

        for (const auto& pos : state.queenPositions(opponent)) {
            opponentReachable += floodFillReachableTiles(state, pos);
        }

        return static_cast<int>(playerReachable) - static_cast<int>(opponentReachable);
    }

    inline int spatialInfluenceScore(const GameState& state, Player player) {
        const auto& board = state.board();
        int dim = board.dimension();

        if (dim == 0) {
            return 0;
        }

        double center = (dim - 1) / 2.0;

        auto positionalValue = [&](const Position& pos) {
            double dist = std::abs(pos.row - center) + std::abs(pos.col - center);
            double maxDist = 2.0 * (dim - 1);
            double normalized = 1.0 - (dist / maxDist);
            auto mobility = gatherReachableTiles(board, pos).size();
            return static_cast<double>(mobility) * 0.25 + normalized * 10.0;
            };

        double score = 0.0;
        for (const auto& pos : state.queenPositions(player)) {
            score += positionalValue(pos);
        }

        for (const auto& pos : state.queenPositions(opponentOf(player))) {
            score -= positionalValue(pos);
        }

        return static_cast<int>(score);
    }

    inline int evaluateTerminal(const GameState& state, Player perspective) {
        if (!state.isFinished()) {
            return 0;
        }

        if (state.winner() == perspective) {
            return std::numeric_limits<int>::max() / 4;
        }

        if (state.winner() == opponentOf(perspective)) {
            return std::numeric_limits<int>::min() / 4;
        }

        return 0;
    }

    inline bool isTerminal(const GameState& state) {
        if (state.isFinished()) {
            return true;
        }
        return !hasAnyLegalMove(state, state.currentPlayer());
    }

}

inline int evaluate(const GameState& state, Player perspective) {
    if (state.isFinished()) {
        return detail::evaluateTerminal(state, perspective);
    }

    Player opponent = opponentOf(perspective);
    const int mobilityWeight = 3;
    const int territoryWeight = 2;
    const int spatialWeight = 1;

    int mobility = detail::mobilityCount(state, perspective) - detail::mobilityCount(state, opponent);
    int territory = detail::territoryScore(state, perspective);
    int spatial = detail::spatialInfluenceScore(state, perspective);

    return mobilityWeight * mobility + territoryWeight * territory + spatialWeight * spatial;
}

inline int minimax(GameState& state, int depth, int alpha, int beta, Player maximizingPlayer,
    Player perspective, std::size_t moveCap, const std::atomic_bool* cancel = nullptr) {
    if (cancel && cancel->load()) {
        throw SearchCanceled();
    }

    bool terminal = depth == 0 || detail::isTerminal(state);
    if (terminal) {
        GameState evalState = state;
        evaluateWinState(evalState);
        return detail::evaluateTerminal(evalState, perspective);
    }

    Player current = state.currentPlayer();
    bool isMaximizing = current == maximizingPlayer;
    auto moves = generyyateMovesForPlayer(state, current, moveCap);

    if (moves.empty()) {
        GameState evalState = state;
        evaluateWinState(evalState);
        return detail::evaluateTerminal(evalState, perspective);
    }

    if (isMaximizing) {
        int value = std::numeric_limits<int>::min();
        for (const auto& move : moves) {
            if (cancel && cancel->load()) {
                throw SearchCanceled();
            }
            GameState next = state.clone();
            applyMove(next, move);
            int child = minimax(next, depth - 1, alpha, beta, maximizingPlayer, perspective, moveCap, cancel);
            value = std::max(value, child);
            alpha = std::max(alpha, value);
            if (alpha >= beta) {
                break;
            }
        }
        return value;
    }

    int value = std::numeric_limits<int>::max();
    for (const auto& move : moves) {
        if (cancel && cancel->load()) {
            throw SearchCanceled();
        }
        GameState next = state.clone();
        applyMove(next, move);
        int child = minimax(next, depth - 1, alpha, beta, maximizingPlayer, perspective, moveCap, cancel);
        value = std::min(value, child);
        beta = std::min(beta, value);
        if (alpha >= beta) {
            break;
        }
    }
    return value;
}

inline int depthForDifficulty(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 1;
    case Difficulty::Medium:
        return 2;
    case Difficulty::Hard:
    default:
        return 3;
    }
}

inline std::size_t moveCapForDifficulty(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 6;
    case Difficulty::Medium:
        return 12;
    case Difficulty::Hard:
    default:
        return 14;
    }
}

inline Move getBestMove(const GameState& state, Difficulty difficulty, const std::atomic_bool* cancel = nullptr) {
    GameState rootState = state;
    std::size_t moveCap = moveCapForDifficulty(difficulty);
    auto moves = generateMovesForPlayer(rootState, rootState.currentPlayer(), moveCap);

    if (moves.empty()) {
        return {};
    }

    int searchDepth = std::max(1, depthForDifficulty(difficulty));
    Player maximizingPlayer = rootState.currentPlayer();
    Player perspective = maximizingPlayer;

    struct ScoredMove {
        Move move;
        int heuristic;
    };

    std::vector<ScoredMove> scored;
    scored.reserve(moves.size());

    for (const auto& move : moves) {
        GameState next = rootState.clone();
        applyMove(next, move);
        int heuristic = evaluate(next, perspective);
        scored.push_back({ move, heuristic });
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredMove& a, const ScoredMove& b) {
        return a.heuristic > b.heuristic;
        });

    Move bestMove = scored.front().move;
    int bestScore = std::numeric_limits<int>::min();

    int primaryDepth = std::max(0, searchDepth - 1);
    int shallowDepth = std::max(0, primaryDepth - 1);
    std::size_t deepSlots = (difficulty == Difficulty::Hard && scored.size() > 6) ? 6 : scored.size();

    for (std::size_t idx = 0; idx < scored.size(); ++idx) {
        const auto& entry = scored[idx];
        GameState next = rootState.clone();
        applyMove(next, entry.move);

        int depthForMove = primaryDepth;
        if (difficulty == Difficulty::Hard && idx >= deepSlots) {
            depthForMove = shallowDepth;
        }

        int score = minimax(next, depthForMove, std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max(), maximizingPlayer, perspective, moveCap, cancel);

        if (score > bestScore) {
            bestScore = score;
            bestMove = entry.move;
        }
    }

    return bestMove;
}
