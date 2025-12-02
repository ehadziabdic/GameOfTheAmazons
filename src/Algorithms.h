// Implement minmax with alfa-beta pruning (3 depths for now)- Armin

#pragma once

#include "GameState.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <vector>

class AmazonsAlgorithm {
public:
    // ========================================================================
    // MAIN ENGINE API (Called by GUI)
    // ========================================================================

    /**
     * Returns the best move for the AI based on the current game state and difficulty.
     * This is the main entry point called by the GUI when it's the AI's turn.
     */
    static Move getBestMove(const GameState& state, Difficulty difficulty) {
        const Player aiPlayer = state.currentPlayer();
        const int searchDepth = depthForDifficulty(difficulty);

        std::vector<Move> legalMoves = generateLegalMoves(state, aiPlayer);

        if (legalMoves.empty()) {
            return Move{}; // No legal moves available
        }

        Move bestMove = legalMoves[0];
        int bestScore = std::numeric_limits<int>::min();
        int alpha = std::numeric_limits<int>::min();
        int beta = std::numeric_limits<int>::max();

        // Evaluate each legal move
        for (const Move& move : legalMoves) {
            GameState newState = state.clone();
            applyMove(newState, move);

            // Call minimax for opponent's response
            int score = minimax(newState, searchDepth - 1, alpha, beta, false, aiPlayer);

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }

            alpha = std::max(alpha, score);
        }

