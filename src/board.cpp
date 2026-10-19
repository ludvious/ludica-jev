#include "board.h"

#include <algorithm>

namespace ludica {

Board::Board() : rng_(std::random_device{}()) {}

Board::Board(std::array<int, kCellCount> cells) : cells_(cells), rng_(std::random_device{}()) {}

void Board::reset() {
  cells_.fill(0);
  score_ = 0;
  spawnRandom();
  spawnRandom();
}

std::array<int, Board::kSize> Board::getLine(Direction d, int i) const {
  std::array<int, kSize> line{};
  for (int k = 0; k < kSize; ++k) {
    switch (d) {
      case Direction::Left:  line[k] = cells_[i * kSize + k]; break;
      case Direction::Right: line[k] = cells_[i * kSize + (kSize - 1 - k)]; break;
      case Direction::Up:    line[k] = cells_[k * kSize + i]; break;
      case Direction::Down:  line[k] = cells_[(kSize - 1 - k) * kSize + i]; break;
    }
  }
  return line;
}

void Board::setLine(Direction d, int i, const std::array<int, kSize>& line) {
  for (int k = 0; k < kSize; ++k) {
    switch (d) {
      case Direction::Left:  cells_[i * kSize + k] = line[k]; break;
      case Direction::Right: cells_[i * kSize + (kSize - 1 - k)] = line[k]; break;
      case Direction::Up:    cells_[k * kSize + i] = line[k]; break;
      case Direction::Down:  cells_[(kSize - 1 - k) * kSize + i] = line[k]; break;
    }
  }
}

namespace {

// Slide one line toward index 0: merge adjacent equal tiles once, in order.
// Returns the slid line and adds any merged values to `gained`.
//   [2,2,2,2] -> [4,4,0,0]   (two merges, never a triple)
//   [2,2,4,0] -> [4,4,0,0]
std::array<int, Board::kSize> slideLeft(const std::array<int, Board::kSize>& in, int& gained) {
  std::array<int, Board::kSize> out{};
  int write = 0;
  for (int read = 0; read < Board::kSize; ++read) {
    if (in[read] == 0) continue;
    if (out[write] == 0) {
      out[write] = in[read];
    } else if (out[write] == in[read]) {
      out[write] *= 2;
      gained += out[write];
      ++write;
    } else {
      ++write;
      out[write] = in[read];
    }
  }
  return out;
}

// Would this line change if slid toward index 0? True when a nonzero tile has
// a zero before it (it would shift) or an equal neighbor (it would merge).
bool lineCanSlide(const std::array<int, Board::kSize>& line) {
  bool sawZero = false;
  for (int i = 0; i < Board::kSize; ++i) {
    if (line[i] == 0) {
      sawZero = true;
    } else {
      if (sawZero) return true;
      if (i + 1 < Board::kSize && line[i] == line[i + 1]) return true;
    }
  }
  return false;
}

}  // namespace

bool Board::canMove(Direction d) const {
  for (int i = 0; i < kSize; ++i)
    if (lineCanSlide(getLine(d, i))) return true;
  return false;
}

std::vector<Direction> Board::legalMoves() const {
  std::vector<Direction> out;
  for (Direction d : {Direction::Up, Direction::Down, Direction::Left, Direction::Right})
    if (canMove(d)) out.push_back(d);
  return out;
}

bool Board::move(Direction d) {
  bool changed = false;
  int gained = 0;
  for (int i = 0; i < kSize; ++i) {
    auto line = getLine(d, i);
    auto slid = slideLeft(line, gained);
    if (slid != line) changed = true;
    setLine(d, i, slid);
  }
  if (changed) score_ += gained;
  return changed;
}

std::vector<int> Board::emptyCells() const {
  std::vector<int> out;
  for (int i = 0; i < kCellCount; ++i)
    if (cells_[i] == 0) out.push_back(i);
  return out;
}

void Board::spawnAt(int index, int value) {
  if (cells_[index] != 0) return;  // guard: only fill an empty slot
  cells_[index] = value;
}

void Board::spawnRandom() {
  auto empty = emptyCells();
  if (empty.empty()) return;

  std::uniform_int_distribution<int> pick(0, static_cast<int>(empty.size()) - 1);
  std::uniform_int_distribution<int> value(0, 9);  // 4 in ~10% of spawns, else 2
  cells_[empty[pick(rng_)]] = (value(rng_) == 0) ? 4 : 2;
}

int Board::maxTile() const {
  return *std::max_element(cells_.begin(), cells_.end());
}

}  // namespace ludica
