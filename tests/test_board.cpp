#include <cassert>
#include <iostream>

#include "../src/board.h"

using namespace ludica;

int main() {
  // Merge: [2,2,2,2] slid left -> [4,4,0,0]; two merges, never a triple.
  {
    Board b(std::array<int, 16>{2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    assert(b.move(Direction::Left));
    assert(b.at(0, 0) == 4 && b.at(0, 1) == 4 && b.at(0, 2) == 0 && b.at(0, 3) == 0);
    assert(b.score() == 8);
  }

  // [2,2,4,0] slid left -> [4,4,0,0], score += 4.
  {
    Board b(std::array<int, 16>{2, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    assert(b.move(Direction::Left));
    assert(b.at(0, 0) == 4 && b.at(0, 1) == 4 && b.at(0, 2) == 0 && b.at(0, 3) == 0);
    assert(b.score() == 4);
  }

  // A swipe that changes nothing must report no change and add no score.
  {
    Board b(std::array<int, 16>{2, 4, 8, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    assert(!b.move(Direction::Left));
    assert(b.score() == 0);
    assert(!b.canMove(Direction::Left));
  }

  // Full board, no equal neighbors anywhere -> game over.
  {
    Board b(std::array<int, 16>{2, 4, 2, 4, 4, 2, 4, 2, 2, 4, 2, 4, 4, 2, 4, 2});
    assert(b.legalMoves().empty());
    assert(b.isGameOver());
  }

  // Full board with one merge available -> still has a legal move.
  {
    Board b(std::array<int, 16>{2, 2, 4, 8, 4, 2, 8, 16, 2, 8, 16, 32, 4, 16, 32, 64});
    assert(!b.legalMoves().empty());
    assert(!b.isGameOver());
  }

  std::cout << "all board tests passed\n";
  return 0;
}
