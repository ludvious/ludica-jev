#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "policy.h"

namespace ludica {

// "up" / "down" / "left" / "right" — how a Direction is spelled on the wire.
const char* directionName(Direction d);

// Serialize a board as strategy-language text for the model (uplift step 1):
// the grid, the largest tile and whether it is cornered, and the empty count.
std::string boardToState(const Board& b);

// Build the /v1/systemone body asking "which swipe next?", with the legal moves
// as the choice criteria. Each criteria description carries that move's
// computed consequence (see describeMove); a no-op direction is never offered.
nlohmann::json buildMoveRequest(const std::string& model, const Board& board,
                                const std::vector<Direction>& legal);

// Parse a /v1/systemone response into a Decision (move, confidence, per-move
// probabilities), accepting the move only if it is one of `legal`. Returns
// nullopt on a malformed body or an illegal choice, so the caller falls back.
std::optional<Decision> parseDecision(const std::string& body,
                                      const std::vector<Direction>& legal);

// Picks moves by asking a local decision model (jevos) over HTTP, using Jev's
// wire format. If the server is down or the reply is unusable, it falls back to
// the first legal move, so the game never freezes or crashes.
class DecisionModelPolicy : public MovePolicy {
public:
  DecisionModelPolicy(std::string host = "127.0.0.1", int port = 8017,
                      std::string model = "jev-latest")
      : host_(std::move(host)), port_(port), model_(std::move(model)) {}

  Decision chooseMove(const Board& board) override;
  std::string name() const override;

private:
  std::string host_;
  int port_;
  std::string model_;
};

}  // namespace ludica
