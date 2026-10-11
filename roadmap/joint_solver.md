<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: Unified / Joint Solver for Titration Fitting

**Status:** ADD (design + evaluation) · **Owner:** Conrad Hübler · **Author note:** Claude Generated

## Context

SupraFit fits titration models (`nmr_any`, `uvvis_any`, `fl_any`, the fixed 1:1 / 2:1 / 1:2
variants, and `itc_any`) with a **two-level nested solver**:

- an **outer** non-linear least-squares optimiser over the global stability constants (log β) and the
  linear local parameters (NMR shifts / UV-Vis ε / fluorescence coefficients), and
- an **inner** speciation solve that turns each data point's total concentrations + the current β into
  free / species concentrations.

The inner solve runs **once per data point on every outer residual evaluation**, and again for every
finite-difference Jacobian column. That re-solving dominates fit time, and it is the cost this work
package evaluates removing by treating the equilibrium concentrations and the fit parameters as **one**
optimisation instead of nesting.

This document is a design/evaluation artefact only — **no production code is changed here**.

## Goal

Reduce fit wall-time (dominated by inner speciation re-solves) without regressing the numerical
robustness the current provably-convex inner Newton buys, and without breaking the established
LevMar reference behaviour that Cross-Validation / Reduction Analysis depend on. Concretely: evaluate
(a) a joint full-space/KKT formulation, (b) warm-started / one-shot nesting, and (c) analytic
concentration sensitivities, and recommend what to build first.

## Current Nested Architecture

### Outer loop — the fit-parameter optimiser

Dispatched in `src/core/minimizer.cpp:97-102`: the `"FitSolver"` optimizer-config key selects `VarProFit`
(if the model `SupportsVarPro()`) or the classic `NonlinearFit`.

| Variant | File | What it optimises | Jacobian |
|---|---|---|---|
| Classic full-vector LM | `src/core/optimizer/eigen_levenberg.cpp` | globals **+ all** locals jointly | `Eigen::NumericalDiff` (finite diff over the whole vector) |
| VarPro (Kaufman) | `src/core/optimizer/varpro_levenberg.cpp` | only the non-linear globals; locals projected out | forward-difference over globals only |

- Classic: `MyFunctor::operator()` (`eigen_levenberg.cpp:68-83`) sets the **full** parameter vector on
  the model and calls `model->Calculate()`; wrapped in `Eigen::NumericalDiff<MyFunctor>`
  (`eigen_levenberg.cpp:113`) and `Eigen::LevenbergMarquardt` (`:114`). The step loop is
  `eigen_levenberg.cpp:134-150`. NumericalDiff perturbs **every** parameter one at a time, so each
  Jacobian costs `(n_global + n_series·n_local)` extra full `Calculate()` sweeps.
- VarPro: `VarProFit` (`varpro_levenberg.cpp:59`). Its `residualVector` lambda
  (`varpro_levenberg.cpp:84-95`) sets the globals, calls `model->ProjectLinearParameters()` to refit the
  linear locals by masked least-squares, then `Calculate()`. The forward-difference Jacobian
  (`varpro_levenberg.cpp:117-125`) perturbs **only** the globals, so a Jacobian costs `n_global` extra
  sweeps — the locals no longer multiply the count. This is already a large win when `n_series` is big.

`ProjectLinearParameters()` for the reaction models is `nmr_any_Model::ProjectLinearParameters`
(`nmr_any_Model.cpp:193-200`) → `CalculateConcentrations()` + `AbstractTitrationModel::SolveLinearMasked()`
(`AbstractTitrationModel.cpp:326-367`, a per-series masked `colPivHouseholderQr`).

### Inner solve — speciation / concentration

- Wrapper: `SpeciationEngine::solve(totals)` (`src/core/speciationengine.cpp:68-81`) →
  `BFGSConcentrationSolver::solve()` (`src/core/bfgsconcentrationsolver.cpp:172-334`).
