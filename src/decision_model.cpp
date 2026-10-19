#include "decision_model.h"

#include <algorithm>
#include <chrono>
#include <exception>

#include <httplib.h>

namespace ludica {

namespace {

// The decision rule, folded into the question. jevos is a decision model: it
// scores the options we hand it, it does not generate text. The per-move
// consequences are computed in code (see describeMove) and placed in the
// criteria descriptions, so the model ranks facts instead of simulating the
// board — the "do sums in code" principle from the jevos wiki.
constexpr const char* kStrategyRule =
    "Each option describes what that swipe would do. Choose the swipe that "
    "keeps the largest tile cornered; among those, prefer more points, more "
    "empty cells, and more mergeable pairs.";

// What one legal swipe would do, computed in code so the model only ranks
// facts instead of simulating the board. The numbers are: merged tile value
// (points), empty cells after the move, adjacent equal pairs (future merges),
// and whether the largest tile is still in a corner.
std::string describeMove(const Board& board, Direction d) {
  Board next = board;
  next.move(d);
  int gained = next.score() - board.score();
  int empties = static_cast<int>(next.emptyCells().size());
  int maxTile = next.maxTile();
  int n = Board::kSize - 1;
  bool cornered = next.at(0, 0) == maxTile || next.at(0, n) == maxTile ||
                  next.at(n, 0) == maxTile || next.at(n, n) == maxTile;
  int pairs = 0;
  for (int r = 0; r < Board::kSize; ++r)
    for (int c = 0; c < Board::kSize; ++c) {
      int v = next.at(r, c);
      if (v == 0) continue;
      if (c + 1 < Board::kSize && next.at(r, c + 1) == v) ++pairs;
      if (r + 1 < Board::kSize && next.at(r + 1, c) == v) ++pairs;
    }
  return "+" + std::to_string(gained) + " points, " + std::to_string(empties) +
         " empty cells, " + std::to_string(pairs) + " mergeable pairs, largest tile " +
         (cornered ? "cornered" : "not cornered");
}

std::optional<Direction> directionFromString(const std::string& s) {
  if (s == "up") return Direction::Up;
  if (s == "down") return Direction::Down;
  if (s == "left") return Direction::Left;
  if (s == "right") return Direction::Right;
  return std::nullopt;
}

}  // namespace

const char* directionName(Direction d) {
  switch (d) {
    case Direction::Up:    return "up";
    case Direction::Down:  return "down";
    case Direction::Left:  return "left";
    case Direction::Right: return "right";
  }
  return "?";
}

std::string boardToState(const Board& b) {
  std::string s = "2048 board, rows top to bottom (0 = empty):\n";
  for (int r = 0; r < Board::kSize; ++r) {
    for (int c = 0; c < Board::kSize; ++c) {
      if (c) s += ' ';
      s += std::to_string(b.at(r, c));
    }
    s += '\n';
  }
  int maxTile = b.maxTile();
  int n = Board::kSize - 1;
  bool cornered = b.at(0, 0) == maxTile || b.at(0, n) == maxTile ||
                  b.at(n, 0) == maxTile || b.at(n, n) == maxTile;
  s += "Largest tile: " + std::to_string(maxTile);
  s += cornered ? " (in a corner). " : " (not in a corner). ";
  s += "Empty cells: " + std::to_string(static_cast<int>(b.emptyCells().size())) + ".\n";
  return s;
}

nlohmann::json buildMoveRequest(const std::string& model, const Board& board,
                                const std::vector<Direction>& legal) {
  nlohmann::json criteria = nlohmann::json::object();
  for (Direction d : legal) criteria[directionName(d)] = describeMove(board, d);

  return nlohmann::json{
      {"model", model},
      {"state", boardToState(board)},
      {"questions",
       {{"move",
         {{"type", "choice"},
          {"instructions", kStrategyRule},
          {"criteria", criteria}}}}},
  };
}

std::optional<Decision> parseDecision(const std::string& body,
                                      const std::vector<Direction>& legal) {
  Decision d;
  try {
    auto j = nlohmann::json::parse(body);
    auto ans = j.at("answers").at("move");
    std::string choice = ans.at("choice").get<std::string>();
    auto dir = directionFromString(choice);
    if (!dir) return std::nullopt;
    d.move = *dir;
    d.confidence = ans.value("confidence", -1.0);
    d.model = j.value("model", std::string{});
    if (ans.contains("probabilities")) {
      for (auto& [key, val] : ans["probabilities"].items()) {
        auto pd = directionFromString(key);
        if (pd) d.probabilities[dirIndex(*pd)] = val.get<double>();
      }
    }
  } catch (const std::exception&) {
    return std::nullopt;
  }
  // Only accept a move we actually offered.
  if (std::find(legal.begin(), legal.end(), d.move) == legal.end()) return std::nullopt;
  return d;
}

Decision DecisionModelPolicy::chooseMove(const Board& board) {
  auto legal = board.legalMoves();
  Decision out;
  out.move = legal.front();  // contract: non-empty (caller checks game over)
  out.fallback = true;

  auto t0 = std::chrono::steady_clock::now();
  std::string body = buildMoveRequest(model_, board, legal).dump();

  httplib::Client cli(host_, port_);
  cli.set_connection_timeout(1, 0);
  cli.set_read_timeout(5, 0);
  auto res = cli.Post("/v1/systemone", body, "application/json");
  out.latencyMs =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

  if (!res || res->status != 200) return out;

  auto parsed = parseDecision(res->body, legal);
  if (!parsed) return out;

  parsed->latencyMs = out.latencyMs;
  parsed->fallback = false;
  return *parsed;
}

std::string DecisionModelPolicy::name() const {
  return "decision-model(" + model_ + ")";
}

}  // namespace ludica
