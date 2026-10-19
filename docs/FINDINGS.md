# ludica-jev — findings

The measured answer to the project's question.

## Question

How far does a text-based **decision model** (jevos, a local Jev-compatible
model) get on 2048, versus a **search AI** (expectimax) that can actually plan?

## Method

- `./build/bench decision 30` — 30 headless games.
- The decision-model prompt is **code-computed consequences**: `describeMove`
  computes each move's points, empty cells, mergeable pairs, and corner status
  in code and puts them in the `criteria` descriptions, so the model ranks
  facts instead of simulating the board.
- Ceiling: expectimax at depth 3.
- Random baseline: **987 ± 414**, from LMGame-Bench (arXiv 2505.15146).

## Results

| Policy | Avg score | Median | Best tile | Wins | Confidence | Latency |
| --- | --- | --- | --- | --- | --- | --- |
| `expectimax` (d3) | ~30,000 | — | 2048 | consistent | n/a | — |
| `decision_model` | 946 | 916 | 256 | 0 / 30 | 0.41 | ~550 ms |
| random (LMGame-Bench) | 987 ± 414 | — | — | 0 | — | — |

The decision model is **statistically indistinguishable from random play** — a
~30× gap behind the search ceiling. Its final tile peaks at 64–128 and never
reaches 512.

## Conclusion

On a numeric-planning game like 2048, a small decision model plays at random
level. The gap is intrinsic to the model's lack of lookahead and arithmetic,
not to prompt quality: the model reports moderate confidence (0.41) yet still
plays at random, so its confidence is miscalibrated.

More generally: anything we compute to help the model (points, empties, pairs)
is something the code could act on directly with `std::max`. A decision model
only earns its place where the decision depends on **semantic judgment over
unstructured text** (routing, moderation, policy checks) — not on **simulating
a numeric game state**.

## Parked

Fine-tuning Laya on game-specific data (needs GPU training + replay data) is the
only path that could genuinely move the number — and is a different project.