- Method: despite the class name, the default is a **damped Levenberg-Marquardt Newton** on the strictly
  convex log-space mass-balance potential `G(x)=Σexp(x_i)+Σc_j(x)−Σt_i x_i`, with the **analytic Hessian**
  `diag(s_i)+Σ c_j m_j m_jᵀ` (`bfgsconcentrationsolver.cpp:94-170`), solved by Cholesky
  (`LLT`, `:299`). 7-8 iterations, uniform 1e-12 mass balance (see `bfgsconcentrationsolver.h:28-62`).

### Where the inner solve is invoked from the outer loop

The model's `Calculate()` → `CalculateVariables()` (virtual, `AbstractModel.h:927`) →
`CalculateConcentrations()`, which loops over all points and solves speciation per point:

- `nmr_any_Model::CalculateConcentrations` (`nmr_any_Model.cpp:149-180`): loop `for i in [DataBegin,DataEnd)`
  building `totals` and calling `m_speciation.solve(totals)` at **`nmr_any_Model.cpp:153`**.
- Same pattern: `uvvis_any_Model.cpp:158`, `fl_any_Model.cpp:158`, and `itc_any_Model.cpp:178`
  (ITC's loop is inherently **sequential** — incremental heat depends on the previous point,
  `itc_any_Model.cpp:172-174`).

So one outer residual eval = `N` inner solves; one FD Jacobian = `(#perturbed params + 1)·N` inner
solves. With the classic solver and many series this is thousands of inner solves per outer iteration,
times the Monte-Carlo / CV repeat count.

### Key finding: the advertised warm start is discarded in the fit path

`BFGSConcentrationSolver` documents warm-starting from the previous solve
(`bfgsconcentrationsolver.h:108-111`), and it does keep `m_free` as a warm start at the end of `solve()`
(`bfgsconcentrationsolver.cpp:331`). **But** the model path calls `SpeciationEngine::solve` →
`setTotalConcentrations` for **every** point (`speciationengine.cpp:70`), and
`setTotalConcentrations` unconditionally clears the guess flag (`bfgsconcentrationsolver.cpp:64`), so
`solve()` re-runs `Guess()` (`bfgsconcentrationsolver.cpp:180-181`) and overwrites `m_free` with the
cold low-concentration heuristic. **Point-to-point warm-starting therefore never happens in a real fit**,
and the "7-8 iterations/solve" figure is a **cold-start** number. `benchmark_speciation.cpp` reproduces
the same cold pattern (it too calls `setTotalConcentrations` per point, `benchmark_speciation.cpp:82`),
so today's headline inner cost is measured without the warm start that a sweep of nearby points would
naturally provide. This is the cheapest, lowest-risk speedup available and is the seed of Approach A.

## The Math of a Joint Formulation

### Inner problem (per point)

For point `i` with total-concentration vector `t_i` (length `n_comp`), stoichiometry matrix `M`
(`n_comp × n_species`), and constants `β`, the free concentrations `s_i = exp(x_i)` (log variables
`x_i` enforce positivity) satisfy mass balance, which is the stationarity of the convex `G_i`:

```
g_i(x_i, β) := ∇_x G_i = s_i + M c_i(x_i,β) − t_i = 0,
  with c_{i,j} = β_j ∏_k s_{i,k}^{M_kj} = exp(log β_j + Mⱼᵀ x_i).
```

`∂g_i/∂x_i = H_i = diag(s_i) + Σ_j c_{i,j} mⱼ mⱼᵀ` is exactly the SPD Hessian the inner solver already
forms and Cholesky-factors (`bfgsconcentrationsolver.cpp:143-155, :299`). Because
`∂c_{i,j}/∂(ln β_j) = c_{i,j}`, the coupling to the globals is the cheap column
`∂g_i/∂(ln β_j) = mⱼ c_{i,j}`.

### Observable model (linear in the locals)

