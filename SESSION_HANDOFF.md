# SupraFit — Session Handoff (2026-07-11)

Branch: `feature/bfgs-speciation-solver`. Everything below is **committed** on that branch except
the open bug at the end. Copyright stays with Conrad Hübler; new code marked "Claude Generated".

## What this work delivered

Generalised the flexible titration models to **arbitrary N-component equilibria driven by free-text
reaction equations**, rewrote the speciation solver for speed, made the solver method switchable, and
fixed a fit-divergence. Commits (newest first):

- `ef99e624` Fix nmr_any/uvvis_any fit runaway via a data-derived initial guess (`GuessLgBeta`).
- `3a01b8ee` Make speciation solver method selectable (LevMar default / BFGS legacy).
- `5f6627f2` Speed up speciation solver 12–27× (BFGS → damped Newton / Levenberg–Marquardt).
- `4efd0533` Roadmap work packages (Python interface; ScriptModel performance).
- `e6c0d6ba` N-component reaction-driven speciation (the big feature).

### Architecture
- **`src/core/reactionparser.{h,cpp}`** — parses `A + B <=> AB`, `2 A <=> A2`, `A + C <=> AC`;
  intermediate complexes may be reused as reactants (`A + AB <=> A2B`), resolved to elementary
  components with cycle detection. Arrows: `<=> = <-> -> ⇌`.
- **`src/core/speciationengine.{h,cpp}`** — wraps `ReactionSystem` + `BFGSConcentrationSolver`;
  embedded (by value, `m_speciation`) in `AbstractTitrationModel` and (separately) in `itc_any`.
- **`AbstractTitrationModel`** — now N-component: `m_component_count`, `InitialConcentration(i,c)`,
  dynamic `InputParameterSize()`, `BuildSpeciationFromReactions()`, `GuessLgBeta()`,
  `ReactionComponentMismatch()`. `Reactions_Json` (type 6) added; `Species_Json` (type 5) deprecated.
- **Models**: `nmr_any` (N-comp, observed component 0, mole-fraction×shift), `uvvis_any` (Beer–Lambert
  over free comps + species×ε), `itc_any` (stays 2-component — totals from the cell/syringe protocol —
  but gains arbitrary species). Legacy `MaxA/MaxB/MaxSelfA/Species` grid is the backward-compatible
  fallback (empty `Reactions` ⇒ grid; classic projects load unchanged).
- **Runtime citations**: `src/core/citations.{h,cpp}` + `AbstractModel::CitationKeys()/CitationBlock()`;
  BFGS-driven models cite Musketeer (DOI 10.1039/d4sc03354j) in `ModelInfo()`.
- **GUI**: `ui/widgets/reactioneditorwidget.{h,cpp}` (live-parsed reaction editor, PrepareBox type 6);
  `ui/widgets/dynamicmodelwidget.{h,cpp}` (scalable QTableView parameter tab, opt-in via
  `UseDynamicParameterWidget()`, additive). `ModelDataHolder::AddModel` shows a "Reaction / data
  mismatch" dialog when reactions need more components than data columns.

### The solver (`src/core/bfgsconcentrationsolver.{h,cpp}`)
Despite the class name it now defaults to a **damped Levenberg–Marquardt Newton** on the convex
log-space potential with the **analytic Hessian** `diag(s_i) + Σ c_j m_j m_jᵀ`. Steps solve
`(H+λI)d=-g` by Cholesky; accepted on a decrease of the objective **or** the gradient norm (the
gradient branch reaches 1e-12 mass balance where the objective is flat); λ self-tunes. Precomputes
`log β`, allocation-free `Objective`. **Selectable**: `setMethod(Method::LevenbergMarquardt|BFGS)` on
the solver and `SpeciationEngine::setMethod`. Perf: 7–8 iterations vs 54–146 for BFGS, uniform 1e-12,
1:1 2.33 ms→86 µs, 2:1/1:1 12×, self-agg 19×, 3-comp 20×. Benchmark tool:
`src/tests/benchmark_speciation` (manual; args `<reps> <threshold> [levmar|bfgs]`).

### The InitialGuess fix (`ef99e624`)
`nmr_any` seeded `lg β = guess_K + order` = `[6,7]` for 1:1/1:2 — too high for AB₂, dropping the
optimiser into a flat runaway direction (`lg β(AB2) → ~110`, SSE stuck). The model is correct (at the
true β it reproduces the classic SSE exactly; from any sane start it converges). `GuessLgBeta` now
derives `lg β ≈ (order-1)·(-lg c_ref)` from stoichiometry + data concentration scale. Recovers
`[3.81, 5.92]` matching the classic model. Regression: `test_nmr_ncomponent::testFit_1_1_1_2`.

## Tests / build
- Debug tests: `cd debug && cmake . && make <target> -j4`; run `./src/tests/<t>`. Release (for perf):
  `cd release`. NOTE: shell cwd persists between calls — always `cd` explicitly.
- Green: `test_reactionparser` (16), `test_nmr_selfaggregation` (5), `test_nmr_ncomponent` (6),
  `test_uvvis_any` (3), `test_itc_any` (4), `test_bfgs_solver` (5), reference regression (14/0, ~55 s).
