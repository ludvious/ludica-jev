# ludica-jev — architecture map

Start here when you want to read the code. This file is a map, not a tutorial:
each heading is one concept, and it points you at the exact file (and where
reasonable, the symbol) to open. Keep it open in one window, code in the other.

For the *why* behind the project (game choice, model choice, roadmap), see
`DECISIONS.md`.

## The one idea that shapes everything

> **Code owns the rules. A policy only picks a move.**

The board, the moves, the merges, the random spawns, win/lose — all of that is
plain C++ in `src/board.cpp`. An AI ("policy") never touches any of it. It only
*reads* a board and returns a `Direction`. The game loop applies the move and
spawns the tile.

This is the same shape every serious game-AI integration uses, including the
decision-model (Jev/Laya) work we'll add later: enumerate the legal moves in
code, let the AI choose among them, validate and apply in code. If the AI says
something illegal, it cannot corrupt the game — there is no path to do so.

## Directory layout

```
ludica-jev/
  CMakeLists.txt          build definition (raylib fetched automatically)
  README.md               build / run / test instructions
  assets/
    Poppins.ttf           bundled UI font (OFL)
  src/
    main.cpp              raylib window, game loop, rendering, decision panel
    board.h / board.cpp   the 2048 rules (pure logic, no rendering)
    policy.h              the MovePolicy interface
    expectimax.h / .cpp   search ceiling: expectimax over the move/spawn tree
    decision_model.h/.cpp decision model over HTTP (jevos), with a fallback
  tests/
    test_board.cpp        asserts on merge/slide/game-over rules
    test_expectimax.cpp   asserts the search returns legal, deterministic moves
    test_decision_model.cpp asserts serialization, parsing, and the fallback
  bench/
    bench.cpp             headless score comparison of the policies
  docs/
    ARCHITECTURE.md       this file (how the code is arranged)
    DECISIONS.md          the why: choices, models, roadmap
    FINDINGS.md           measured results: the gap, the prompt experiments
```

## Concept map (read in this order)

1. **The board rules** — `src/board.h`, `src/board.cpp`.
   - `slideLeft` is the whole move algorithm. Everything else (right/up/down)
     is a rotation of the board so the same code applies. `getLine`/`setLine`
     in `board.cpp` do that rotation.
   - `legalMoves` is the "enumerate legal actions" step every policy relies on.
   - `spawnRandom` is the only place the game introduces randomness.
2. **The policy seam** — `src/policy.h`.
   - One abstract method, `chooseMove`. That tiny interface is why we can swap
     the search and decision-model policies without touching the game loop.
3. **The search ceiling** — `src/expectimax.h`, `src/expectimax.cpp`.
   - `evaluate` scores a leaf board (empty cells, smoothness, monotonicity,
     corner bonus). `bestMove` picks the best move; `searchMax` (move = max)
     and `expectedValue` (spawn = average) alternate — that alternation is what
     makes it *expecti*max rather than minimax.
   - Note the `shortcut:` comment in `expectedValue`: it only spawns 2s.
4. **The decision-model sidecar** — `src/decision_model.h`, `src/decision_model.cpp`.
   - `boardToState` describes the board; `describeMove` computes what each legal
     swipe would do (points, empty cells, mergeable pairs, corner status);
     `buildMoveRequest` turns those into the `/v1/systemone` `choice` criteria;
     `parseDecision` maps the answer back and rejects anything not offered.
     `chooseMove` falls back to the first legal move if the server is down or
     the reply is unusable.
5. **The game** — `src/main.cpp`.
   - Auto-plays one move every `kMoveInterval` seconds. Space pauses, R
     restarts, ESC quits. The first argument picks the policy:
     `./build/ludica-jev decision` runs the decision model, anything else expectimax.
   - `drawPanel` shows the last `Decision`: move, confidence, per-move
     probability bars, latency, and a fallback flag. Fonts load from
     `assets/Poppins.ttf` and fall back to the raylib default if missing.
6. **The rules' safety net** — `tests/test_board.cpp`, `tests/test_expectimax.cpp`,
   `tests/test_decision_model.cpp`.

## How the policies will compare (roadmap)

| Policy | Status | What it is |
| --- | --- | --- |
| `expectimax` | done | search ceiling; plans ahead over random spawns |
| `decision_model` | done | jevos over HTTP, `/v1/systemone`, with fallback |

The deliverable is the score gap between `expectimax` (how well a good AI can
do) and `decision_model` (how far a Jev-style model gets on a numeric puzzle).

## Build & run

```
cmake -B build
cmake --build build
./build/ludica-jev            # expectimax plays
./build/ludica-jev decision   # the decision model plays (needs `jev serve` running)
ctest --test-dir build    # run all tests
```

The first configure downloads and builds raylib and the two header-only libs
(nlohmann/json, cpp-httplib); that is the only network step.