Every supported observable is **linear in the local parameters** once concentrations are known. For a
design row `d_i(x_i)` (e.g. observed-component mole fractions per species, `m_molar_ratios` in
`nmr_any_Model.cpp:189`) and per-series local vector `φ_l`:

```
y_model(i,l) = d_i(x_i)ᵀ φ_l.
```

### Outer objective and the nested vs joint views

```
Nested:  min_{β,φ}  Φ = Σ_{i,l} w_{i,l} ( y_obs(i,l) − d_i(x_i(β))ᵀ φ_l )²,
         where x_i(β) is defined implicitly by g_i(x_i,β)=0  (solved inner-loop).
```

The **joint / full-space** view promotes every `x_i` to an optimisation variable and enforces mass
balance as an equality constraint:

```
Joint:   min_{β,φ,{x_i}}  J = ½ Σ_{i,l} w_{i,l} ( y_obs(i,l) − d_i(x_i)ᵀ φ_l )²
         s.t.  g_i(x_i, β) = 0   for each point i   (n_comp equations per point).
```

Lagrangian `L = J + Σ_i λ_iᵀ g_i`. KKT stationarity:

```
∂L/∂x_i = ∂J/∂x_i + H_iᵀ λ_i = 0            (per point)
∂L/∂λ_i = g_i(x_i, β)          = 0          (per point, mass balance)
∂L/∂φ_l = ∂J/∂φ_l              = 0          (linear least-squares in the locals)
∂L/∂β   = Σ_i (∂g_i/∂β)ᵀ λ_i   = 0          (couples all points through β)
```

### Sparsity: arrowhead / block-bordered

Order the unknowns as `[ (x_1,λ_1), …, (x_N,λ_N), φ, β ]`. Each per-point block `(x_i,λ_i)` couples
**only** to the shared `φ` and `β` — never to another point. The Newton/KKT matrix is therefore
**block-bordered ("arrowhead")**: `N` independent `2·n_comp × 2·n_comp` diagonal blocks plus a thin
border of width `(n_local·n_series + n_global)`. The per-point diagonal blocks are invertible using the
inner solver's own `H_i` factorisation, so a **Schur complement** eliminates all `2N·n_comp` per-point
unknowns and leaves a small dense system of exactly the outer size `(β, φ)`. Doing that elimination
**exactly** is algebraically identical to the analytic-sensitivity approach below — the "full-space" and
"reduced" formulations are two views of the same Newton step.

### Analytic sensitivities (implicit-function theorem)

Differentiating the converged constraint `g_i(x_i(β), β) = 0`:

```
H_i (dx_i/dβ_j) + mⱼ c_{i,j} = 0   ⟹   dx_i/dβ_j = − H_i⁻¹ (mⱼ c_{i,j}).
```

Hence the outer residual Jacobian w.r.t. the globals, **reusing the Cholesky factor of `H_i`** from the
converged inner solve (one back-substitution per (point, β_j), no extra inner solve, no finite
differencing):

```
d y_model(i,l)/dβ_j = φ_lᵀ (∂d_i/∂x_i) (dx_i/dβ_j)
                    = − φ_lᵀ (∂d_i/∂x_i) H_i⁻¹ mⱼ c_{i,j}.
```

### How this interacts with VarPro

The locals `φ` enter only the objective and only linearly. In the reduced/Schur view, **eliminating the
`φ` block is precisely VarPro's linear projection** (`SolveLinearMasked`). So a joint solver **subsumes**
VarPro's local-projection as one of its eliminated blocks; the analytic-sensitivity route
**complements** VarPro — VarPro removes the locals from the Jacobian, sensitivities remove the finite
differencing over the globals. The two are orthogonal wins and stack.

## Candidate Approaches

