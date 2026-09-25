# Efficient computational verifier for the 3x−1 map

This repository provides `verify2.c`, a computational verifier designed to check
large ranges of the positive 3x−1 map efficiently. It uses affine class bounds and
reverse-path pruning to avoid tracing every starting value individually. The
author's production run with this verifier covered every 1 ≤ n < 2^48; that range
demonstrates the tool's use and extends the 2^44 range reported by Cochin (2026).

## Background

For the 3x−1 map, odd n advances via C(n) = (3n−1)/2 and even n is halved. The
conjecture is that every positive integer eventually reaches one of the three
known cycles {1}, {5, 7, ...}, {17, ...}. The conjecture itself remains open.
Cochin, P. (2026), "Lower Bounds for Cycle Lengths in the Juggler Map"
(Zenodo, DOI: 10.5281/zenodo.22865237, v1.0.2), reports in Remark 5.20 (p. 64)
the author's own verification that every 1 ≤ n < 2^44 reaches 1, 5 or 17. The paper
states that its author found no published verification floor for the 3x−1 map. This
repository reports an independent extension to all 1 ≤ n < 2^48. A broader
prior-work search is in progress.

## Verifier design and performance

- Odd n is advanced via the accelerated map C(x) = (3x−1)/2 until it drops below n
  (strong induction). Only the cycle minima 1, 5, 17 are handled as a special case.
- `verify.c` (sieve-based, mod-2^K residue sieve to skip classes) was used to verify
  up to 2^44.
- `verify2.c` (faster version): odd numbers are split into classes on a binary tree.
  The relevant bounds are affine (degree-1) functions of the position within a
  class, so checking both interval endpoints decides the whole class.
  A 3-adic reverse-path technique (P) is combined in for early pruning.
  In a historical full (non-strided) run over [2^40, 2^42), it was 36.0x faster
  than `verify.c` by elapsed time; comparing its full-range rate with the old
  run's fastest chunk gives 27.4x ([benchmark data](results/intel-benchmark-2p40-2p42.json)).
  This benchmark used a pre-release build of `verify2.c`, and background CPU
  load was not controlled between runs.

## Demonstrated verification range

| N | Status | Notes |
|---|---|---|
| 2^44 | done | independently reproduces Cochin (2026) |
| 2^46 | done | verified with `verify2.c` in this repo |
| 2^48 | done | AMD full run with `verify2.c`: chunks=2563/2563, maxsteps=734 at n=105343905708261 ([log](results/amd-ryzen7-5700x-2p48.txt)) |

No counterexample (an n that fails to reach any of the three cycles) has been found.

## Correctness checks

- Self-check mode (mode 2): every member of every dropped class is independently
  recomputed by brute-force forward iteration; zero mismatches across the ranges
  tested.
- Mutation testing: key branches of the decision logic (checking only one endpoint
  instead of both, dropping the cycle-minimum exclusion, skipping the integrality
  check on the reverse path, etc.) were deliberately broken to confirm the test
  suite detects genuine bugs, while confirming that the cycle-minimum exclusion
  itself is redundant (removing it changes nothing), backed by both targeted test
  cases and full `mode 2` comparisons.
- Implementation bugs found in external code review (missing input validation, an
  unchecked accounting step on the checkpointed production path, unchecked
  allocation failures, etc.) were fixed before the production runs.

## Build & run

```sh
docker build -t 3xm1-verify:local .
docker run --rm 3xm1-verify:local <LO> <HI> <mode> <F> <L> <LEAFN>

gcc -O3 -march=native -fopenmp -Wall -o verify2 verify2.c -lm
./verify2 <LO> <HI> <mode> <F> <L> <LEAFN> [chunk logfile]
```

The Docker build context contains only `verify2.c` and `Dockerfile`; local logs
and review notes are excluded by `.dockerignore`. Use the native build when a
checkpoint file is needed.

- `mode`: 0 = normal run, 1 = strict mode (no (P), leaves count stopping time by
  brute force), 2 = self-check
- Passing `chunk`/`logfile` enables a checkpointed run that can be interrupted and
  resumed.

Regression tests: `t2.sh` (single-run consistency), `t3.sh` (checkpoint
consistency), `test_cm.c` / `mut.sh` (cycle-minimum handling and mutation testing
of the decision logic).

## License

Unless otherwise noted, the code, documentation, and result files in this repository
are licensed under the MIT License (see `LICENSE`).

## Acknowledgments

Development, code review, and verification tooling were carried out with the
assistance of Claude (Anthropic; Claude Fable 5.1, Claude Opus 5.5, and Claude
Sonnet 5, across different stages of the work). The author directed the
methodology, reviewed all results, and is solely responsible for the content.

## Citation

See `CITATION.cff`. In brief:

> Shimizu, H. (2026). *Efficient computational verifier for the 3x-1 map*
> (Version 0.1.0) [Software]. Source code: https://github.com/yygotm/3x-1-verification