- GUI builds + launches offscreen (`QT_QPA_PLATFORM=offscreen`). Links `suprafit_gui models core
  cutechart`.

## FIXED (2026-07-12) — crash: nmr_any + Monte Carlo, then add a classic model
User repro (GUI): fit an `nmr_any` 1:1/1:2 model, run **Monte Carlo**, then **add a classic 1:1/1:2
model** → crash.

**Root cause (NOT heap corruption — a null dereference).** ASan on the GUI (with SupraFit's own
SIGSEGV handler disabled so ASan could report) showed: `SEGV, READ at 0x8 (zero page)` at
`DataClass::LoadSystemParameter()` (`dataclass.cpp:473`), which does `emit Info()->SystemParameterLoaded()`
with a **null** `Info()`. `Info()` returns `d->m_info` (a `QPointer<DataClassPrivateObject>`).
`DataClassPrivate`'s copy constructors **shared** `m_info` by pointer (`dataclass.cpp:97` and `:130`)
while `~DataClassPrivate` **deletes** it. Every `Override*Table()` calls `d.detach()`, deep-copying the
project's `DataClassPrivate` per Monte-Carlo worker clone; the clones are freed via `QObject::deleteLater`,
so the GUI event loop runs their `~DataClassPrivate`, which deleted the **live project's** `m_info`
(`QPointer` → silently null; hence the `connect(...): invalid nullptr parameter` warnings just before the
crash). Adding the classic model then `emit`ted on the null `Info()`.

Why it never reproduced headless before: (a) my synthetic `m_systemObject` was empty, so
`LoadSystemParameter` never dereferenced; (b) models use a `deleteLater` deleter, so without a running
event loop the worker clones were never destroyed → `m_info` never deleted.

**Fix:** `DataClassPrivate`'s copy constructors now create their own `m_info` (`new DataClassPrivateObject`)
instead of aliasing the source's. Regression: `test_dataclass::testInfoOwnershipAcrossDetach` (fails
without the fix, passes with it). Confirmed fixed in the GUI (ASan, no crash after two 2000-step MC runs +
adding the classic model). Also committed: `src/ui/main.cpp` skips installing the SIGSEGV/SIGABRT
handler under `__SANITIZE_ADDRESS__` (so ASan can report), and `core` links `fmt::fmt-header-only`
(clean-build `fmt::vprint` link fix). ASan tree in `build-asan/` (not committed).

**Leak fixes (same area):** `~DataClassPrivate` now also deletes `m_independent_raw_model`/
`m_dependent_raw_model` (allocated by every ctor/copy-ctor, never freed), and the four `set*Table`
setters delete the table they replace (they overwrote the `QPointer` without freeing the old one).
Validated with LeakSanitizer (those leak classes gone; `test_dataclass` ASan-clean). **Still leaking
(NOT fixed):** `OverrideInDependentTable`/`OverrideDependentTable` replace a table without freeing the
old one — but in Monte Carlo those tables are tracked in `m_ptr_table` and freed at run end, and deleting
them from a worker thread would race the main thread's `QPointer` cleanup. Belongs in the flagged MC/
table-ownership restructuring, not a point fix.

### Historical (pre-fix) notes
Data file that triggered it (external, do NOT depend on it — use in-repo similar data):
`~/NextCloud/TUBAF/.../Simulation/1_1_1_2_001_8.dat` (host~1e-3, guest to ~4e-3, 6 signal series,
German decimal comma).

Ruled out so far:
- Speciation solver robust to lg β=18 (residual 1e-13); classic model fits this data cleanly.
- `m_defined_model` IS copied to clones (`AbstractModel.cpp:137`) → clone structure is correct.
- Opening a classic 1:1/1:2 project **fixture** (`data/samples/reference/simulated_1_1_1_2.json`) in
  the GUI does NOT crash. The synthetic 1:1/1:2 fit test does NOT crash. So it needs the **MC** step.

### ASan investigation (2026-07-12) — headless core is CLEAN; bug is GUI-side
Built an ASan tree `build-asan/` (RelWithDebInfo, `-fsanitize=address -O1 -g`, `ML_NEURAL_NETWORKS=OFF`,
`USE_PCH=ON`) and a headless repro `src/tests/test_mc_crash.cpp` (McCrashTest): fit `nmr_any` 1:1/1:2
(6 series, Gaussian-noised → real residuals) → full Monte Carlo (JobManager, SEy, 60–100 steps,
2–3 threads) → create+fit a classic `nmr_ItoI_ItoII` on the same `DataClass`. Ran under ASan in **both**
setups: legacy `MaxA/MaxB` grid AND the reaction-editor path (`Reactions: A + B <=> AB\nA + 2 B <=> AB2`,
which uniquely calls `BuildSpeciationFromReactions`/`UpdateComponentHeaders`).
**Result: SURVIVED both, ZERO ASan errors.** So the corruption is NOT in the headless core path.

Additionally ruled out by reading the code:
- MC clones **share** the `DataClass` private (`d = other->d`) but `Override*` calls `d.detach()` first
  → deep-copies when refcount>1, isolating the clone. The original/`data` tables stay pristine.