| # | Approach | Attacks | Expected speedup | Robustness | Complexity / risk | Fit with abstractions |
|---|---|---|---|---|---|---|
| **A** | **Warm-started / one-shot nested** — keep nesting; reuse the previous outer-iterate's per-point `x_i`; optionally cap inner iterations + loosen inner tol early | inner **iteration count** per solve | inner iters/solve ~7-8 → ~1-2 near convergence; **~3-5× on inner cost** (the dominant term) | **Unchanged** — same convex Newton, just better seeds | **Low.** Per-point `x_i` cache in `SpeciationEngine`; stop clearing the warm start; adaptive inner tol | Contained in `SpeciationEngine`/`BFGSConcentrationSolver` + the model call sites; orthogonal to VarPro/LM |
| **B** | **Analytic outer Jacobian via implicit sensitivities** — replace FD Jacobian columns with `−φᵀ(∂d/∂x)H⁻¹ mⱼ c_j`, reusing `H_i`'s factor | the **`n_global` extra sweeps** per outer iteration (FD Jacobian) | removes `n_global·N` inner solves per iteration; grows with `n_global` and `N`; also kills FD noise → better conditioning near the optimum | **Improved** near the optimum (exact derivatives) | **Medium-High.** Solver must expose the factor + species; each model must expose `∂d_i/∂x_i`; wire into VarPro (and/or classic) Jacobian | New virtuals on `AbstractModel`/`AbstractTitrationModel`; `SpeciationEngine` returns factor + `c_j`; complements VarPro |
| **C** | **Full-space / SQP KKT** — all `{x_i}, β, φ` unknowns; one arrowhead Newton step per iteration; Schur-eliminate per-point blocks | inner **and** outer merged (mass balance only satisfied at the joint solution) | modest over B (same factorisations); saves only the "fully converge inner every step" work | **Riskier** — the joint problem is nonconvex in β; loses the per-point convexity that makes today's inner solve bulletproof; needs its own trust-region/line-search globalisation | **High.** A new solver alongside the minimizer dispatch; large sparse assembly + KKT regularisation | Largest new surface; would live beside `NonlinearFit`/`VarProFit` in the `FitSolver` dispatch |

### Notes and honest downsides

- **A** is the only approach that changes nothing about the math or the reference behaviour — it just
  stops throwing away information. The one risk is a *bad* warm start on a discontinuous total-conc jump;
  mitigate by falling back to `Guess()` if a warm-started solve fails to converge in a few steps. The
  "one-shot" (cap inner to 1 Newton step) variant is more aggressive: an under-converged inner solve
  biases the outer residual and Jacobian, so it needs an adaptive tolerance that tightens as the outer
  fit converges, or it can stall the outer optimiser. Recommend shipping the warm-start half first and
  treating the iteration cap as an opt-in experiment.
- **B** is the principled speedup but is only *correct* if `∂d_i/∂x_i` is implemented per model
  (mole-fraction / Beer-Lambert / fluorescence design rows all differ) and validated against finite
  differences. It reuses the inner `H_i` factorisation, but that factor is currently local to `solve()`
  and discarded — exposing it (and the converged `c_j`) is the main plumbing. ITC's inter-point coupling
  (`itc_any_Model.cpp:172-174`) makes its observable Jacobian more involved than the titration models'
  (mass balance stays per-point, but the heat observable couples neighbours), so stage ITC last.
- **C** is the most code and the least safe. Its theoretical-only advantage over B — not driving each
  inner solve to 1e-12 at every outer step — is small because B already reuses the same factorisations,
  and it forfeits the current solver's headline property (a single global minimum per point, guaranteed).
  Not recommended as a first or second step.

## Recommendation

**Pursue A first, then B. Do not build C now.**

1. **Approach A (warm-started nested).** Highest value per unit risk. Fix the discarded warm start
   (`bfgsconcentrationsolver.cpp:64` clears the guess that `speciationengine.cpp:70` triggers every
   point), add a per-point `x_i` cache so each outer iterate seeds each point from *its own* previous
   solution, and loosen the inner tolerance early in the outer fit (it is pinned at 1e-12 in
   `AbstractTitrationModel.cpp:315` and `speciationengine.cpp:25`). Zero change to results, large change
   to inner iteration count. Stacks with both VarPro and classic LM.
