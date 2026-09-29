# v0.4.0 — Verification extended to Y₀ = 4,524,254,009,935,345

This release adds two CPU verification runs of the unchanged `verify2.c`
(SHA-256 67f6b14ac9538aba5e72a8a7d51856e114da97cbfac8e01f8747c06244818e63),
built in the pinned Docker image of the earlier AMD runs
(sha256:fde54a7328d6517ee95557e353b157b09637ea2a0628ee2ff509779a8114b3df)
on a Ryzen 7 5700X, with mode=0 F=26 L=10 LEAFN=16 chunk=256:

- [2^50, 2^52): exit 0, 2563/2563 chunks, fails=0, 1,688,849,860,263,936 odd
  starting values exactly accounted for, maxsteps=749 at n=2,231,162,091,855,129,
  summed chunk time 30,344.484 s.
- [2^52, Y₀) with Y₀ = 4,524,254,009,935,345: exit 0, 2563/2563 chunks,
  fails=0, 10,327,191,282,424 odd starting values exactly accounted for,
  maxsteps=604 at n=4,512,101,545,839,377, summed chunk time 276.159 s.

Together with the archived runs, the demonstrated range is now
1 <= n < Y₀ (about 1.0046 · 2^52). No verifier source, build procedure or
earlier measurement record is changed. This finite verification does not prove
the 3x-1 conjecture. The CPU+GPU extension of v0.3.x was not used.

- v0.4.0 version DOI: https://doi.org/10.5281/zenodo.23047609
- All-versions DOI: https://doi.org/10.5281/zenodo.22962036

# v0.3.1 — Citation metadata correction

This documentation-only patch supplies the v0.3.1 Zenodo DOI in
README.md and CITATION.cff, records the published v0.3.0 DOI separately, and
replaces the outdated statement that the v0.3.0 DOI had not been assigned.

- v0.3.1 version DOI: https://doi.org/10.5281/zenodo.22994318
- v0.3.0 published version DOI: https://doi.org/10.5281/zenodo.22993052
- All-versions DOI: https://doi.org/10.5281/zenodo.22962036

No verifier implementation, build procedure, validation result or raw
measurement record is changed. No new computation or performance measurement
was performed for this patch. Verification coverage remains 1 <= n < 2^50.

## v0.3.0 implementation and measurement record

# v0.3.0 — Optional AMD HIP GPU backend

This release adds GPU tracing of leaf arithmetic-progression descriptors, GPU
reduction, two-stream dispatch and CPU handling of exceptional trajectories.
The CPU still generates and prunes the class tree. The existing `verify2.c`
and its CPU build remain available.

Tested on Ryzen 7 5700X (8 cores / 16 threads), Radeon RX 9070 XT (`gfx1201`),
official ROCm 10.0.0 and ROCDXG 1.2.2 in WSL Ubuntu 26.04.1.

The GPU implementation repeated **[2^48, 2^50)** with exit code 0, **161/161**
continuous nonduplicated chunks, **fails=0**, and **422,212,465,065,984** odd
starting values exactly accounted for. Wall time including initialization:
**2 h 39 min 20.908 s**; summed computation time: **2 h 39 min 17.146 s**.
Mode-0 maxsteps=838 at n=588,127,695,714,561 has an independently checked
certificate. It is not an exact first-stopping-time record.

In a separate warmup + three-repetition interleaved benchmark over
**[2^48-2^40, 2^48)**, steady CPU/GPU medians were **19.856 / 12.438 s**
(**1.596x**), with ranges **18.903–21.355 / 11.755–12.957 s**. Category
counters matched in all runs. This is a partial-range result; no measured
whole-range CPU/GPU speedup is claimed.

The package includes source, build instructions, raw logs, execution conditions,
source/build SHA-256 records, checkpoint and API checks, real GPU mode-2 checks,
and an independent production-log auditor. Runtime GPU source bytes match the
measured build. At 16 producers, the GPU front end requests about **1.88 GB**
of pinned host memory. Other GPU architectures and native Windows builds were
not tested. A compact-formatting compiler warning remains in the recorded build.

The demonstrated range remains **1 <= n < 2^50**, combining the archived CPU
coverage with this repeated upper-interval GPU run. This finite verification
does not prove the 3x-1 conjecture and does not claim verification to 2^52.

See [GPU build and run instructions](https://github.com/yygotm/3x-1-verification/tree/v0.3.0/gpu)
and [measurement evidence](https://github.com/yygotm/3x-1-verification/tree/v0.3.0/results/v0.3.0).
The v0.3.0 Zenodo archive is https://doi.org/10.5281/zenodo.22993052.
