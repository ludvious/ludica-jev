#pragma once

#include <algorithm>

#include "policy.h"

namespace ludica {

// Expectimax search: for each legal move, average over every possible random
// spawn, recurse, and score the leaves with a static heuristic. This is the
// "how well can a real AI do" ceiling that the decision model is measured
// against. 2048 is stochastic (a random tile spawns after each move), so the
// correct search is expectimax, not plain minimax.
class ExpectimaxPolicy : public MovePolicy {
public:
  // `depth` = how many moves ahead to search (>= 1). Higher is stronger but
  // exponentially slower; 4 is a good default for turn-based play.
  explicit ExpectimaxPolicy(int depth = 4) : depth_(std::max(1, depth)) {}

  Decision chooseMove(const Board& board) override;
  std::string name() const override;

private:
  int depth_;
};

}  // namespace ludica
