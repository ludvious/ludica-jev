#pragma once

#include <array>
#include <string>

#include "board.h"

namespace ludica {

// Index of a Direction into a 4-element array (Up, Down, Left, Right). Lets a
// policy store per-direction values (e.g. probabilities) without a std::map.
constexpr int dirIndex(Direction d) {
  switch (d) {
    case Direction::Up:    return 0;
    case Direction::Down:  return 1;
    case Direction::Left:  return 2;
    case Direction::Right: return 3;
  }
  return 0;
}

// The result of asking a policy for a move: the move, plus optional diagnostics.
// Expectimax fills only `move`; the decision model fills the rest. confidence
// stays -1 when there are no diagnostics.
struct Decision {
  Direction move{};
  double confidence = -1.0;               // [0,1], or -1 = no diagnostics
  std::array<double, 4> probabilities{};  // per dirIndex(); 0 = unavailable
  double latencyMs = 0.0;
  bool fallback = false;                  // model was down or unusable
  std::string model;                      // served model name, if reported
};

// A MovePolicy decides the next swipe for a given board.
//
// This is the seam where search, RL, and decision-model policies all plug in.
// The contract is deliberately tiny: read the board, return a Decision.
// A policy never mutates the board and never spawns tiles; the game loop does
// that. That separation is what keeps every policy safe to swap and to test.
class MovePolicy {
public:
  virtual ~MovePolicy() = default;

  // Precondition: board.legalMoves() is non-empty (caller checks game over).
  virtual Decision chooseMove(const Board& board) = 0;

  // Human-readable label, for logs and the on-screen HUD.
  virtual std::string name() const = 0;
};

}