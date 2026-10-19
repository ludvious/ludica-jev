#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "../src/board.h"
#include "../src/decision_model.h"
#include "../src/expectimax.h"

using namespace ludica;

namespace {

struct GameStats {
  std::vector<long long> scores;  // final score per game
  std::vector<int> tiles;         // max tile reached per game
  int wins = 0;                   // reached >= 2048
  long long moveSum = 0;
  double latencySum = 0;          // ms, decision model only
  int fallbacks = 0;
  double confidenceSum = 0;
  int decisions = 0;              // chooseMove calls with diagnostics
};

GameStats run(MovePolicy& policy, int games, int maxMoves) {
  GameStats s;
  for (int g = 0; g < games; ++g) {
    Board b;
    b.reset();
    int moves = 0;
    int best = 0;
    while (!b.isGameOver() && moves < maxMoves) {
      Decision d = policy.chooseMove(b);
      s.latencySum += d.latencyMs;
      if (d.fallback) ++s.fallbacks;
      if (d.confidence >= 0.0) { ++s.decisions; s.confidenceSum += d.confidence; }
      b.move(d.move);
      b.spawnRandom();
      best = std::max(best, b.maxTile());
      ++moves;
    }
    s.scores.push_back(b.score());
    s.tiles.push_back(best);
    s.moveSum += moves;
    if (best >= 2048) ++s.wins;
  }
  return s;
}

void print(const char* label, const GameStats& s) {
  int n = (int)s.scores.size();
  if (n == 0) { std::cout << label << ": no games\n"; return; }

  std::vector<long long> sorted = s.scores;
  std::sort(sorted.begin(), sorted.end());
  long long mean = std::accumulate(s.scores.begin(), s.scores.end(), 0LL) / n;
  long long median = sorted[n / 2];

  std::cout << label << "  (" << n << " games)\n";
  std::cout << "  score: mean " << mean << "  median " << median
            << "  min " << sorted.front() << "  max " << sorted.back() << "\n";
  std::cout << "  best tile " << *std::max_element(s.tiles.begin(), s.tiles.end())
            << "  wins " << s.wins << "/" << n
            << "  avg moves " << (s.moveSum / n) << "\n";

  std::cout << "  final tile:";
  const int buckets[] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
  for (int t : buckets)
    std::cout << " " << t << "x" << std::count(s.tiles.begin(), s.tiles.end(), t);
  int big = (int)std::count_if(s.tiles.begin(), s.tiles.end(), [](int t) { return t >= 2048; });
  std::cout << " 2048+x" << big << "\n";

  if (s.decisions > 0) {
    std::cout << "  avg latency " << (s.latencySum / s.decisions) << " ms"
              << "  avg confidence " << (s.confidenceSum / s.decisions)
              << "  fallbacks " << s.fallbacks << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string policy = argc > 1 ? argv[1] : "both";
  int games = argc > 2 ? std::atoi(argv[2]) : 10;
  int depth = argc > 3 ? std::atoi(argv[3]) : 3;
  const int kMaxMoves = 10000;

  std::cout << "ludica-jev benchmark: " << games << " games, policy=" << policy
            << ", expectimax depth " << depth << "\n\n";

  if (policy == "both" || policy == "expectimax") {
    ExpectimaxPolicy p(depth);
    print("expectimax", run(p, games, kMaxMoves));
    std::cout << "\n";
  }
  if (policy == "both" || policy == "decision") {
    DecisionModelPolicy p;
    print("decision-model (needs `jev serve`)", run(p, games, kMaxMoves));
  }
  return 0;
}
