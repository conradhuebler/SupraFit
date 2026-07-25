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
