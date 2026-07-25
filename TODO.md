# SupraFit Development TODO - UI & Core Refinements

## Quick Wins (Low Effort, High Impact)
- [ ] Make printout precision configurable
- [ ] Replace deprecated Qt functions
- [x] **COMPLETED (Nov 3 2025)**: Remove deprecated SetData() from MainWindow
  - [x] Removed legacy `SetData()` method completely
  - [x] Updated header - removed deprecated declaration
  - [x] All GUI code now uses `setDataFromProjectManager()` instead
- [ ] Remove dead code from `SupraFitGui`:
  - [ ] Remove `m_hasData` member
  - [ ] Remove `m_logfile` and `m_printlevel` members
  - [ ] Remove unused `LogFile()` function
  - [ ] Remove commented-out `EditData()` slot
  - [ ] Remove `m_model_toolbar` and `m_system_toolbar` (never added to window)

---

## Phase 1: UI Toolbar & Menu Refactoring (High Priority)
**Reference**: `docs/UI_IMPROVEMENT_PLAN.md` (Phase 1)
**Impact**: High usability improvement, moderate complexity

- [ ] Create `MenuManager` class for hierarchical menu building
  - [ ] Implement `Data` menu (Edit, Split actions)
  - [ ] Implement `Model` menu with `Experiment` submenu hierarchy
    - [ ] NMR, ITC, UV/VIS, Fluorescence submenus
  - [ ] Implement `Evaluation` menu (Statistics, Analyse, Compare)
- [ ] Remove old toolbar buttons from `MDHDockTitleBar` and `SupraFitGui`
- [ ] Keep minimal essential toolbar (New, Open, Save, Optimize All only)

---

## Phase 2: Log & Project View Consolidation (High Priority)
**Reference**: `docs/UI_IMPROVEMENT_PLAN.md` (Phase 2)
**Impact**: Better debugging visibility, low risk

- [ ] Create `LogWidget` class (QPlainTextEdit or QTextBrowser)
- [ ] Refactor left dock widget to `QTabWidget`
  - [ ] Tab 1: Projects (existing `m_project_view`)
  - [ ] Tab 2: Log (new `LogWidget`)
- [ ] Redirect message/warning/info signals to new `LogWidget`
- [ ] Remove `MessageDock` from status bar

---

## Phase 3: ModelWidget UX Enhancements (Medium Priority)
**Reference**: `docs/UI_IMPROVEMENT_PLAN.md` (Phase 3)
**Impact**: Better fit result presentation, medium complexity

- [ ] Implement read-only mode for converged fits
  - [ ] Check `m_model->isConverged()` in `ModelWidget::Repaint()`
  - [ ] Set spinboxes to read-only when converged
  - [ ] Visual indication (background color or disabled state)
- [ ] Improve local parameter display
  - [ ] Refactor `ModelElement` to use `QTableView` with custom model
  - [ ] Add columns: Parameter Name, Value, Std. Error
  - [ ] Each row = parameter for data series ("Datenreihen")
  - [ ] Clear "Ergebnisse des Fits" (fit results) presentation

---

## Phase 4: Dialog Improvements (Medium Priority)
**Reference**: `docs/REFACTORING_DIALOGS.md`
**Impact**: Consistent UI, improved UX

- [ ] Create base `Dialog` class for consistency
- [ ] Refactor `ImportData` dialog (plugin-based file format support)
- [ ] Improve `ResultsDialog` (table/chart display with filtering)
- [ ] Improve `StatisticDialog` (info display + input validation)

---

## Phase 5: Core Architecture - Task Execution (Medium Priority)
**Reference**: `docs/CLI_UI_CONSOLIDATION_PLAN.md` (Phase 2)
**Impact**: Code consistency, reduces bugs

- [ ] Create `TaskController` class in `src/core/`
  - [ ] Encapsulate `JobManager` and `Minimizer`
  - [ ] Methods: `runFitting()`, `runStatisticalAnalysis()`
  - [ ] Progress and completion signals
- [ ] Refactor `ModelWidget` to use `TaskController`
- [ ] Simplify `SupraFitCli` task execution

---

