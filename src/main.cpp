#include <cstdio>
#include <memory>
#include <string>

#include "raylib.h"

#include "board.h"
#include "decision_model.h"
#include "expectimax.h"

using namespace ludica;

namespace {

constexpr int kWindowWidth = 720;
constexpr int kWindowHeight = 580;

constexpr int kCell = 100;
constexpr int kGap = 10;
constexpr int kOriginX = 24;
constexpr int kOriginY = 96;

constexpr int kPanelX = 480;
constexpr int kPanelY = 96;
constexpr int kPanelW = 216;

constexpr float kMoveInterval = 0.15f;  // seconds between auto-moves

const Color kBg{250, 248, 239, 255};
const Color kText{119, 110, 101, 255};
const Color kAccent{246, 94, 59, 255};
const Color kBarBg{235, 228, 218, 255};

Color tileColor(int value) {
  switch (value) {
    case 0:    return {205, 193, 180, 255};
    case 2:    return {238, 228, 218, 255};
    case 4:    return {237, 224, 200, 255};
    case 8:    return {242, 177, 121, 255};
    case 16:   return {245, 149,  99, 255};
    case 32:   return {246, 124,  95, 255};
    case 64:   return {246,  94,  59, 255};
    case 128:  return {237, 207, 114, 255};
    case 256:  return {237, 204,  97, 255};
    case 512:  return {237, 200,  80, 255};
    case 1024: return {237, 197,  63, 255};
    case 2048: return {237, 194,  46, 255};
    default:   return {60, 58, 50, 255};
  }
}

// Draw a tile number, shrinking the font until it fits the cell.
void drawTileNumber(Font font, int v, int cx, int cy, int size, Color c) {
  std::string s = std::to_string(v);
  while (size > 16 && MeasureTextEx(font, s.c_str(), size, 1).x > kCell - 14) size -= 2;
  Vector2 m = MeasureTextEx(font, s.c_str(), size, 1);
  DrawTextEx(font, s.c_str(), {(float)cx - m.x / 2, (float)cy - m.y / 2}, size, 1, c);
}

void drawPanel(Font font, const Decision& last, const char* policyName) {
  //DrawTextEx(font, "AI MOVES", {kPanelX, kPanelY}, 14, 1, kText);
  // Show the model that actually answered, not the request alias.
  std::string shown = last.model.empty() ? std::string(policyName)
                                         : "decision-model(" + last.model + ")";
  DrawTextEx(font, shown.c_str(), {kPanelX, kPanelY + 18}, 18, 1, kText);

  int y = kPanelY + 44;
  std::string moveLine = "move: " + std::string(directionName(last.move));
  DrawTextEx(font, moveLine.c_str(), {(float)kPanelX, (float)y}, 16, 1, kText);
  y += 22;

  char buf[64];
  if (last.fallback) {
    DrawTextEx(font, "fallback (no server)", {(float)kPanelX, (float)y}, 16, 1, RED);
    return;
  }

  bool hasInfo = last.confidence >= 0.0;
  if (!hasInfo) {
    DrawTextEx(font, "search policy - no model info", {(float)kPanelX, (float)y}, 15, 1, kText);
    return;
  }

  snprintf(buf, sizeof buf, "conf: %.2f", last.confidence);
  DrawTextEx(font, buf, {(float)kPanelX, (float)y}, 16, 1, kText);
  y += 22;
  snprintf(buf, sizeof buf, "latency: %.0f ms", last.latencyMs);
  DrawTextEx(font, buf, {(float)kPanelX, (float)y}, 16, 1, kText);
  y += 26;

  const Direction dirs[4] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
  int barY = y;
  for (int i = 0; i < 4; ++i) {
    Direction d = dirs[i];
    double p = last.probabilities[dirIndex(d)];
    bool chosen = (d == last.move);
    Color c = chosen ? kAccent : kText;
    DrawTextEx(font, directionName(d), {(float)kPanelX, (float)barY}, 15, 1, c);
    int bx = kPanelX + 48, bw = kPanelW - 48 - 38;
    DrawRectangle(bx, barY + 4, bw, 9, kBarBg);
    if (p > 0.0) DrawRectangle(bx, barY + 4, (int)(bw * p), 9, c);
    snprintf(buf, sizeof buf, "%.0f%%", p * 100.0);
    DrawTextEx(font, buf, {(float)(bx + bw + 6), (float)barY}, 14, 1, c);
    barY += 30;
  }
}

void drawGame(Font font, const Board& board, const Decision& last,
              const char* policyName, int best, int moves,
              bool gameOver, bool paused) {
  BeginDrawing();
  ClearBackground(kBg);

  DrawTextEx(font, "2048-jev", {24, 10}, 30, 1, kText);
  std::string stats = "score " + std::to_string(board.score()) +
                      "   best " + std::to_string(best) +
                      "   max " + std::to_string(board.maxTile()) +
                      "   moves " + std::to_string(moves);
  DrawTextEx(font, stats.c_str(), {24, 70}, 16, 1, kText);

  for (int r = 0; r < Board::kSize; ++r)
    for (int c = 0; c < Board::kSize; ++c) {
      int v = board.at(r, c);
      int x = kOriginX + c * (kCell + kGap);
      int y = kOriginY + r * (kCell + kGap);
      DrawRectangleRounded({(float)x, (float)y, (float)kCell, (float)kCell}, 0.08f, 0, tileColor(v));
      if (v) {
        int size = v >= 1000 ? 30 : v >= 100 ? 38 : 46;
        drawTileNumber(font, v, x + kCell / 2, y + kCell / 2, size, v >= 8 ? WHITE : kText);
      }
    }

  drawPanel(font, last, policyName);

  std::string footer;
  Color fc = LIGHTGRAY;
  if (gameOver) { footer = "GAME OVER - R to restart"; fc = RED; }
  else if (paused) { footer = "PAUSED - Space to resume"; fc = kAccent; }
  else { footer = "Space pause | R restart | ESC quit"; }
  DrawTextEx(font, footer.c_str(), {kOriginX, kOriginY + 4 * (kCell + kGap) + 16}, 18, 1, fc);

  EndDrawing();
}

}  // namespace