- MC table cleanup is `QPointer`-guarded (`~DataClassPrivate`, `m_ptr_table`) → the generic double-free
  is defended.
- MC clones are constructed on the **main thread** (`setModel`→`Clone` in `GenerateData` before
  `threadpool->start`) → no clone-construction race.
- No mutable static/shared state in `bfgsconcentrationsolver`/`speciationengine`/`reactionparser`;
  `nmr_any` does not override `PreventThreads`.

So the bug lives in the **GUI layer**, prime suspects the new `nmr_any`-specific widgets:
`DynamicModelWidget` (live `QTransposeProxyModel`/`QTableView` over `GlobalTable()`/`LocalTable()`,
`reset()` on `Recalculated`), `ReactionEditorWidget`, and the MC-results / model-widget / chart
lifecycle in `ModelDataHolder::ActiveModel`.

**Next step (in progress): GUI + ASan.** Building `suprafit` in `build-asan/`. Operator reproduces with
the real workflow (reaction editor + SEy MC); ASan prints the exact corrupting write + allocation stack.
Build fix required en route (committed): `core` now links `fmt::fmt-header-only` (CMakeLists.txt ~272) —
`analysis_manager.cpp`/`projectmanager.cpp` use `fmt::print` but `core` linked no fmt, so their objects
emitted an external `fmt::vprint` reference nothing resolved on a clean build (worked only via stale
objects). `src/tests/test_mc_crash.cpp` is a **temporary** harness (remove once the crash is fixed).

## Follow-ups the user raised (not done)
- `itc_any` InitialGuess uses `K+K` for higher species — same runaway risk; could reuse a GuessLgBeta
  analogue (itc doesn't derive from AbstractTitrationModel).
- Fit-solver **variable projection**: optimise only the nonlinear lg K, project out the linear
  shifts/ε (they are already refit via QR in `UpdateShifts`) — the main remaining fit speedup.
- GUI **config** option to pick the speciation method per model (call + benchmark flag exist; GUI
  option not yet).
- "nmr_any slower than the classic model" — expected (general iterative vs specialised solver).

## Roadmaps (from sub-agents, committed)
`roadmap/python_interface.md` (recommend JSON/CLI wrapper first, then pybind11; drop embedding),
`roadmap/scriptmodel_performance.md` (found: ExprTk never binds local params; per-point map lookups;
dead ChaiScript/Duktape/Python backends).

## 2026-07-12 (session 2) — series-toggle fix, CI, and two GUI findings

**Fixed — model line/error series hidden until manual toggle.** `ModelElement::DisableSignal`
(`src/ui/widgets/modelelement.cpp:251`) passed a plain 0/1 to `LineSeries::showLine(int)`, which treats
its argument as a `Qt::CheckState` and only shows on `== Qt::Checked (2)` (`external/CuteChart/src/
series.cpp:94`). The load path (ctor `toggleActive`, `Update`, reactive binding) drives it with 1, so
`showLine(1)` emitted `visibilityChangeRequested(false)` → fit + error line-series hidden; the manual
checkbox delivers a real `stateChanged(2)`. Fix: pass a `bool` (`state != 0`) so the `showLine(bool)`
overload runs. (The raw-data scatter showed because its visibility is set directly, never via the buggy
`showLine(int)`.)

**CI (`.github/workflows/ccpp.yml`).** Now also builds `feature/**` branches (was master-only). The
release job publishes per-branch: master → the shared `nightly` prerelease; a feature branch → its own
`ci-<branch>` prerelease (public, downloadable AppImage/tar.gz/zip/dmg for others to test) so it never
clobbers the master nightly. PRs still build-only. (Submodule consistency — the orphaned
`external/least-squares-cpp` gitlink that had broken `checkout --recurse-submodules` — is already
resolved, TECHNICAL_DEBT D6.)

**Open finding — two windows show the same projects (TECHNICAL_DEBT D9).** `SupraFit::ProjectManager` is
a process-wide singleton. Two halves: (1) `ProjectTree::getUnifiedProjectList()` (`projecttree.cpp:31`)
reads `instance().getLoadedProjectIds()` with no per-window filter; (2) every `SupraFitGui` ctor
connects to the same singleton's signals (`suprafitgui.cpp:175`), so a second window (`NewWindow()` →
`new SupraFitGui`, `suprafitgui.cpp:904`) both displays all projects and reacts to the other's loads.
Recommended fix (Rank 1): make `ProjectManager` instantiable, one instance per `SupraFitGui`, keep
`instance()` for CLI/tests; thread a `ProjectManager*` through `ProjectTree`/`MainWindow`/`ModelDataHolder`
(they hard-code `instance()` at `mainwindow.cpp:138`, `modeldataholder.cpp:485,788,880,1354`). Keep
app-level `qApp` properties (threads, settings) shared. Rank 2: owner-window-tagged singleton + per-window
filter. Rank 3 (stopgap): tree reads per-window `m_data_list` — fixes only the duplication, not the
signal fan-out. Not implemented (bigger change, touches CLI-shared code — needs a go-ahead on direction).