## Phase 6: Data Generation Unification (Low Priority)
**Reference**: `docs/CLI_UI_CONSOLIDATION_PLAN.md` (Phase 3)
**Impact**: Consistency, single source of truth

- [ ] Create `DataFactory` class or enhance `ProjectManager`
  - [ ] Implement `generateProject(const QJsonObject& config)`
- [ ] Refactor `ImportData` dialog to use unified method
- [ ] Simplify `SupraFitCli::GenerateData()`

---

## Phase 7: Performance & Cleanup (Low Priority)
**Reference**: `docs/REFACTORING_MAIN_WINDOW.md` (Phase 4-5)
**Impact**: Better responsiveness, maintainability

- [ ] Optimize project storage (QHash/QMap for UUID lookup)
- [ ] Implement lazy loading for large projects
- [ ] Optimize view updates (only changed parts)
- [ ] Use smart pointers in `SupraFitGui` and `ModelWidget`
- [ ] Add unit tests for core components

---

## General Code Improvements
- [ ] More robust quantile function (see https://octave.org/doc/v4.0.1/Descriptive-Statistics.html#XREFquantile)
- [ ] More efficient Model Comparison algorithm for many parameters (consider genetic algorithms)
- [ ] Add comprehensive documentation to complex classes
- [ ] Implement timing analysis for heavy operations

---

## SupraFit 3.0 Beta

### Fit results and reference data
- [ ] **2:1/1:1 cross-validation mean deviates from the reference by 22 %** (`ReferenceProjectsTest::referenceResampleCVRA`, `simulated_2_1_1_1.json`, ¹H 2:1/1:1, p6: 4.65-4.67 vs 5.96). The CV block is exhaustive leave-one-out (19 points, no sampling), so the deviation is reproducible. Open: which of the 19 fits deviates, and whether the reference or the recomputed value is the correct one. Deliberately not blocking the scripted-model merge.
- [ ] **The same CV mean jitters by ~2 % between runs** (4.645 / 4.666 / 4.738 over three) — unexpected for exhaustive leave-one-out; the test sets `threads=4`, so start at the JobManager's result ordering.
- [ ] Regenerate the 2:1/1:1 reference data so the stored SSE matches the closed-form cubic root (`referenceFit` is 0.13 % off at a 0.1 % tolerance — the known, deliberate consequence of the solver change).

### Solver defaults
- [ ] **Decide the default fit solver.** `OptimConfigBlock` ships `FitSolver = "LevMar"` (`global.h`); VarPro is opt-in although the fixed NMR/UV-VIS models support it (conditionally on their options) alongside the `*_any` models. Equivalence is covered (`test_varpro`, `test_varpro_cv`) and VarPro is 2-5x faster. **`benchmark_dimer_flat`, measured 2026-07-24 (release, 6 configurations): neither solver wins throughout, so a blanket switch is not supported.** VarPro recovers the truth exactly in 3 of 6 but returns lg β12 = -7.32 / 0.15 in the two runs started at lg β(A2) = 5, at an SSE (1e-3) that noisy real data would hide; LevMar sits on its start value in 2 of 6 (SSE 2.5 and 11.1). One adversarial scenario — enough to reject the blanket switch, not enough to conclude the opposite.
- ✅ **`isConverged()` fixed (2026-07-24).** It used to read backwards in both failure modes — LevMar reported converged while sitting on its start at SSE 11.1, VarPro reported *not* converged where it landed exactly on the truth. The benchmark numbers quoted above come from that run; SSE and parameters were unchanged by the fix, only the flag.
- [ ] Worth trying instead of one default: VarPro to reach the valley, then a full-vector LevMar polish — VarPro's failures are wrong parameters at a plausible SSE, LevMar's are visible non-movement.
- [ ] If flipped after all: choose between `VarPro` and `VarProAnalytic` (the latter falls back to finite differences on masked data), keep `LevMar` reachable as the reference oracle the equivalence tests compare against, and check what stored projects do — their own optimizer config must keep winning over the new default.

### ITC thermogram — GUI verification still outstanding
The thermogram dialog was routed through the core `ItcProcessor` and merged without these checks: the
rendering path has no test harness, and the core pin test (`testAbsoluteIntegralsPinned`) covers only
the integrals. Compare against `bugfix/thermo-nmr-chart-fixes`, the frozen starting point — not
against `master`, which did not have the processor.

- [ ] Load an `.itc` experiment: column 0 shows the file's injection volumes.
- [ ] Load an experiment, then a dilution: column 0 and the four concentration fields stay unchanged.
- [ ] Column 3 against the baseline, with and without dilution. It now shows *more* digits — the GUI
      used to round to 6 significant digits while the CLI always read full precision. Expected.
- [ ] Clear the dilution field: column 3 falls back to the experiment values. **This did not happen
      before — deliberate change, confirm it is wanted.**
- [ ] Import a comment-only file via "Import Row": warning instead of a crash.
- [ ] `.dh` and `.dat` export: byte-identical before and after.
- [ ] Dilution series in the chart: appears only with a dilution loaded, "Show Dilution" works.
- [ ] **Decide F5:** `Raw()` used to write the dilution file path unconditionally, so with
      `StoreFileName` off the dilution kept a path while the experiment did not. Both are symmetric
      now (no path written in that mode). Revertible if the asymmetry was wanted.

### ITC thermogram — libpeakpick maths ("Track B", own branch, changes numbers)
- [ ] **B0 Decouple Eigen** (`CMakeLists.txt:210`) — blocks every structural change below.
- [ ] **B1 `Peak.end` convention** — SupraFit builds the range inclusive, libpeakpick reads it exclusive; the core of the integration discrepancy.
- [ ] **B2 Undefined behaviour → defined** (`spectrum.h:170-176`, `:198-204`: the `i >= size() && i < 0` guard is never true).
- [ ] **B3 Number-changing corrections, one commit each with the measured delta** (`baseline.h:484`: `|| gradient - 1` is always true).
- [ ] **B4 Hygiene** — `spectrum` rule-of-zero (a user destructor plus copy operators suppress moves, so every copy is deep).
- [ ] Deferred with a FIXME in the code: `nxlinregress.h:56` uses `initial[i]` where `initial[j]` is meant.
- Guard for all of the above: `testAbsoluteIntegralsPinned` pins the 20 unscaled integrals at 1e-9 (the OpenMP reduction scatters in the last bit, ~1e-16). A moved number needs a reason, not a new expectation.

### itc_any
- [ ] **`itc_any` fits run away from a correctly scaled seed** — recorded on 2026-07-17, separate from
      the seeding fix and not caused by it.

---

## Completed Tasks

### ✅ ProjectManager Integration (January 2025)
**Phase 1 of CLI_UI_Consolidation**: ProjectManager fully implemented
- Singleton for centralized project handling
- Unified file I/O logic (CLI + GUI)
- UUID-based project tracking with caching
- Thread-safe operations with signal notifications

### ✅ CLI ProjectManager Migration & GUI Cleanup (November 3, 2025)
**Combo-Paket Completion**: Core integration and cleanup
- **Phase 1**: CLI ProjectManager Migration
  - Migrated `PrintFileStructure()` to use ProjectManager API
  - Converted 24 TODO comments to MIGRATION POINT markers for future work
  - Foundation laid for deeper CLI refactoring
- **Phase 2**: Removed deprecated GUI methods
  - Deleted unused `SetData()` method from MainWindow
  - Enforced use of `setDataFromProjectManager()` throughout GUI
  - Clean code removal without breaking references
- **Phase 3**: Enhanced AnalysisManager error handling
  - Added try-catch blocks around JobManager operations
  - Implemented error signal emission for debugging
  - Graceful error recovery: skips failed methods, continues processing
- **Build verified**: Clean compilation, CLI & GUI both functional

---

## Key Documentation
- `docs/UI_IMPROVEMENT_PLAN.md` - Main UI strategy (4 phases)
- `docs/REFACTORING_MAIN_WINDOW.md` - MainWindow refactoring
- `docs/REFACTORING_DIALOGS.md` - Dialog improvements
- `docs/CLI_UI_CONSOLIDATION_PLAN.md` - Architecture consolidation
- `src/client/usage_example.md` - CLI documentation