        return bestMove;
    }

    // ========================================================================
    // MOVE GENERATION (Task 2 - needed for AI)
    // ========================================================================

    /**
     * Ray casting: walks in a direction (dx, dy) until hitting a board edge, queen, or arrow.
     * Returns all reachable empty tiles in that direction.
     */
    static std::vector<Position> rayCast(const Board& board, const Position& start, int dx, int dy) {
        std::vector<Position> reachable;
        int row = start.row + dx;
        int col = start.col + dy;

        while (board.isInsideBoard(row, col)) {
            TileContent tile = board.getTile(row, col);

            if (tile != TileContent::Empty) {
                break; // Hit a queen or arrow
            }

            reachable.push_back({ row, col });
            row += dx;
            col += dy;
        }

        return reachable;
    }

    /**
     * Generate all valid destination squares for a queen using ray casting in 8 directions.
     */
    static std::vector<Position> generateQueenDestinations(const Board& board, const Position& queenPos) {
        std::vector<Position> destinations;

        // 8 directions: horizontal, vertical, and diagonal
        const std::vector<std::pair<int, int>> directions = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1},     // vertical and horizontal
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}    // diagonals
        };

        for (const auto& [dx, dy] : directions) {
            std::vector<Position> tiles = rayCast(board, queenPos, dx, dy);
            destinations.insert(destinations.end(), tiles.begin(), tiles.end());
        }

        return destinations;
    }

    /**
     * Generate all legal moves (queen movement + arrow placement) for a given player.
     */
    static std::vector<Move> generateLegalMoves(const GameState& state, Player player) {
        std::vector<Move> allMoves;
        const Board& board = state.board();
        const std::vector<Position>& queens = state.queenPositions(player);

        for (const Position& queenPos : queens) {
            // Generate all possible queen destinations
            std::vector<Position> queenDestinations = generateQueenDestinations(board, queenPos);

            for (const Position& queenDest : queenDestinations) {
                // Temporarily move queen to simulate arrow shooting
                Board tempBoard = board;
                tempBoard.setTile(queenPos.row, queenPos.col, TileContent::Empty);
                tempBoard.setTile(queenDest.row, queenDest.col,
                    player == Player::White ? TileContent::WhiteQueen : TileContent::BlackQueen);

                // Generate arrow destinations from new queen position
                std::vector<Position> arrowDestinations = generateQueenDestinations(tempBoard, queenDest);

                // Create full moves (queen movement + arrow placement)
                for (const Position& arrowDest : arrowDestinations) {
                    Move move;
                    move.player = player;
                    move.queenFrom = queenPos;
                    move.queenTo = queenDest;
                    move.arrow = arrowDest;
                    allMoves.push_back(move);
                }
            }
        }

        return allMoves;
    }

    // ========================================================================
    // WIN CHECKING
    // ========================================================================

    /**
     * Check if a player has no legal moves (loses the game).
     */
    static bool hasNoLegalMoves(const GameState& state, Player player) {
        return generateLegalMoves(state, player).empty();
    }

    /**
     * Check if the game has reached a terminal state.
     */
    static bool isTerminal(const GameState& state) {
        if (state.isFinished()) {
            return true;
        }
        return hasNoLegalMoves(state, state.currentPlayer());
    }

    // ========================================================================
    // HEURISTIC EVALUATION COMPONENTS
    // ========================================================================

    /**
     * Mobility heuristic: count legal moves for the player.
     * More moves = better position.
     */
    static int calculateMobility(const GameState& state, Player player) {
        return static_cast<int>(generateLegalMoves(state, player).size());
    }

    /**
     * Territory heuristic: flood-fill from each queen to count reachable tiles.
     * Uses BFS with queen-style movement (8 directions).
     */
    static int calculateTerritory(const GameState& state, Player player) {
        const Board& board = state.board();
        const std::vector<Position>& queens = state.queenPositions(player);

        std::set<std::pair<int, int>> visited;
        std::queue<Position> queue;

        // Start BFS from all queens of this player
        for (const Position& queenPos : queens) {
            queue.push(queenPos);
            visited.insert({ queenPos.row, queenPos.col });
        }

        // 8 directions for queen movement
        const std::vector<std::pair<int, int>> directions = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1},
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
        };

        int territoryCount = 0;

        while (!queue.empty()) {
            Position current = queue.front();
            queue.pop();

            for (const auto& [dx, dy] : directions) {
                int newRow = current.row + dx;
                int newCol = current.col + dy;

                if (!board.isInsideBoard(newRow, newCol)) {
                    continue;
                }

                if (visited.count({ newRow, newCol })) {
                    continue;
                }

                TileContent tile = board.getTile(newRow, newCol);

                // Can only traverse empty tiles or own queens
                if (tile == TileContent::Empty ||
                    (player == Player::White && tile == TileContent::WhiteQueen) ||
                    (player == Player::Black && tile == TileContent::BlackQueen)) {

                    visited.insert({ newRow, newCol });
                    queue.push({ newRow, newCol });
                    territoryCount++;
                }
            }
        }

        return territoryCount;
    }

    /**
     * Spatial influence heuristic: bonus for queens near center, penalty for edge positions.
     */
    static int calculateSpatialInfluence(const GameState& state, Player player) {
        const Board& board = state.board();
        const std::vector<Position>& queens = state.queenPositions(player);
        const int dim = board.dimension();
        const double center = dim / 2.0;

        int spatialScore = 0;

        for (const Position& queenPos : queens) {
            // Calculate distance from center (Manhattan distance)
            double distFromCenter = std::abs(queenPos.row - center) + std::abs(queenPos.col - center);

            // Bonus for being near center (max bonus at center, decreases with distance)
            int centerBonus = static_cast<int>((dim - distFromCenter) * 2);

            // Penalty if on the edge with limited mobility
            int edgePenalty = 0;
            if (queenPos.row == 0 || queenPos.row == dim - 1 ||
                queenPos.col == 0 || queenPos.col == dim - 1) {

                // Count available moves from this position
                std::vector<Position> moves = generateQueenDestinations(board, queenPos);
                if (moves.size() < 5) {
                    edgePenalty = 10; // Penalty for trapped edge queen
                }
            }

            spatialScore += centerBonus - edgePenalty;
        }

        return spatialScore;
    }

    // ========================================================================
    // COMBINED EVALUATION FUNCTION
    // ========================================================================

    /**
     * Evaluate the game state for a given player.
     * Combines mobility, territory, and spatial influence with weights.
     * Higher score = better position for the player.
     */
    static int evaluate(const GameState& state, Player player) {
        const Player opponent = opponentOf(player);

        // Calculate heuristics for both players
        int playerMobility = calculateMobility(state, player);
        int opponentMobility = calculateMobility(state, opponent);

        int playerTerritory = calculateTerritory(state, player);
        int opponentTerritory = calculateTerritory(state, opponent);

        int playerSpatial = calculateSpatialInfluence(state, player);
        int opponentSpatial = calculateSpatialInfluence(state, opponent);

        // Combine with weights (difference between player and opponent)
        int mobilityScore = (playerMobility - opponentMobility) * MOBILITY_WEIGHT;
        int territoryScore = (playerTerritory - opponentTerritory) * TERRITORY_WEIGHT;
        int spatialScore = (playerSpatial - opponentSpatial) * SPATIAL_WEIGHT;

        return mobilityScore + territoryScore + spatialScore;
    }

    // ========================================================================
    // MINIMAX WITH ALPHA-BETA PRUNING
    // ========================================================================

    /**
     * Minimax algorithm with alpha-beta pruning.
     * Returns the evaluation score for the given state.
     *
     * @param state Current game state
     * @param depth Remaining search depth
     * @param alpha Best score for maximizing player
     * @param beta Best score for minimizing player
     * @param maximizingPlayer True if current player is maximizing
     * @param originalPlayer The AI player we're optimizing for
     */
    static int minimax(GameState state, int depth, int alpha, int beta,
        bool maximizingPlayer, Player originalPlayer) {

        // Terminal conditions: depth reached or game over
        if (depth == 0 || isTerminal(state)) {
            return evaluate(state, originalPlayer);
        }

        Player currentPlayer = state.currentPlayer();
        std::vector<Move> legalMoves = generateLegalMoves(state, currentPlayer);

        // If no legal moves, opponent wins
        if (legalMoves.empty()) {
            Player winner = opponentOf(currentPlayer);
            // Large penalty/bonus for losing/winning
            return winner == originalPlayer ? 100000 : -100000;
        }

        if (maximizingPlayer) {
            int maxEval = std::numeric_limits<int>::min();

            for (const Move& move : legalMoves) {
                GameState newState = state.clone();
                applyMove(newState, move);

                int eval = minimax(newState, depth - 1, alpha, beta, false, originalPlayer);
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);

                // Alpha-beta pruning
                if (beta <= alpha) {
                    break;
                }
            }

            return maxEval;

        }
        else {
            int minEval = std::numeric_limits<int>::max();

            for (const Move& move : legalMoves) {
                GameState newState = state.clone();
                applyMove(newState, move);

                int eval = minimax(newState, depth - 1, alpha, beta, true, originalPlayer);
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);

                // Alpha-beta pruning
                if (beta <= alpha) {
                    break;
                }
            }

            return minEval;
        }
    }

    // ========================================================================
    // MOVE VALIDATION AND APPLICATION (Task 2)
    // ========================================================================

    /**
     * Validate a selected move for GUI user input.
     * Verifies:
     * - The selected piece is the current player's queen
     * - The queen destination is legally reachable
     * - The arrow destination is legally reachable from the new queen position
     * - All paths are unobstructed
     */
    static bool validateMove(const GameState& state, const Move& move) {
        const Board& board = state.board();
        const Player currentPlayer = state.currentPlayer();

        // 1. Verify it's the current player's move
        if (move.player != currentPlayer) {
            return false;
        }

        // 2. Verify the starting position contains the player's queen
        if (!board.isInsideBoard(move.queenFrom.row, move.queenFrom.col)) {
            return false;
        }

        TileContent fromTile = board.getTile(move.queenFrom.row, move.queenFrom.col);
        if ((currentPlayer == Player::White && fromTile != TileContent::WhiteQueen) ||
            (currentPlayer == Player::Black && fromTile != TileContent::BlackQueen)) {
            return false;
        }

        // 3. Verify the queen destination is valid and reachable
        if (!board.isInsideBoard(move.queenTo.row, move.queenTo.col)) {
            return false;
        }

        // Check if queen destination is in the list of legal queen moves
        std::vector<Position> validQueenDests = generateQueenDestinations(board, move.queenFrom);
        bool queenDestValid = std::find(validQueenDests.begin(), validQueenDests.end(), move.queenTo)
            != validQueenDests.end();

        if (!queenDestValid) {
            return false;
        }

        // 4. Verify the arrow destination is valid and reachable from new queen position
        if (!board.isInsideBoard(move.arrow.row, move.arrow.col)) {
            return false;
        }

        // Simulate queen at new position to check arrow destinations
        Board tempBoard = board;
        tempBoard.setTile(move.queenFrom.row, move.queenFrom.col, TileContent::Empty);
        tempBoard.setTile(move.queenTo.row, move.queenTo.col,
            currentPlayer == Player::White ? TileContent::WhiteQueen : TileContent::BlackQueen);

        std::vector<Position> validArrowDests = generateQueenDestinations(tempBoard, move.queenTo);
        bool arrowDestValid = std::find(validArrowDests.begin(), validArrowDests.end(), move.arrow)
            != validArrowDests.end();

        if (!arrowDestValid) {
            return false;
        }

        // All checks passed
        return true;
    }

    /**
     * Apply a move to the game state (modifies the state).
     * Note: For GUI usage, call validateMove() first before applying.
     */
    static void applyMove(GameState& state, const Move& move) {
        // Move the queen
        state.updateQueenPosition(move.player, move.queenFrom, move.queenTo);

        // Place the arrow
        state.addArrow(move.arrow);

        // Record move in history
        state.recordMove(move);

        // Switch player
        state.setCurrentPlayer(opponentOf(move.player));
    }

    /**
     * Map difficulty level to search depth (plies).
     */
    static int depthForDifficulty(Difficulty difficulty) {
        switch (difficulty) {
        case Difficulty::Easy:
            return 8;   // 8 plies (4 moves ahead)
        case Difficulty::Medium:
            return 10;  // 10 plies (5 moves ahead)
        case Difficulty::Hard:
            return 12;  // 12 plies (6 moves ahead)
        default:
            return 10;
        }
    }

private:
    // Heuristic weights for evaluation function
    static constexpr int MOBILITY_WEIGHT = 10;
    static constexpr int TERRITORY_WEIGHT = 5;
    static constexpr int SPATIAL_WEIGHT = 2;
};
