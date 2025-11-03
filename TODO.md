# SupraFit Development TODO - UI & Core Refinements

## Quick Wins (Low Effort, High Impact)
- [ ] Make printout precision configurable
- [ ] Replace deprecated Qt functions
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

## Completed: ProjectManager Integration (January 2025)
✅ **Phase 1 of CLI_UI_Consolidation**: ProjectManager fully implemented
- Singleton for centralized project handling
- Unified file I/O logic (CLI + GUI)
- UUID-based project tracking with caching
- Thread-safe operations with signal notifications

---

## Key Documentation
- `docs/UI_IMPROVEMENT_PLAN.md` - Main UI strategy (4 phases)
- `docs/REFACTORING_MAIN_WINDOW.md` - MainWindow refactoring
- `docs/REFACTORING_DIALOGS.md` - Dialog improvements
- `docs/CLI_UI_CONSOLIDATION_PLAN.md` - Architecture consolidation
- `src/client/usage_example.md` - CLI documentation
