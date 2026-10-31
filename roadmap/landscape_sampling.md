<!--
Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: SSE-Landscape Sampling (Metadynamics-like) for Global Search and Confidence Intervals

**Status:** WP-A, WP-B, WP-C, WP-C2, WP-D (GUI incl. 2D surface), WP-E (measured) ADDED (`src/capabilities/landscapesampler.*`, `landscapesearch.*`, machine-tested), C3-E ADD · **Owner:** Conrad Hübler · **Idea:** operator · **Author note:** Claude Generated

## Goal

Sample the nonlinear fit parameters with a history-dependent bias (well-tempered metadynamics), using the
least-squares sum SSE(θ) as the "energy", to (a) find further minima and (b) reconstruct the SSE surface
well enough to read **likelihood-based confidence regions** from it.

Existing neighbours: `GlobalSearch` (multistart / grid), `WeakenedGridSearch` (one-parameter sweeps with refit,
cutoff), `MonteCarloStatistics`, `ResampleAnalyse`. The sampler is a new `AbstractSearchClass` next to them,
not a replacement.

## Decisions (operator)

- No physical dynamics: **T is an artificial parameter**, like the step size. Both samplers are offered:
  **Metropolis MC** (needs SSE only) and **Langevin-like** (forward/central finite-difference gradient of SSE).
- **Well-tempered** bias (hill height decays with the bias already deposited, bias factor ΔT is a setting).
- An **SSE threshold** bounds the accessible region; the run must not end at the first crossing or the
  first filled basin, otherwise small hills hide further minima (see Termination).

## Design

### Sampling space
- Variables: **log K** of the global parameters (K ≥ 0, K spans decades; a flat valley towards K→∞ becomes a
  bounded or at least linear direction). Box limits per parameter from the model's bounds.
- Locals (shifts, ε, coefficients) are **not sampled**: SSE(θ) is evaluated with `ProjectLinearParameters()`
  for `SupportsVarPro()` models, i.e. SSE is already the profile over the linear parameters. This keeps the
  dimension at 1-3 for the usual models. v1 supports VarPro models only; classic models later (refit locals
  per point, expensive).
- Gradient (Langevin): finite differences over the sampled variables only; step relative to the parameter, tied
  to the inner solver tolerance (too small a step reads solver noise). An analytic gradient from the speciation
  sensitivities is possible later ([joint_solver.md](joint_solver.md), option c).

### Temperature
- Natural scale: `kT = 2 σ²`, `σ² = SSE_min / (n - p)` (then exp(-SSE/kT) is the likelihood). `T` is a multiple of
  that scale (default 1), setting only how easily barriers are crossed, not what the result means.

### Bias
- Gaussian hills at the visited points, width per variable (default: fraction of the box in log K).
- ≤ 3 variables: bias on a grid (fast lookup/update). More: hill list with a cutoff radius.
- Well-tempered: height `w · exp(-V(θ)/ΔT)`; hills deposit every `stride` steps.

### Two outputs from one run
1. **Profile from the samples:** every SSE evaluation is exact and deterministic, so the profile
   `SSE_prof(θ_i) = min SSE over visited points in the bin of θ_i` needs no bias reconstruction. This is what
   the confidence interval needs (profile, not marginal).
2. **Reconstructed free-energy surface** `-V(θ)` (well-tempered limit, scaled by `(T+ΔT)/ΔT`) for plotting and as
   a convergence check (bias stops changing).

### Confidence region
`SSE ≤ SSE_min · (1 + p/(n-p) · F_{α;p,n-p})`; per parameter the interval is the extent of the profile below
that level. Compare against Monte Carlo and `WeakenedGridSearch` on the same model.

### Termination
- **SSE threshold `SSE_cut`** (default: a multiple of the confidence level, e.g. 3x the F-level): moves with
  SSE above it are rejected (wall); the bias therefore fills only the accessible region.
- Stop by **coverage + patience**, not by the first event: no new bin below `SSE_cut` visited and no new
  minimum found for `N_patience` deposits, after at least `N_min` steps; hard cap `MaxSteps`.
- Each new local minimum found is polished with the normal minimiser (and reported like a `GlobalSearch` hit).
- Wall confirmed by the operator. Patience counts only once the hill height is below `minHillFraction` of its start value
  (bias converged); otherwise a visited but unfilled basin ends the run before the walker climbs out.

### Parallelism and reproducibility
Independent walkers on the thread pool (like `GlobalSearch`), bias per walker or shared (multiple-walkers
metadynamics, later). Seed is a setting; results are stored in the project JSON like the other analyses.

## Work packages (one buildable commit each)

