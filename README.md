# Efficient computational verifier for the 3x−1 map

This repository provides `verify2.c`, a computational verifier designed to check
large ranges of the positive 3x−1 map efficiently. It uses affine class bounds and
reverse-path pruning to avoid tracing every starting value individually. The
author's production runs with this verifier collectively covered every
1 ≤ n < 2^50: a full run over 1 ≤ n < 2^48 followed by an extension run over
2^48 ≤ n < 2^50. This extends the 2^44 range reported by Cochin (2026).

## Background

For the 3x−1 map, odd n advances via C(n) = (3n−1)/2 and even n is halved. The
conjecture is that every positive integer eventually reaches one of the three
known cycles {1}, {5, 7, ...}, {17, ...}. The conjecture itself remains open.
Cochin, P. (2026), "Lower Bounds for Cycle Lengths in the Juggler Map"
(Zenodo, DOI: 10.5281/zenodo.22865237, v1.0.2), reports in Remark 5.20 (p. 64)
the author's own verification that every 1 ≤ n < 2^44 reaches 1, 5 or 17. The paper
states that its author found no published verification floor for the 3x−1 map. This
repository reports an independent extension to all 1 ≤ n < 2^50. A broader
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

## v0.3.0: optional GPU backend

An optional AMD HIP backend keeps the class tree on the CPU and sends leaf
descriptors to the GPU for expansion, trajectory tracing and reduction. The
original CPU verifier remains available. Build instructions, resource requirements
and tested hardware are in [gpu/README.md](gpu/README.md).

The GPU version independently repeated `[2^48, 2^50)` with 8 CPU cores / 16
threads and an RX 9070 XT: exit 0, 161/161 chunks, fails=0, and exactly
422,212,465,065,984 odd starts accounted for. Wall time including initialization
was 2 h 39 min 20.908 s. In a separate interleaved steady benchmark over
`[2^48-2^40, 2^48)`, median CPU/GPU times were 19.856 / 12.438 s (1.596x).
This ratio applies to that partial interval; a same-conditions whole-range
CPU/GPU ratio was not measured. Full evidence is in
[results/v0.3.0](results/v0.3.0/README.md).

This release adds a GPU implementation and a repeated verification of the
existing upper interval. It does not extend the demonstrated range to 2^52.

## Demonstrated verification range

| N | Status | Notes |
|---|---|---|
| 2^44 | done | independently reproduces Cochin (2026) |
| 2^46 | done | verified with `verify2.c` in this repo |
| 2^48 | done | AMD full run with `verify2.c`: chunks=2563/2563, maxsteps=734 at n=105343905708261 ([log](results/amd-ryzen7-5700x-2p48.txt)) |
| 2^50 | done | AMD extension run over [2^48, 2^50): chunks=2563/2563, fails=0, maxsteps=838 at n=588127695714561 ([log](results/amd-ryzen7-5700x-2p48-to-2p50.txt)) |

The full 2^48 run and the extension run together account for all
562,949,953,421,312 odd starting values below 2^50. Run parameters and SHA-256
hashes are recorded in [the verification manifest](results/amd-verification.json).
No counterexample (an n that fails to reach any of the three cycles) was found
in this finite range.

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

The code, documentation, and result files in this repository are licensed under
the [MIT License](LICENSE).

## Acknowledgments

Development, code review, and verification tooling were carried out with the
assistance of Claude (Anthropic; Claude Fable 5.1, Claude Opus 5.5, and Claude
Sonnet 5, across different stages of the work). Documentation, public release
preparation, and archive checks were assisted by Codex (OpenAI). The author
directed the methodology, reviewed all results, and is solely responsible for
the content.

## Citation

The archived v0.2.0 release has DOI
[10.5281/zenodo.22968237](https://doi.org/10.5281/zenodo.22968237).
To cite the archived v0.2.0 version:

> Shimizu, H. (2026). *Efficient computational verifier for the 3x-1 map*
> (Version 0.2.0) [Computer software]. Zenodo.
> https://doi.org/10.5281/zenodo.22968237

For v0.3.0, use the version metadata in `CITATION.cff` and the GitHub release.
A v0.3.0 Zenodo DOI has not yet been assigned.

Source code: https://github.com/yygotm/3x-1-verification

Previous release (v0.1.0): https://doi.org/10.5281/zenodo.22962037