2. **Approach B (analytic sensitivities).** The principled follow-up: expose the inner `H_i` factor +
   converged `c_j` from `SpeciationEngine`, add a per-model `∂d_i/∂x_i`, and replace the FD Jacobian
   columns in `VarProFit` (and optionally the classic path) with the analytic expression. Biggest win
   when `n_global` and `n_series` are large, and it *improves* conditioning near the optimum. Validate
   analytic-vs-FD before trusting it, and gate it behind a new `"FitSolver"` value so the FD path stays
   the oracle.
3. **Approach C (full-space KKT).** Revisit only if A+B leave a measured, real bottleneck — unlikely,
   given they already remove both dominant costs while keeping the convex inner solve.

Rationale: A attacks the dominant cost (inner iterations) with no numerical risk and no reference
change; B attacks the other dominant cost (FD Jacobian sweeps) and is exact; both reuse existing
abstractions and complement VarPro; C's marginal upside does not justify discarding the provably-convex
inner Newton or writing a new KKT solver.

### Rough effort

| Approach | Effort | Risk |
|---|---|---|
| A — warm start + per-point cache + adaptive inner tol | ~2-4 days | Low |
| A' — optional one-shot (inner iteration cap) experiment | +1-2 days | Medium (outer stalling) |
| B — analytic sensitivities (titration models; ITC last) | ~1.5-2.5 weeks | Medium |
| C — full-space / SQP KKT solver | ~4-8 weeks + robustness hardening | High |

## Phased Roadmap

- **Phase 0 — Instrument (0.5 day).** Add total-inner-iterations and total-inner-solves counters to a
  fit and print them in `benchmark_varpro.cpp`. Freeze baseline: cold-start iters/solve, fit wall-time
  vs `n_series ∈ {3,10,30,60}` and vs `n_global`, and the reference SSE/β.
- **Phase 1 — A: warm start (1-2 days).** Stop clearing the warm start point-to-point; add a per-point
  `x_i` cache keyed by point index in `SpeciationEngine`, seeded from the previous outer iterate; keep a
  `Guess()` fallback if a warm-started solve fails. Re-run Phase-0 benchmark (expect inner iters/solve to
  fall sharply). Results must be **bit-for-bit** unchanged (warm start changes the path, not the fixed
  point) — assert against the frozen SSE/β.
- **Phase 2 — A: adaptive inner tolerance (1 day).** Loosen the inner convergence threshold while the
  outer SSE change is large; tighten to 1e-12 as the outer fit converges. Guard CV/RA parity.
- **Phase 3 — B: expose inner Jacobian (3-4 days).** Return the converged `H_i` Cholesky factor and
  species `c_j` from `SpeciationEngine::solve` (or a sibling accessor). Unit-test `dx_i/dβ_j` against
  finite differences on the benchmark systems.
- **Phase 4 — B: analytic outer Jacobian (1 wk).** Add per-model `∂d_i/∂x_i` (nmr mole fractions,
  uvvis/fl Beer-Lambert). Replace the FD Jacobian in `VarProFit` with the analytic columns behind a new
  `"FitSolver"` value (`VarProAnalytic`). Validate SSE/β against the LevMar oracle to VarPro tolerance.
- **Phase 5 — B: ITC + classic path (optional, 3-5 days).** Extend sensitivities to `itc_any`'s coupled
  observable and, if worthwhile, to the classic full-vector LM.
- **Phase 6 — C (deferred).** Only if a real bottleneck remains after A+B.

## Benchmarking

- **Inner solver:** `src/tests/benchmark_speciation.cpp` already reports ns/solve and iter/solve on 1:1,
  2:1/1:1, 1:1/1:2, self-aggregation, and 3-component sweeps. For Approach A, make it **warm-start**
  (seed each point from the previous point's result rather than calling `setTotalConcentrations`, which
  clears the guess) and report the drop in iter/solve; this isolates the inner win from the fit.
