#include "expectimax.h"

#include <cmath>
#include <limits>
#include <utility>

namespace ludica {

namespace {

double expectedValue(const Board& b, int depth);  // forward: mutual recursion

constexpr double kInf = std::numeric_limits<double>::infinity();

// log2 of a tile; 0 (empty) maps to 0 so gaps are compared by exponent.
double log2v(int v) { return v == 0 ? 0.0 : std::log2(static_cast<double>(v)); }

// Static leaf evaluation. The weights are classic 2048 values — good, not
// optimal. They trade off open space, a smooth board, and a monotone "snake"
// that funnels the big tiles toward the top-left corner.
double evaluate(const Board& b) {
  constexpr double kEmpty = 270.0;
  constexpr double kSmoothness = 0.1;
  constexpr double kMonotonicity = 47.0;

  // Empty cells, counted inline to avoid allocating on this hot path.
  int empty = 0;
  for (int r = 0; r < Board::kSize; ++r)
    for (int c = 0; c < Board::kSize; ++c)
      if (b.at(r, c) == 0) ++empty;

  // Smoothness: sum of |exponent gaps| between horizontal/vertical neighbors.
  double smoothness = 0.0;
  for (int r = 0; r < Board::kSize; ++r)
    for (int c = 0; c < Board::kSize; ++c) {
      if (c + 1 < Board::kSize)
        smoothness += std::abs(log2v(b.at(r, c)) - log2v(b.at(r, c + 1)));
      if (r + 1 < Board::kSize)
        smoothness += std::abs(log2v(b.at(r, c)) - log2v(b.at(r + 1, c)));
    }

  // Monotonicity: reward tiles decreasing toward the top-left.
  double monotonicity = 0.0;
  for (int r = 0; r < Board::kSize; ++r)
    for (int c = 0; c + 1 < Board::kSize; ++c)
      monotonicity += static_cast<double>(b.at(r, c)) - b.at(r, c + 1);
  for (int c = 0; c < Board::kSize; ++c)
    for (int r = 0; r + 1 < Board::kSize; ++r)
      monotonicity += static_cast<double>(b.at(r, c)) - b.at(r + 1, c);

  // Bonus for keeping the largest tile in a corner.
  int n = Board::kSize - 1;
  int maxTile = b.maxTile();
  double corner = (b.at(0, 0) == maxTile || b.at(0, n) == maxTile ||
                   b.at(n, 0) == maxTile || b.at(n, n) == maxTile)
                      ? 1.0
                      : 0.0;

  return kEmpty * empty + kMonotonicity * monotonicity
         - kSmoothness * smoothness + 10000.0 * corner;
}

// The best move from a max node, and its value. `depth` must be >= 1.
// Shared by searchMax (value only) and chooseMove (move only).
std::pair<Direction, double> bestMove(const Board& b, int depth) {
  auto legal = b.legalMoves();
  Direction best = legal.front();
  double bestVal = -kInf;
  for (Direction d : legal) {
    Board next = b;
    next.move(d);
    double v = expectedValue(next, depth - 1);
    if (v > bestVal) {
      bestVal = v;
      best = d;
    }
  }
  return {best, bestVal};
}

// Max node: the best value reachable by one legal move from `b`. This is the
// player's choice, so we maximize.
double searchMax(const Board& b, int depth) {
  if (depth == 0) return evaluate(b);
  if (b.legalMoves().empty()) return evaluate(b);  // terminal; unreachable after a move
  return bestMove(b, depth).second;
}

// Chance node: the average value over every possible random spawn. This is
// the game's choice, so we take the expectation, not the max.
// shortcut: only spawns 2s (4s are 10% of spawns; ignoring them is a standard,
//   small simplification that halves the work). Add the 4-spawn term here to
//   restore the exact expectation when we want it.
double expectedValue(const Board& b, int depth) {
  auto empty = b.emptyCells();
  if (empty.empty()) return evaluate(b);  // a changed board always leaves a gap

  double sum = 0.0;
  for (int idx : empty) {
    Board spawned = b;
    spawned.spawnAt(idx, 2);
    sum += searchMax(spawned, depth);
  }
  return sum / static_cast<double>(empty.size());
}

}  // namespace

Decision ExpectimaxPolicy::chooseMove(const Board& board) {
  Decision d;
  d.move = bestMove(board, depth_).first;
  return d;
}

std::string ExpectimaxPolicy::name() const {
  return "expectimax(depth=" + std::to_string(depth_) + ")";
}

}  // namespace ludica