int main(int argc, char** argv) {
  InitWindow(kWindowWidth, kWindowHeight, "2048 AI");
  SetTargetFPS(60);

  // Bundled font; fall back to raylib's default if the file is missing.
  Font font = LoadFontEx("assets/Poppins.ttf", 64, nullptr, 0);
  bool customFont = font.texture.id != 0;
  if (!customFont) font = GetFontDefault();

  // Policy by first argument: "decision" uses the local decision model (needs
  // `jev serve` on 127.0.0.1:8017); anything else uses expectimax.
  std::unique_ptr<MovePolicy> policy;
  if (argc > 1 && std::string(argv[1]) == "decision")
    policy = std::make_unique<DecisionModelPolicy>();
  else
    policy = std::make_unique<ExpectimaxPolicy>();

  Board board;
  board.reset();
  Decision last;
  bool gameOver = false;
  bool paused = false;
  int best = 0;
  int moves = 0;
  double lastMove = GetTime();

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) { paused = !paused; lastMove = GetTime(); }
    if (IsKeyPressed(KEY_R)) {
      board.reset();
      gameOver = false;
      moves = 0;
      lastMove = GetTime();
    }

    if (!paused && !gameOver && GetTime() - lastMove >= kMoveInterval) {
      last = policy->chooseMove(board);
      board.move(last.move);
      board.spawnRandom();
      ++moves;
      if (board.score() > best) best = board.score();
      lastMove = GetTime();
      gameOver = board.isGameOver();
    }

    drawGame(font, board, last, policy->name().c_str(), best, moves, gameOver, paused);
  }

  if (customFont) UnloadFont(font);
  CloseWindow();
  return 0;
}
