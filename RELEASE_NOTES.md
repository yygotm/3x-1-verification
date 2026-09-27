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
