#pragma once

#include <array>
#include <random>
#include <vector>

namespace ludica {

// One of the four swipes in 2048.
enum class Direction { Up, Down, Left, Right };

// The 2048 board and its rules. Pure logic: no rendering, no AI here.
//
// This is the "code owns the rules" layer. A policy only ever *reads* the
// board and returns a Direction; the game loop applies the move and spawns the
// random tile. Keeping the rules here means both policies (search, decision
// model) and the renderer all share one source of truth.
class Board {
public:
  static constexpr int kSize = 4;
  static constexpr int kCellCount = kSize * kSize;

  Board();  // empty board, RNG seeded from hardware entropy
  explicit Board(std::array<int, kCellCount> cells);  // deterministic setup, for tests

  void reset();  // clear and spawn the two starting tiles

  int at(int row, int col) const { return cells_[row * kSize + col]; }

  // Which moves would change the board right now? Never mutates.
  std::vector<Direction> legalMoves() const;
  bool canMove(Direction d) const;

  // Apply a swipe if legal. Returns true if the board changed, and adds the
  // merged tile values to score().
  bool move(Direction d);

  // Add a 2 (90%) or 4 (10%) in a random empty cell. No-op when full.
  void spawnRandom();

  // Deterministic search helpers. emptyCells() lists empty slots by row-major
  // index; spawnAt() places a tile in one. The search uses these to enumerate
  // the possible random spawns without touching the real RNG.
  std::vector<int> emptyCells() const;
  void spawnAt(int index, int value);

  bool isGameOver() const { return legalMoves().empty(); }

  int score() const { return score_; }
  int maxTile() const;

private:
  // One line (row or column) of the board, oriented so the move direction
  // points at index 0. Lets slide/move logic be written once for "left".
  std::array<int, kSize> getLine(Direction d, int i) const;
  void setLine(Direction d, int i, const std::array<int, kSize>& line);

  std::array<int, kCellCount> cells_{};
  int score_ = 0;
  std::mt19937 rng_;
};

}  // namespace ludica
