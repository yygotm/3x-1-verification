# v0.3.0 measurement and validation evidence

## Production GPU verification

On 2026-09-27, `[2^48, 2^50)` was run once in mode 0, F=26, L=10,
LEAFN=16, without striding or GPU audit. CPU: Ryzen 7 5700X, 8 cores / 16
OpenMP threads. GPU: RX 9070 XT (`gfx1201`), official ROCm 10.0.0 in WSL.

| Item | Result |
|---|---:|
| Process exit code | 0 |
| Continuous, nonduplicated chunks | 161 / 161 |
| Odd starting values, exactly accounted | 422,212,465,065,984 |
| fails | 0 |
| Wall time, including initialization | 9,560.908480372 s (2 h 39 min 20.908 s) |
| Sum of timed computation sections | 9,557.146301270 s |
| GPU initialization | 1.842948327 s |
| mode-0 maxsteps / argmax | 838 / 588,127,695,714,561 |

`compute_seconds` includes tree generation and leaves; it is not GPU-kernel time.
Mode 0 maxsteps is a certificate bound, not an exact first stopping time. Python
arbitrary-precision forward iteration and an independent inverse path certify
the reported argmax (see `audit.json`). The raw GPU `DESC_SUMMARY` includes
positive descriptor/leaf counts and CPU fallback work.

Unity was closed before the run. The monitored checks did not observe Unity
running during it; all other background activity was not exhaustively excluded.
This is a measurement on a household Windows/WSL system.
No same-conditions CPU run over this entire interval was made for v0.3.0.
Historical v0.2.0 CPU results establish verification coverage, not this release's
full-range CPU/GPU speed ratio. The interval is half-open and does not extend to 2^52.

## Controlled partial-range comparison

Interval `[2^48-2^40, 2^48)`, mode 0, F26/L10/LEAFN16, LC10/JK10,
16 threads, no striding or GPU audit. One full warmup then three interleaved
CPU/GPU repetitions within persistent processes. Statistics match in all eight
runs, fails=0, odd count=549,755,813,888, maxsteps=561.

| Steady computation time | Median | Min - max |
|---|---:|---:|
| CPU comparator (`gpu/baseline.c`) | 19.856478520 s | 18.903013223 - 21.354889195 s |
| GPU front end | 12.438356900 s | 11.754569070 - 12.956733295 s |

Median ratio: **1.596390800x** for this interval. Paired ratios:
1.596390800 / 1.816730930 / 1.458933575. These steady times exclude process/GPU
startup. No cold CPU/GPU comparison or extrapolated whole-range speedup is claimed.

## Recorded correctness checks

The repaired implementation passed 16 Python tests normally and under `-O`,
16 rejected LC/JK configurations, checkpoint ASan/UBSan tests (1 valid and 12
invalid cases), 832 independent backend cases and 127 fallback cases, and 20
rejected API/lifecycle cases. Two mode-2 front ends each actually traced 452,954
GPU members with selfcheck_bad=0. CPU/GPU category counters matched for small,
odd-endpoint, upper-bound, strict and width-2^36 cases. Checkpoint resume and
independent argmax checks passed. The build retains a `-Wmisleading-indentation`
warning for compact `desc_flush` formatting; the original warning is preserved.

## Files and provenance

- `production/`: original raw log, stdout/stderr, status, audit and build hashes.
- `production/source-snapshot/`: all 15 sources present at the production build,
  including historical comparison/profile variants; only the front end and backend
  under `gpu/` are the release runtime. This snapshot is archival evidence.
- `benchmark/`: all repetitions, raw outputs, commands and source/artifact hashes.
- `validation/`: recorded checks and rejection/sanitizer outputs.
- `release-provenance.json`: mapping of packaged runtime files to measured hashes.

Original files are copied byte-for-byte, including their historical local paths.
Object/executable hashes identify the original builds; binaries are not distributed.
New builds need not reproduce those binary hashes. Packaged audit scripts are made
self-contained, and the benchmark runner omits unused experiment generation code.
The GPU C/C++ runtime files are byte-identical to the measured files.

To independently audit the copied production log without modifying it:

```sh
python3 results/v0.3.0/production/audit-production.py
python3 results/v0.3.0/production/test-audit.py
```