- **Fit level:** `src/tests/benchmark_varpro.cpp` already fits `nmr_any`/`uvvis_any` under
  `FitSolver=LevMar` vs `FitSolver=VarPro` across `n_series ∈ {3,10,30,60}` and prints wall-time +
  reached SSE. Add columns for **total inner solves** and **total inner iterations** per fit, and a new
  `FitSolver=VarProAnalytic` column for Approach B. Success = same SSE/β (to VarPro tolerance) at lower
  wall-time and far fewer inner iterations.
- **Correctness oracle:** the classic full-vector LM (`FitSolver=LevMar`) stays the reference. Assert
  Approach A reproduces it bit-for-bit; assert Approach B matches to the same tolerance
  `test_varpro`/`test_varpro_cv` already use, and cross-check analytic `dx/dβ` against finite
  differences (Phase 3). Watch the documented VarPro caveat on flat/ill-determined directions
  (`varpro_levenberg.cpp:33-41`): report RA/MC parity, not just fit SSE.
- Context: the inner LevMar-Newton needs 7-8 cold iterations (~4 µs/solve at -O3) vs 23-146 for the
  legacy BFGS on ill-conditioned points — so the inner solve is already fast per call; the win is
  **fewer calls and fewer iterations per call**, which is what these benchmarks must expose.

## Risks

- **Warm-start regressions (A):** a warm start from a distant point could slow (not wrong — the fixed
  point is unchanged) a solve; keep the `Guess()` fallback and a max-iteration trip.
