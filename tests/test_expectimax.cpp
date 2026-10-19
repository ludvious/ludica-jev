#include <algorithm>
#include <cassert>
#include <iostream>

#include "../src/board.h"
#include "../src/expectimax.h"

using namespace ludica;

static bool isLegal(const Board& b, Direction d) {
  auto legal = b.legalMoves();
  return std::find(legal.begin(), legal.end(), d) != legal.end();
}

int main() {
  ExpectimaxPolicy policy(2);  // shallow depth keeps this test fast

  // Mid-game boards (each definitely has at least one legal move).
  std::array<int, 16> states[] = {
      {2, 2, 4, 8, 4, 8, 16, 32, 8, 16, 32, 64, 16, 32, 64, 128},  // a merge is open
      {0, 2, 0, 4, 2, 0, 4, 0, 0, 4, 0, 8, 4, 0, 8, 0},            // sparse board
      {2, 4, 8, 16, 4, 8, 16, 32, 8, 16, 32, 64, 16, 32, 64, 0},   // one gap
  };

  for (auto& cells : states) {
    Board b(cells);
    assert(!b.isGameOver());
    Decision d = policy.chooseMove(b);
    assert(isLegal(b, d.move));                    // never picks an illegal move
    assert(policy.chooseMove(b).move == d.move);   // deterministic on the same board
  }

  std::cout << "all expectimax tests passed\n";
  return 0;
}
