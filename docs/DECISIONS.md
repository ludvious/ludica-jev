# ludica-jev — decisions log

The *why* behind the project, as agreed in the kickoff meeting. For *how the
code is arranged*, see `ARCHITECTURE.md`.

## Mission

Apply the new "decision models" (Jev, Laya, jevos) to games, and get a
measurable answer: how far does a text-based decision model get on a game that
a real AI (search) can play well? The deliverable is that gap, not a game that
sells.

## The game: 2048

- Turn-based, CPU-only, 2D, **the AI plays the player** (auto-player).
- Chosen as a training vehicle: small enough to build end-to-end, and it makes
  the gap *visible* — 2048 needs long-horizon numeric planning, which is
  exactly where a text decision model is weakest. Snake/Battleship/Tetris are
  progressively *easier* for a decision model, so they'd blur the result.

## The models

| | Jev (TypeSafe) | jevos (feder-cr) | Laya (Convai) |
| --- | --- | --- | --- |
| Offline | no (hosted) | yes, CPU | yes, GPU/CPU |
| Cost | per-token API | free, MIT | free, Apache-2.0 |
| Latency | 70–500 ms | 28–130 ms | 5–40 ms GPU / 200–600 ms CPU |
| Zero-shot | highest | middle | near-random on broad tasks |
| Fine-tunable | no | no | yes (RLCD) |

**Decision:** start with **jevos over an HTTP sidecar**, speaking
`/v1/systemone` (Jev's wire format). Rationale: it runs local/CPU/MIT, and the
wire format is shared by jevos, Laya, and hosted Jev — so swapping models is a
URL change, not a code change. `llama.cpp` in-process embedding was rejected:
more code to read, and its only benefit (single binary) doesn't matter for a
turn-based game. Fine-tuning Laya is parked (needs GPU for *training* only;
inference stays CPU).

## Architecture principle

> **Code owns the rules. A policy only picks a move.**

Enumerate legal moves in code → policy returns one → code validates and applies
it. The model *selects*, it never *generates*, so a bad AI answer can't corrupt
the game. This is the same pattern serious game-AI integrations use.

## Policies

| Policy | Role | Status |
| --- | --- | --- |
| `expectimax` | the ceiling (search) | done |
| `decision_model` | the mission | done (sidecar + fallback) |
| RL (Q-learning) | deferred | can slot in later behind the same seam |

Two policies (search ceiling + decision model); RL was dropped deliberately and
stays a later option with zero refactor.

## Decision-model uplift ladder (how we'll help the model)

Ordered by effort, cheapest first. Steps 1–3 are implemented in
`src/decision_model.cpp`; step 5's "code computes, model selects" idea is too,
via `describeMove` (each move's consequence goes into its criteria description).
Steps 4 and 6 remain:

1. **State serialization** — feed strategy language, not raw numbers.
2. **Rule-in-the-question** — put the 2048 strategy rule in the instructions.
3. **Legal moves only** — never offer a no-op direction.
4. **Confidence cascade** — hand low-confidence moves to search/heuristics.
5. **Decision model as evaluator** (stretch) — search runs the lookahead, the
   model rates the leaves.
6. **Fine-tune Laya** (parked) — needs GPU training; wait for replay data.

## Docs convention

Inline comments explain *why* (not what); `docs/` holds navigable maps so the
code can be read top-down by jumping to the symbol of interest.

**Standing rule:** docs are updated in the *same change* as the code they
describe — a task, bug fix, feature, or refactor is not "done" until
`DECISIONS.md` and/or `ARCHITECTURE.md` reflect it. Docs drift is treated as
incomplete work, never as a follow-up.

## Status

| Item | Status |
| --- | --- |
| Board rules + tests | done |
| `MovePolicy` interface + raylib loop | done |
| `expectimax` + `decision_model` policies | done |
| UI: font, pause, decision panel | done |
| Benchmark runner | done |
| Gap comparison (deliverable) | done — see `docs/FINDINGS.md` |
