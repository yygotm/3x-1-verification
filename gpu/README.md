# Optional AMD HIP GPU backend (v0.3.0)

The CPU generates and prunes the affine class tree. Eligible leaves are sent as
arithmetic-progression descriptors. The GPU expands each descriptor, traces up
to 16 members, and reduces the counters and maximum. Exceptional trajectories
are returned for CPU handling. A dispatcher and two streams overlap CPU tree
generation with GPU work. `descriptor-large.c` is the measured production front end.
The original root `verify2.c` and CPU Docker image are retained.

## Tested environment

AMD Ryzen 7 5700X (8 physical cores / 16 threads), Radeon RX 9070 XT (`gfx1201`),
Windows/WSL Ubuntu 26.04.1, official ROCm 10.0.0 and ROCDXG 1.2.2.
This is the tested configuration, not a claim of compatibility with every HIP GPU.
The code uses GCC/OpenMP, `unsigned __int128`, HIP and POSIX facilities.
There is no native Windows/MSVC build or GPU Docker image in this release.

## Build and run

Provide `gcc`, `hipcc` and Python 3 in WSL/Linux. On the measured WSL installation:

```sh
export PATH=/opt/rocm/core-10.0/bin:$PATH
export LD_LIBRARY_PATH=/opt/rocm/core-10.0/lib:/opt/rocm/lib:/usr/lib/wsl/lib
bash gpu/build.sh
OMP_NUM_THREADS=16 OMP_DYNAMIC=FALSE gpu/build/descriptor-large 1 65537 0 26 10 16
```

`GPU_ARCH` overrides the build target; architectures other than `gfx1201` have not
been tested. No modified ROCm runtime is needed. The harness uses the installation
paths above; adjust them for another installation.

CLI: `LO HI mode [F L LEAFN [chunk logfile]]`. Bounds are half-open.
The HIP backend requires compile-time `LC=10`, `JK=10`, runtime `L=10`, and
`LEAFN <= 16`. Modes are 0 normal, 1 strict CPU path, 2 self-check.
The mode-2 tests in this release actually trace GPU work and check it independently.
Mode 0 `maxsteps` is a verification certificate bound, not necessarily the exact
first stopping time. Tied argmax values may differ between runs.

The measured production command was:

```sh
OMP_NUM_THREADS=16 OMP_DYNAMIC=FALSE OMP_PROC_BIND=FALSE taskset -c 0-15 \
  gpu/build/descriptor-large 281474976710656 1125899906842624 0 26 10 16 \
  4096 production.log
```

Use a new log for a new run. With the same range, parameters and chunk size,
the checkpoint path can resume completed chunks. Mode 2 and `BENCH_STRIDE` cannot
be combined with checkpointing. Performance runs must omit `BENCH_STRIDE` and
`GPU_AUDIT`; those options change the workload.

Capacity is 131,072 descriptors with two streams. At 16 producers, mode 0,
initialization requests 1,879,049,984 bytes of pinned host memory. Allocation or
HIP errors fail the run. A 4 GiB pinned allocation budget is enforced. Reducing
the OpenMP thread count reduces the producer allocation; this changes performance.

## Reproduce checks and the partial-range benchmark

```sh
cd gpu
python3 test-repairs.py
python3 -O test-repairs.py
python3 final-check.py
# Use the checks-* path printed by final-check.py:
python3 measure-fixed.py /absolute/path/to/checks-directory
```

These commands create new result directories and execute GPU work. `final-check.py`
checks independent backend arithmetic, rejected API calls, mode 2, range boundaries,
strict mode and CPU/GPU statistics. It targets `gfx1201`.
`measure-fixed.py` performs one warmup and three interleaved CPU/GPU repetitions
over `[2^48-2^40, 2^48)`. Its CPU comparator is `baseline.c`, the CPU version of
the measured front end, rather than a differently parameterized historical run.

Recorded evidence is in [results/v0.3.0](../results/v0.3.0/README.md).
The production range `[2^48, 2^50)` was verified once with this backend. No full
GPU verification to `2^52` or full-range CPU/GPU speed ratio is reported.
