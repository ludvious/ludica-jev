#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "../src/board.h"
#include "../src/decision_model.h"

using namespace ludica;

int main() {
  // Serializer: strategy language, not just raw numbers.
  {
    Board b(std::array<int, 16>{2, 8, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    std::string s = boardToState(b);
    assert(s.find("Largest tile: 8") != std::string::npos);
    assert(s.find("(not in a corner)") != std::string::npos);
    assert(s.find("Empty cells: 13") != std::string::npos);
  }

  // Request: only the legal moves are offered, each with its consequence.
  {
    Board b(std::array<int, 16>{2, 2, 4, 8, 4, 8, 16, 32, 8, 16, 32, 64, 16, 32, 64, 128});
    auto legal = b.legalMoves();
    auto req = buildMoveRequest("jev-latest", b, legal);
    assert(req["model"] == "jev-latest");
    assert(req["questions"]["move"]["type"] == "choice");
    auto crit = req["questions"]["move"]["criteria"];
    assert(crit.size() == legal.size());
    for (Direction d : legal) {
      assert(crit.contains(directionName(d)));
      std::string desc = crit[directionName(d)].get<std::string>();
      assert(desc.find("points") != std::string::npos);
    }
  }

  // Response parsing: good choice with probabilities, illegal choice, malformed.
  {
    std::vector<Direction> legal = {Direction::Up, Direction::Left, Direction::Right};
    std::string good =
        R"({"model":"jevos-v5","answers":{"move":{"type":"choice","choice":"left","probabilities":{"left":0.7,"up":0.2,"right":0.1},"confidence":0.7}}})";
    auto d = parseDecision(good, legal);
    assert(d.has_value());
    assert(d->move == Direction::Left);
    assert(d->confidence >= 0.0);
    assert(d->probabilities[dirIndex(Direction::Left)] > 0.6);

    std::string illegal =
        R"({"model":"jevos-v5","answers":{"move":{"type":"choice","choice":"down","probabilities":{},"confidence":0.1}}})";
    assert(!parseDecision(illegal, legal).has_value());

    assert(!parseDecision("not json", legal).has_value());
  }

  // Fallback: no server listening -> a legal move, flagged as fallback.
  {
    DecisionModelPolicy p("127.0.0.1", 39999);  // nothing on this port
    Board b(std::array<int, 16>{2, 4, 2, 4, 4, 2, 4, 2, 2, 4, 2, 4, 4, 2, 4, 0});
    auto legal = b.legalMoves();
    Decision d = p.chooseMove(b);
    assert(std::find(legal.begin(), legal.end(), d.move) != legal.end());
    assert(d.fallback);
  }

  std::cout << "all decision-model tests passed\n";
  return 0;
}
