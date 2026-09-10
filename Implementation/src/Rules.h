#pragma once
#ifndef RULES_H
#define RULES_H

#include <algorithm>
#include <limits>
#include <array>
#include <stdexcept>
#include <vector>
#include "GameState.h"

// Map a Player enum to its corresponding TileContent (queen piece)
inline TileContent tileForPlayer(Player player) {
	switch (player) {
	case Player::White:
		return TileContent::WhiteQueen;
	case Player::Black:
		return TileContent::BlackQueen;
	default:
		throw std::runtime_error("Invalid for tile mapping");
	}
}
// Casts a "ray" from a starting position in direction (dx, dy) and collects all empty tiles until hitting a non-empty tile or border
using PositionList = std::vector<Position>;


inline PositionList rayCast(const Board& board, const Position& start, int dx, int dy) {
	PositionList tiles;
	if (!board.isInsideBoard(start.row, start.col)) {
		return tiles;
	}

	int row = start.row + dx;
	int col = start.col + dy;

	// Continue moving in the direction until blocked
	while (board.isInsideBoard(row, col)) {
		auto tile = board.getTile(row, col);
		if (tile != TileContent::Empty) {
			break;
		}
		tiles.push_back({ row, col });
		row += dx;
		col += dy;
	}
	return tiles;
}

// Returns all reachable tiles from a given position
inline PositionList gatherReachableTiles(const Board& board, const Position& start) {
	static constexpr std::array<std::pair<int, int>, 8> kDirections = {
		std::pair{1, 0},  std::pair{-1, 0}, std::pair{0, 1},  std::pair{0, -1},
		std::pair{1, 1},  std::pair{1, -1}, std::pair{-1, 1}, std::pair{-1, -1}
	};

	PositionList reachable;
	for (const auto& [dx, dy] : kDirections) {
		auto ray = rayCast(board, start, dx, dy);
		reachable.insert(reachable.end(), ray.begin(), ray.end());
	}
	return reachable;
}

// Check if a list of positions contains a specific target
inline bool containsPosition(const PositionList& positions, const Position& target) {
	return std::any_of(positions.begin(), positions.end(),
		[&target](const Position& pos) { return pos == target; });
}

// Generates all legal moves for a given player
// Uses moveCap to allow limiting results
inline std::vector<Move> generateMovesForPlayer(const GameState& state, Player player,
	std::size_t moveCap = (std::numeric_limits<std::size_t>::max)()) {

	std::vector<Move> moves;

	if (player == Player::None) {
		return moves;
	}

	const auto& board = state.board();
	const auto& queens = state.queenPositions(player);

	// For each queen belonging to the player
	for (const auto& queenPos : queens) {
		auto queenTargets = gatherReachableTiles(board, queenPos);

		// For each possible queen destination
		for (const auto& queenDest : queenTargets) {

			// Simulate the queen move
			Board simulatedBoard = board;
			simulatedBoard.setTile(queenPos.row, queenPos.col, TileContent::Empty);
			simulatedBoard.setTile(queenDest.row, queenDest.col, tileForPlayer(player));

			// Now find arrow targets using modified board
			auto arrowTargets = gatherReachableTiles(simulatedBoard, queenDest);

			// Combine queen move + arrow shot into full moves
			for (const auto& arrowDest : arrowTargets) {
				moves.push_back({ player, queenPos, queenDest, arrowDest });

				// Stop early if moveCap exceeded
				if (moves.size() >= moveCap)
					return moves;
			}
		}
	}
	return moves;
}

// Check if the player has any legal move (used for detecting checkmate-like situations)
inline bool hasAnyLegalMove(const GameState& state, Player player) {
	if (player == Player::None) {
		return false;
	}

	const auto& board = state.board();
	const auto& queens = state.queenPositions(player);

	for (const auto& queenPos : queens) {
		auto queenTargets = gatherReachableTiles(board, queenPos);
		if (queenTargets.empty())
			continue;

		for (const auto& queenDest : queenTargets) {
			// Simulate queen move
			Board simulatedBoard = board;
			simulatedBoard.setTile(queenPos.row, queenPos.col, TileContent::Empty);
			simulatedBoard.setTile(queenDest.row, queenDest.col, tileForPlayer(player));

			// Check if any arrow target exists
			auto arrowTargets = gatherReachableTiles(simulatedBoard, queenDest);
			if (!arrowTargets.empty())
				return true;
		}
	}
	return false;
}

// Full legality check for a move 
inline bool isMoveLegal(const GameState& state, const Move& move) {
	// Cannot move if game ended
	if (state.isFinished())
		return false;

	// Must be correct player's turn
	if (move.player != state.currentPlayer())
		return false;

	const auto& board = state.board();

	// All positions must be inside the board
	if (!board.isInsideBoard(move.queenFrom.row, move.queenFrom.col) ||
		!board.isInsideBoard(move.queenTo.row, move.queenTo.col) ||
		!board.isInsideBoard(move.arrow.row, move.arrow.col)) {
		return false;
	}

	// Starting tile must contain player's queen
	auto expectedTile = tileForPlayer(move.player);
	if (board.getTile(move.queenFrom.row, move.queenFrom.col) != expectedTile)
		return false;

	// Queen movement must be valid
	auto queenTargets = gatherReachableTiles(board, move.queenFrom);
	if (!containsPosition(queenTargets, move.queenTo))
		return false;

	// Simulate queen move to validate arrow move
	Board simulatedBoard = board;
	simulatedBoard.setTile(move.queenFrom.row, move.queenFrom.col, TileContent::Empty);
	simulatedBoard.setTile(move.queenTo.row, move.queenTo.col, expectedTile);

	auto arrowTargets = gatherReachableTiles(simulatedBoard, move.queenTo);
	if (!containsPosition(arrowTargets, move.arrow))
		return false;

	return true;
}

// Determines if the game should end after a move
inline bool evaluateWinState(GameState& state) {
	if (state.isFinished())
		return true;

	Player current = state.currentPlayer();

	if (!hasAnyLegalMove(state, current)) {
		// No moves - opponent wins
		state.markFinished(opponentOf(current));
		return true;
	}
	return false;
}

inline void applyMove(GameState& state, const Move& move) {
	if (!isMoveLegal(state, move)) {
		throw std::invalid_argument("Tried to play illegal move!");
	}

	// Move queen
	state.updateQueenPosition(move.player, move.queenFrom, move.queenTo);

	// Place arrow on board
	state.addArrow(move.arrow);

	// Record move in history
	state.recordMove(move);

	// Next player's turn
	state.setCurrentPlayer(opponentOf(move.player));

	// Evaluate if someone wins after move
	evaluateWinState(state);
}

#endif