| WP | Content | Test |
|---|---|---|
| A | Headless sampler core on a plain functor `f(θ)` (MC + Langevin, well-tempered bias, grid, wall, termination); no model dependency | analytic surfaces: double well (barrier known), banana valley, flat unbounded valley (must stop at `MaxSteps`/wall), profile of a quadratic = known ellipse |
| B | Profile and F-threshold confidence region from visited points | quadratic surface: interval equals the analytic one; compare with constrained refit on a 1:1 model |
| C | `LandscapeSearch : AbstractSearchClass` on a VarPro model (log K, `ProjectLinearParameters`), JSON out, box from the fit, box expansion — ADDED | scan (1 constant) and exact 2D profile (2 constants) |
| C2 | method id 8, `ModelStatisticsStore`, project JSON save/restore, `JobManager`, polish of the minimum candidates with the minimiser — ADDED | job -> model -> export -> import |
| C3 | `StatisticTool::CalculateLandscapeMetrics`, `analysis_manager`/CLI option, agent tool (`tools_analysis`), `MLFeatureExtractor`; ends that stay `Unsampled` (neighbour bin behind the wall) | vs. `WeakenedGridSearch` and Monte Carlo |
| D | UI tab (few fields, rest behind ..more..), list entry, result page: one chart with a `Show:` choice (profiles, one profile, a surface per pair as height lines), interval table, `docs/ASSISTANT_HELP.md` entry — ADDED | offscreen tests with a real run and with a synthetic three-constant result |
| E | MEASURED, see docs/landscape_sampling_measurement_2026-10-24.md - Evaluation: number of SSE evaluations vs. profile by constrained refit and vs. MC; quote the worst case over several data sets (not one sample) | measurement document in `docs/` + index line |

## Risks

- Unbounded valleys never converge (bias keeps filling): wall, box and `MaxSteps` are required, not optional.
- The bias surface is a marginal at finite T; only the sample profile is a profile. Do not label the surface a CI.
- The honest benchmark is the cheap alternative (profile likelihood by warm-started constrained refit,
  multistart). If the sampler does not beat it on found minima or cost, it stays an expert option.

## Findings (WP-C)

- Isotropic Metropolis steps fail on the thin tilted valleys of correlated constants (acceptance 3 %, profile edges
  unvisited); adaptive proposals (chain covariance, acceptance target) fix that (test `adaptiveProposalFollowsAThinValley`).
- The extent of the level along one variable alone understates the profile extent of correlated constants, so the box
  is doubled and the walk repeated while an interval reaches it (`MaxExpansions`); costs rounds, not accuracy.
- NMR 1:1/1:2: with few points below one equivalent lg beta 1 is open to the top, and lg beta 2 stays open towards
  "no second complex" even with good data. Sampler and an exact grid profile agree; the intervals say so with `Box` ends.
- Cost for two constants about 7e5 SSE evaluations (15 s, 4 threads), not cheaper than a grid; the sampler is for three
  or more variables and for finding other minima. Measure in WP-E.
- Hill width is a fraction of the box; after a box expansion the hills are wide against a thin valley. Candidate
  improvement: derive the hill width from the chain covariance as well.

## Decision after WP-E

Measured 2026-10-24: the sampler gives the same intervals as the profile likelihood by constrained refit but costs 14-3000x
the SSE evaluations; random multistart beats it for finding a deeper minimum on the one case available. Operator decision:
build the GUI (WP-D) anyway. Proposal not yet decided: add the constrained-refit profile as the cheap interval method.

## Minima search (2026-10-26)

`Mode` "Minima" (GUI task "Find other minima"): wall `SearchWall` x SSE of the fit, fixed box, temperature scaled to the wall, minima refitted, merged by
basin identity, with their models for Load/Add. Barrier crossing is proven on analytic double wells; on titration data a second basin was
not found beyond a flat valley and a separate region without the second species (data set of `finderFindsTheOtherBasin`) - **the WP-E
"wrong basin" was a stalled fit, see the correction in `docs/landscape_sampling_measurement_2026-10-24.md`**. Done for two constants (2026-10-26): the unvisited cells of the explored region are evaluated exactly (at most bins^2 = 3600
evaluations, the rectangle grows until the region below the wall is enclosed), surface and profiles are exact and the intervals equal an independent grid;
for two constants the walk is then almost beside the point. Three constants (2026-10-31): holes of the pair surfaces (cells enclosed by visited ones) are closed exactly by a search over the third constant (`ExactSurface`, at most `ExactPairCells` = 3000 cells, about 25 evaluations each; measured 14x the evaluations of a 1500-step run, worst excess over a dense scan 5e-8). Reading the surface from the bias was built and measured on a correlated paraboloid (3 variables, 4 seeds): no better than interpolating the visited cells (mean error 0.9 against 0.7 SSE units at a level excess of 6; the bias is not converged where the gaps are) and removed. Open for four and more constants: gaps in the surfaces, the exact fill needs a search over several constants. A better benchmark for the finder needs a data set with a proven barrier (not found yet).