- **One-shot bias (A'):** capping inner iterations under-converges mass balance and biases the outer
  residual/Jacobian; only safe with an adaptive tolerance that tightens near the outer optimum. Ship as
  opt-in.
- **Analytic-derivative correctness (B):** per-model `∂d_i/∂x_i` is easy to get subtly wrong; mandatory
  FD cross-check and behind a new `FitSolver` flag with LevMar as oracle.
- **Reference/statistics parity:** CV and Reduction Analysis are solver-sensitive on flat directions
  (documented in `varpro_levenberg.cpp:33-41`); any new solver must be validated on RA/MC, not only fit
  SSE, and the classic LevMar path must remain available.
- **ITC coupling (B):** `itc_any`'s sequential per-point heat model complicates the observable Jacobian;
  stage it last and keep it on the FD path until validated.
- **Full-space nonconvexity (C):** the joint problem is nonconvex in β; a full-space solver forfeits the
  per-point global-minimum guarantee and needs real globalisation work — the main reason to defer it.

## Status & remaining work (2026-07-13)

**Delivered (committed on `feature/bfgs-speciation-solver`):**
- **Approach A — done.** Point-to-point warm start (`8b9ba806`) + per-point `x_i` cache keyed by data
  index in `SpeciationEngine::solve(totals, index)` (`3fa7dc60`). Gated to the convergent Newton method
  (a stalled BFGS point is a bad seed). Results unchanged (strictly convex ⇒ start-independent); big fit
  speedups (`test_varpro` 3.5 s→0.73 s, `test_varpro_cv` 14.7 s→2.5 s). Verified thread-safe under MC/CV
  (per-model-clone engine) by `test_speciation_warmstart`.
- **Approach B — working for `nmr_any`, incl. CV/RA.** `BFGSConcentrationSolver::sensitivityMatrix()` =
  ∂x/∂lnβ from the stored solution Hessian (`78098f43`, validated vs FD). `AbstractModel::
  AnalyticVarProJacobian` (default: none → FD) implemented on `nmr_any` as the **full Golub–Pereyra**
  Jacobian — the Kaufman (φ-fixed) form matched a fixed-φ FD but its rank-deficient Gauss–Newton Hessian
  stalled the outer LM; adding the projection derivative ∂φ/∂β fixed it (`c853b5fb`). Now **mask-aware**:
  compacts rows to the residual list and projects per series over the checked rows, so `VarProAnalytic`
  also accelerates Cross-Validation / Reduction Analysis. New `FitSolver=VarProAnalytic`. Validated:
  analytic == full FD Jacobian to 1e-7 (full and masked); recovers β at the same SSE as FD VarPro
  (~1e-23); CV/RA boxplots match FD VarPro (`test_varpro_cv`).
- Phase 0 instrumentation (inner-solve/iteration counters in `benchmark_varpro`) — **not yet done**; the
  speedups above are wall-time from the test suite, not the isolated inner-iteration counts.
- GUI: the speciation-solver switch is validated (operator-confirmed click behaviour).

**Remaining — priority order:**
1. ~~**Analytic Jacobian for CV / RA (masked projection).**~~ **Done (2026-07-13).**
   `nmr_any::AnalyticVarProJacobian` now compacts its rows to the residual list (active+checked, i-major
   /j-minor) and builds the projection `DᵀD`/`R` per series over the checked rows — the same mask
   `SolveLinearMasked` uses (CV/RA disable whole rows via `isChecked`; a shrunk `DataBegin..DataEnd`
   window is still FD-fallback as it is not the CV/RA mechanism). Validated: analytic == FD on masked
   data (`test_varpro::analyticJacobianMasked`, 3e-7), and `VarProAnalytic` CV/RA boxplots match FD
   VarPro (`test_varpro_cv`, both fixtures). The unified path subsumes the full-data case.
2. **MC/RA solver-behaviour investigation (projected vs joint).** *Behaviour is allowed to differ between
   the projected solvers and the classic joint LM — but it must be characterised, not silently inherited.*
   MC already warm-starts each resample from the point-fit optimum (`NonLinearFitThread` does no
   `InitialGuess`), so it is built to stay in the nearby minimum; the solver only sets how far the
   ill-determined/nuisance directions may drift from there. Hypothesis to test: VarPro re-projects a
   nuisance species' linear partner (shift/ε) every step, flattening its β direction → the nuisance
   constant gets more "play" and its MC scatter smears the correlated correct constants, whereas the
   joint LM keeps it pinned. Deliverable: a reproducible 3-way MC comparison (LevMar / VarPro /
   VarProAnalytic) on a 1:1/1:2 dataset fitted with an *added* dimerisation species `2 A <=> A2` (a true
   nuisance), reporting the distribution/scatter/correlation of β(A2) and of the correct β(AB)/β(AB2).
   Open choice: synthetic (self-contained) vs the operator's real `1_1_1_2`-with-dimerisation data; and
   CLI-benchmark (printed distributions) vs a ctest with a frozen expectation (turns red on solver change
   → forces the investigation).
3. **Extend `VarProAnalytic` to the other engine models.** `uvvis_any` (Beer–Lambert Σ c·ε), `fl_any`
   (Σ c·φ), then `itc_any` last (its sequential per-point heat couples neighbours → the observable
   Jacobian is more involved). Each needs its own `AnalyticVarProJacobian`; the `sensitivityMatrix()`
   infrastructure is shared. Same FD cross-check gate as `nmr_any`.
4. **Benchmark `VarProAnalytic` vs FD VarPro** (Phase 0 + the `benchmark_varpro` column): total inner
   solves / iterations and wall-time vs `n_series` and `n_global` — quantify the removed `N·n_global`
   speciation re-solves per Jacobian.

**Non-solver housekeeping (surfaced during this work):**
- The `release/` build tree has an **empty `CMAKE_BUILD_TYPE`** → it compiles **without `-O3`**;
  reconfigure with `-DCMAKE_BUILD_TYPE=Release` (a full rebuild) for production/benchmark numbers.

*Deliberately NOT done: Approach C (full-space KKT) — deferred as in the recommendation above.*

---
*Claude Generated. Planning artefact + status. Copyright remains with Conrad Hübler.*
