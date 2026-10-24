<!--
Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - measurement document (WP-E of roadmap/landscape_sampling.md)
-->

# Landscape sampling vs profile likelihood, Monte Carlo and multistart (2026-10-24)

Tool: `src/tests/benchmark_landscape.cpp` (target `benchmark_landscape`, manual, not a ctest), release build, 4 threads for the
sampler and Monte Carlo, 1 thread for the reference. Commit of the sampler at the time: after WP-C2 plus `MaxRange` and re-centring.

## What was compared

- **Sampler**: `LandscapeSearch`, default settings (Metropolis, adaptive proposals, 4 walkers, box from the fit, `MaxRange` 10,
  re-centring on a deeper minimum), 95 % level.
- **Reference**: profile likelihood by constrained refit - bracketing and bisection on one constant, Nelder-Mead over the
  others, warm started, at the same level. This is the cheap alternative from the design. Written for this benchmark, not an
  optimised implementation.
- **Monte Carlo**: `JobManager` job, 500 steps, timed only (it is a percentile interval of resampled fits, a different
  statistic; accuracy is not compared).
- **Multistart** (part 2): random starts of the ordinary minimiser, uniform in lg beta 0..8.

Data: synthetic `nmr_any` titrations, noise realisations 1-5, shifts chosen so that the species differ; all fits started from the
better of the blind fit and a fit from the true constants, so that part 1 compares interval methods and not fits.

## Part 1 - confidence intervals

Worst case over 5 noise realisations per configuration:

| configuration | max SSE evaluations sampler / profile | max wall time sampler (4 thr) | max wall time profile (1 thr) | max end deviation (of interval width) | ends not found as a crossing by both (sum) | unsampled ends (sum) |
|---|---|---|---|---|---|---|
| 1 constant, noise 0.05 | 3131 | 1.6 s | 1 ms | 0.001 | 0 | 0 |
| 1 constant, noise 0.5 | 3895 | 0.7 s | < 1 ms | 0.000 | 0 | 0 |
| 2 constants (1:1/1:2) | 16 | 13.5 s | 1.7 s | 0.033 | 0 | 0 |
| 3 constants (1:1/1:2/2:1) | 14 | 19.2 s | 2.9 s | 0.000 | 8 | 6 |

Monte Carlo (500 steps): 0.04-1.3 s on the same fits.

Reading:

- The intervals agree: worst deviation of an interval end from the reference is 3.3 % of the interval width.
- The sampler is **never cheaper** than the constrained refit here: 14-16 times the SSE evaluations at two and three
  constants, three thousand times at one constant. In wall time (sampler on 4 threads, reference on 1) 4-8 times.
- For three constants 8 ends were reported differently (crossing vs. open) and 6 ends stayed `Unsampled`; the data constrain
  the third constant poorly. Not analysed further.

## Part 2 - global search

32 data sets (4 two-constant systems x 8 noise realisations), blind start of the ordinary fit: **one** ended in a wrong basin
(`nmr 1:1/2:1`, lg beta (3.0, 4.5), seed 6, SSE 0.191 instead of 0.0223).

| | found the better minimum | SSE evaluations | wall time |
|---|---|---|---|
| landscape sampling (re-centred once) | yes | 3.0e5 | 4.0 s |
| multistart, 20 random starts | 7 of 20 starts | - | 3 ms per start |

Multistart needs about three starts (10 ms) for the same result: about 400 times cheaper. One case is no basis for more.

## Limits of this measurement

- Only `nmr_any` with one to three constants, synthetic data, five noise realisations; no real data, no UV/Vis or fluorescence.
- The reference was written for this test (Nelder-Mead, 22 bisection steps); a better one would be faster still.
- Wrong basins were rare (1 of 32); the benefit of a global search is not established by this, in either direction.
- Not measured: more than three constants, multimodal profiles, models without variable projection.

## Consequence

For confidence intervals of the constants the constrained-refit profile is the better method on every case measured. The
sampler delivers the same intervals, finds deeper minima by itself and re-centres, but at 14 times the cost or more. It stays
in the program as the way to look at the landscape, not as the default for intervals. The profile likelihood by constrained
refit is the candidate to add next (proposal, not done).
