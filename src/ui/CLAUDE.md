# UI - SupraFit Graphical User Interface

## Overview
Complete graphical user interface for SupraFit providing intuitive access to all analytical capabilities. Built with Qt6, offering modern UI components for data visualization, model fitting, and result analysis.

## Core Components

### Main Application
- **main.cpp**: GUI application entry point
- **mainwindow/suprafitgui.cpp/h**: Primary application window and controller
- **mainwindow/mainwindow.cpp/h**: Main window implementation with menu system

### Data Management
- **mainwindow/datawidget.cpp/h**: Data table display and editing
- **mainwindow/projecttree.cpp/h**: Project hierarchy and navigation
- **mainwindow/modeldataholder.cpp/h**: Model-data association management
- **mainwindow/editdatacontroller.cpp/h**: `EditDataController` — Edit Data of thermogram/spectra/NMR projects (dialog → `ImportTableData`/`getStoredData` → `checkReplacement` → `replaceProjectData`, incompatible question, stop jobs, undo); dialog/choice/confirm handlers injectable (`test_edit_data`); `ModelDataHolder::ApplyDataReplacement` closes dropped panes and refreshes the rest
- **guitools/outdatedbanner.cpp/h**: `OutdatedBanner` — "This model was fitted to earlier data." + Fit again / Mark as current in model and meta-model panes; `ModelWidget` keeps `"outdated"` in `m_last_model` so Restore/LoadJson never clear the mark

### Visualization
- **mainwindow/chartwidget.cpp/h**: Main charting and plotting interface
- **widgets/modelchart.cpp/h**: Model-specific visualization components
- **guitools/chartwrapper.cpp/h**: Chart abstraction and utilities

### Model Interface
- **mainwindow/modelwidget.cpp/h**: Model parameter configuration and fitting
- **mainwindow/metamodelwidget.cpp/h**: Meta-model and comparison interfaces
- **widgets/modelelement.cpp/h**: Individual model component representation

## Dialog System

### Data Import/Export
- **dialogs/importdata.cpp/h**: Data file import wizard
- **dialogs/spectraimport.cpp/h**: Spectral data import specialized interface; `ImportKeyGuard` + `StatusLine` (OK off without spectra or x values); test `test_spectraimport` (also the "Open Spectra"/"Open NMR Spectra (Tech Preview)" entries of `SupraFitGui`)
- **dialogs/nmrimport.cpp/h**: NMR titration import (renders `NmrHandler`: spectra list, trace editor, signals, δ table with status colours, concentrations); opened by "Import as NMR Spectra (Tech Preview)" (`ImportData`) and Edit Data of a Spectrum project whose raw block is NMR; smoke test `test_nmrimport`
  - Tech Preview label (entries, title, tooltips, `nmrTechPreview` line): remove together, see TODO.md
  - JEOL: group "Processing (FID)" (`nmrFidPanel`, LB, zero fill, ph0/ph1, Auto phase, Auto phase each spectrum) visible only with FIDs; whole real series visible offscreen since the display envelope (series above 5000 points switch to OpenGL, offscreen empty)
  - WP4: "Shift from" peak picking / lineshape fit with options; "Fit details" = 5 columns per signal (only the δ column is editable); model (dashed) + residual of the selected spectrum are drawn after `chartUpdated`
  - Viewer 2: markers/fit/residual drawn with `stackOffset(k)`; hint in `nmrStatus` + cell tooltips when a signal is missing in every used spectrum (`m_missing_hint`; text by threshold: whole spectrum names "search window" first, search window only window/range)
  - "Detection threshold" (`nmrThresholdScope`, `NmrThresholdScope`): a new series starts with "search window" unless memory `thresholdScope` says otherwise; a project's value wins; disabled with a PickPeaks step
  - UX 1: `ImportKeyGuard`, readiness `StatusLine` "nmrReady" on both pages, "Double-click adds the signal" (QSettings `nmrimport/doubleClickAdds`), destructive actions ask via `keyGuard()->confirm()`, `AdaptiveWaiter` around tracking/FID
  - UX 2a: `SeriesSourceBar`, `ConcentrationEditor`, fit options behind `MoreLessToggle` (opens for lineshape fit); `restoreOptions()` (memory) runs before `setData()`, so a project's values win; save only in the `m_updating`-guarded apply paths
  - Viewer fixes: pick in `pickRange()` (Start..End ∩ visible axis), double-click → `NmrHandler::addSignalAtPeak`, signal labels (`setLabels`, row = number), chart x minus `stackOffset(ref).x()`; LB/ph0/ph1 debounced 300 ms when slow, `handler()`/`accept()` flush; marker/fit series reused; timing tool `benchmark_nmrimport`
  - Multiplets: signal table columns Name/Type (combo cell widget)/δ/±/½ span/Pattern, "Multiplet from range" (`pickRange`, ≤ `kMaxMultipletRange`), group "Multiplet options" (`nmrMultipletOptions`, MoreLessToggle: grouping J in Hz, MHz from the FIDs, ripple, centre); `TraceEditorWidget::Label::ticks` draws the bracket
- **dialogs/thermogram.cpp/h**: Thermogram data handling; renders `ItcProcessor`; smoke test `test_thermogram_dialog` (shown/handed-over table = core result, uniform volume, clearing the dilution, `setRaw`)
  - `setScalingFactor(cal2joule)` is seeded at the end of `setUi()`: it renders, so the table and series must exist

### Analysis Configuration
- **dialogs/generatedatadialog.cpp/h**: Data generation parameter setup
- **dialogs/parameterdialog.cpp/h**: Model parameter configuration
- **dialogs/configdialog.cpp/h**: Application settings and preferences

### Results and Analysis
- **dialogs/resultsdialog.cpp/h**: Fitting results presentation
- **dialogs/statisticdialog.cpp/h**: Statistical analysis and validation
- **dialogs/comparedialog.cpp/h**: Model comparison and selection
- **dialogs/regressionanalysisdialog.cpp/h**: Regression analysis tools

### Advanced Tools
- **widgets/scanwidget.cpp/h**: the Scan (grid of start values, job of the global search); tab "Scan" of `StatisticDialog`; **dialogs/advancedsearch.cpp/h** is its modal wrapper (meta model)
- **widgets/analysisparts.cpp/h**: `ParameterSelector` (parameters to test, project dialog resolves per model) and `AddFolded` (`..more..`)
- **dialogs/modaldialog.cpp/h**: Generic modal dialog framework
- **dialogs/genericwidgetdialog.cpp/h**: Reusable widget containers

## Widget Library

### Input Controls
- **widgets/buttons/scientificbox.h**: Scientific notation input
- **widgets/buttons/spinbox.h**: Enhanced numeric input
- **widgets/buttons/hovercheckbox.h**: Interactive checkbox with hover effects

### Data Widgets
- **widgets/DropTable.h**: Drag-and-drop data table interface
- **widgets/parameterwidget.cpp/h**: Parameter input and validation
- **widgets/systemparameterwidget.cpp/h**: System-level parameter configuration

### Specialized Components
- **widgets/spectrawidget.cpp/h**: spectra import (spectra list, x values, VarCovar, concentrations) on the trace editor
- **widgets/traceeditorwidget.cpp/h**: `TraceEditorWidget` — chart (raw/processed, x direction by `setAxisDirection`), range, editable `TraceOpList`; renders only, host stores the list; `chartUpdated()` lets a host add series after a redraw; smoke test `test_traceeditor`
  - Hint line `traceEditorHint` (mouse actions + host text `setDoubleClickHint`), shown with setting `thermogram_guidelines` ("Show hints in the import dialogs"; key kept)
  - View (Viewer 2): user zoom (`m_user_view`) kept over redraws of the same trace ids, moved with a whole-axis shift; wheel/middle button handled in the viewport event filter (`scaleY`, `resetView`), not by CuteChart's `scaleUp/scaleDown` (cursor-based, ITC only)
  - Viewer 3: wheel = intensity factor (`scaleIntensity`, ×1.25/notch, 0.01..1e6, never stored), Ctrl+wheel = `scaleY`; host series go through `displayPoint(k,x,y)` = level + f·y; scaling keeps the axes (no user view, see below), fit y/middle click/new ids → 1
  - Stack (view only): `stackOffset(k)` for host series, `setExcludedTraces`, `setHighlightedTrace`, QSettings `traceeditor/<setSettingsKey>/stackY|stackX|stackReverse`; offsets move points by `replace()`, no rebuild (15 × 131072: 16-22 ms release, rebuild 140-160 ms)
  - Display envelope: traces > `kDisplayPointLimit` drawn from the visible slice, min/max per pixel column (≤ `kMaxEnvelopeColumns`, below CuteChart's OpenGL switch at 5000), recomputed on axis change; series reused when the structure is unchanged (a new series ≈ 8 ms)
  - Auto vs user view: the wheel never sets `m_user_view`; without it, scaled intensities keep `m_auto_*` and a stack change sets y to the nice range at factor 1; labels (`setLabels`, `labelClicked`) are chart items placed in `updateLabels()`
  - `setAxisDirection(Auto|Reversed|Normal)`: Auto = reversed for descending data (default, UV/Vis); `NmrImport` sets Reversed/Normal by `nmrReverseAxis` (QSettings `nmrimport/reverseAxis`); test `test_chartzoom` (ChartZoomTest, real mouse events)
- **widgets/thermogramwidget.cpp/h**: Thermogram display and analysis; `setUi()` = `setupChart`/`setupPeakLists`/`setupControls`/`setupParameterPanel`/`restoreSettings`/`connectControls`, order matters (connections)
  - Not on `TraceEditorWidget`: needs zoom-by-default, select only in peak edit mode, persistent series, keys/double click/scale — see roadmap WP5
- **widgets/exportsimulationwidget.cpp/h**: Simulation export configuration

### Results Display
- **widgets/results/resultswidget.cpp/h**: Main results presentation
- **widgets/results/mcresultswidget.cpp/h**: Monte Carlo results visualization
- **widgets/results/scatterwidget.cpp/h**: Scatter plot analysis
- **widgets/results/searchresultwidget.cpp/h**: Search results presentation

### Optimization Interface
- **widgets/optimizerwidget.cpp/h**: Optimization algorithm configuration
- **widgets/optionswidget.cpp/h**: General options and settings
- **widgets/preparewidget.cpp/h**: Data preparation and validation

## User Interface Features

### Modern Qt6 Interface
- Responsive layout system with FlowLayout
- High-DPI display support
- Theming and customization options
- Keyboard shortcuts and accessibility

### Data Visualization
- Interactive plotting with zoom and pan
- Multiple chart types (scatter, line, contour)
- Real-time data updates during fitting
- Export capabilities (PNG, SVG, PDF)

### Project Management
- Hierarchical project organization
- Session management and auto-save
- Undo/redo functionality
- Project templates and examples

### Model Integration
- Visual parameter adjustment with sliders
- Real-time model preview
- Parameter correlation visualization
- Confidence interval display

## GUI Tools and Utilities

### Layout Management
- **guitools/flowlayout.cpp/h**: Responsive flow-based layouts
- **guitools/flowlayout.cpp/h**: Custom layout algorithms

### Interaction Tools
- **guitools/mime.h**: Drag-and-drop MIME type handling
- **guitools/waiter.h**: Progress indication and user feedback; `AdaptiveWaiter` = wait cursor only if the same work took > 0.3 s last time
- **guitools/importkeyguard.cpp/h**: `ImportKeyGuard` — Enter never presses OK, dirty after the first Show, `confirmDiscard()` from the host's `reject()`, `setConfirmHandler` for tests; not used by the ITC dialog (own rule)
- **guitools/statusline.cpp/h**: `StatusLine` — one elided line, Info/Warning/Error in palette-safe colours, full text as tooltip
- **guitools/dialogmemory.cpp/h**: `DialogMemory` — QSettings `dialogs/<name>/...` of exec() dialogs: geometry as QRect (applied only on a present screen, not below the minimum size), splitters, checked readers that ignore wrong types/out-of-range values
- **guitools/seriessourcebar.cpp/h**: `SeriesSourceBar` — Open directory/File ending/Load file/Add files/path/count + "n file(s) read, m not read"; drops of files/directories via an application event filter (children that take text drops would swallow them)
- **widgets/concentrationeditor.cpp/h**: `ConcentrationEditor` — `DropTable` + paste hint + "Load from file" (`FileHandler`, as Open Project) + "n rows for m spectra"; `Edited()` on paste/load/clear/setTable
- **widgets/messagedock.cpp/h**: Message logging and status display

### Chart Integration
- **guitools/chartwrapper.cpp/h**: Unified charting interface
- **widgets/niceticks.cpp/h**: Automatic axis scaling and formatting

## Current Implementation Status

### ✅ Core Functionality
- Complete data import/export workflows
- Model parameter configuration and fitting
- Results visualization and analysis
- Project management and persistence

### 🔧 Key Integrations
- **CLI Integration**: Seamless data exchange with command-line tools
- **Model System**: Direct access to all analytical models
- **File Formats**: Support for multiple data formats
- **Export Capabilities**: Publication-ready output generation

## Dependencies
- **Qt6**: Widgets, Charts, Qml modules
- **Core libraries**: Complete SupraFit core functionality
- **Visualization**: Qt Charts and custom plotting components

## Usage Patterns

### Typical Workflow
1. Import data via dialogs/importdata
2. Configure model parameters in modelwidget
3. Perform fitting and optimization
4. Analyze results in results widgets
5. Export publication-ready figures

### Advanced Features
- Monte Carlo error analysis
- Global parameter search
- Model comparison and selection
- Batch processing capabilities

---

## Variable Section (Short-term information, regularly updated)

### Recent UI Updates
- 2026-11-07: 🤖 **Polish is the cost of a model pane** — a polished widget moved under a parent with another style is polished again with all its children; the model tabs carry a style sheet, so each pane (~1000 widgets) is polished at `addModelsTab` a second time. Moving that sheet or building in place changed pixels: see [docs/modelwidget_load_2026-11-07.md](../../docs/modelwidget_load_2026-11-07.md). Check pane changes with `SUPRAFIT_LOAD_GRAB`.
- 2026-11-07: 🤖 **Teardown order** — a widget whose children signal into it (tab widgets, project windows, docks) takes them down in its own destructor; left to `~QWidget` they call a half-destroyed object. Check with UBSan and `teardownAfterOpeningFiles` (one file per process).
- 2026-11-04: 🤖 **Opening a file** — `SupraFitGui::BeginLoad`/`EndLoad` keep the wait cursor until the queued window building is done (the load report comes after it); status-bar progress only after 300 ms, never while `loadProject` holds the manager lock (a status bar that appears lays out the project tree, which locks it: deadlock). Measure with `benchmark_projectload` (`LoadTiming` scopes), never by feel.
- 2026-10-24: 🤖 **Landscape Sampling is an analysis of the Statistics page** — tab in `StatisticDialog` (`LandscapeWidget`, `RunLandscape`; disabled without `SupportsVarPro()`), listed by `ResultsDialog`, shown by `ResultsWidget::LandscapeWidget` (profile chart relative to the fit) with the text of `LandscapeSearch::Summary`; test `landscapeSamplingIsStartedListedAndShown`. One chart with a `Show:` choice (Profiles / Profile: x / Surface: x × y, the surfaces from `surfaces` of the result: height lines by marching squares joined into polylines, axes follow the lines; checkbox "2D surfaces of the pairs"); settings page: few fields in sight, the rest behind `..more..`, Langevin's only for Langevin; the task "Find other minima" (mode `Minima`) swaps the confidence for the wall and lists the minima with `Load into model` / `Add as model`; `StatisticDialog::Stage` shows the stage of a running analysis above the bar (no log, no tray). Plan: [roadmap/landscape_sampling.md](../../roadmap/landscape_sampling.md).
- 2026-10-17: 🤖 **Chat dock and endpoint manager come from the shared CuteChat library** (`<cutechat/chatdock.h>`, `<cutechat/endpointdialog.h>`); SupraFit supplies `widgets/suprafitchathost.{h,cpp}` (prompt, tools, project-file storage, wording). Render of the dock unchanged (pixel-identical offscreen, light/dark). See [roadmap/shared_chat_library.md](../../roadmap/shared_chat_library.md).
- 2026-10-15: 🤖 **Main window redesign** (branch `feature/ui-redesign`, [roadmap/ui_redesign.md](../../roadmap/ui_redesign.md)) — one toolbar row with Data/Model/Evaluation menus of the project on screen (`MDHDockTitleBar` owns menus, not buttons), Log under the project list, model pane = header (name, fit row, parameters, actions) + tabs/data series + fit results.
- 2026-10-15: 🤖 **Colours come from the palette** — `guitools/appearance.{h,cpp}` sets System/Light/Dark (setting `appearance`) and restyles every QChartView on show; use palette roles, and `Appearance::ContrastText()` for text on meaningful colours. No window-wide style sheets.
- 2026-07-21: ✅ **MC results widget 15x faster** — the cost was never the chart/render chain (30 ms) but `WriteConfidence` → BC50 integration; see AIChangelog. Measure this path with `benchmark_mcwidget` (offscreen, `src/tests/`), never by feel.
- 2026-07-20: ✅ **Scripted-model form derives its parameters** — `PrepareWidget::DeriveDefinition()` reads inputs/globals/counts out of the equation (`ExprTkEngine::CollectSymbols`); only "Names of local parameters" stays user-owned. The other four fields are read-only *(derived)*.
- 2026-07-20: ✅ **`FieldValue(name)` is the single reader for definition fields** — the model-definition tab filters out every `"once"` descriptor, so reading the visible boxes reported the model's own symbols as undefined (ERR238). Never scan `m_stored_objects` for a field value.
- 2026-07-20: ✅ **Dialog close crash fixed** — `~PrepareWidget` must block the boxes' signals before deleting the Highlighter; `~QSyntaxHighlighter` re-enters the form via `contentsChanged` and would call back into the half-destroyed object.
- 2026-07-20: ✅ **2-column grid instead of FlowLayout** — 280x130 standard cell, wide editors span two; only the field grid scrolls, presets and preview stay put. Clear-all button; presets clear the form before filling.
- 2026-07-17: ✅ **Thermogram import hands over the DataTable** — `Thermogram::ResultTable()` passes `ItcProcessor::resultTable()` straight to `ImportData` instead of rendering `Content()` through `QString::number` (6 significant digits) and parsing it back with `FileHandler`; the CLI already read that table via `ReadITC()`, so GUI and CLI now agree. Both `ImportThermogram` overloads were identical from `exec()` on and share `RunThermogram()`; the leaked `FileHandler` is gone with them. `Content()` removed (no other caller).
- 2026-07-17: ✅ **Thermogram parked blocks removed** — `m_freq`, `m_scale`, `m_exp_base`, `m_dil_base`, `m_refit` were declared and compiled but only ever used inside comments. `m_scale` carried no scaling despite the name (cal→J goes through `setScalingFactor()`/`setScaling()`, as does the `"scaling"` JSON key), so stored projects are unaffected.
- 2026-07-16: ✅ **Thermogram dialog routed through core `ItcProcessor`** — the dialog renders the processor's (volume, net heat) result instead of recomputing the join; the processor owns the injection volumes, the shared cal→J factor and the exp−dilution gate. `UpdateTable()` is a pure renderer over `ResultRows()`; `Content()`/`Raw()` go through `resultTable()`/`toJson()`. "Force inject" is now an explicit "Uniform volume" checkbox; both scaling combos are coupled; clearing the dilution really stops the subtraction. **GUI paths need manual operator validation** (see `SESSION_HANDOFF_THERMO.md`).
- 2026-07-14: ✅ **Settings single source of truth** — `settingsregistry.{h,cpp}` (`SupraFitSettings::registry()`) holds every app setting (key, default, kind, group, label, tooltip, range, dependsOn). `SupraFitGui::ReadSettings/WriteSettings` derive persistence+defaults from it (the 49-entry `m_properties` list and the ~145-line default if-chain are gone); `ConfigDialog` is generated from it (tabs per group, `dependsOn` enable-chains, `Custom` kind for the hand-built directory section). New setting = one table row instead of four edits.
- 2026-07-14: ✅ **Thermogram per-injection volume editable** — column 0 of the import dialog table (`thermogram.cpp`) accepts manual edits (guarded by `m_updating_table`; heat columns read-only). Now persisted through `ItcProcessor::setInjectionVolume` (see 2026-07-16). `ThermogramWidget` scaling relabelled "cal → J" with a tooltip (leaving it at 1 for cal data is why results match calorie tools).
- 2026-07-14: ✅ **Thermodynamics unit toggle** — ConfigDialog checkbox "Report thermodynamics in kcal/mol" → `energy_unit_kcal` qApp property (persisted via `m_properties`); drives `Thermo::CurrentEnergyUnit()` used by `FormatThermo`/`statistic.cpp`. −TΔS row added to ITC output.
- 2026-07-13: ✅ **Speciation solver selectable in the GUI** — a "Speciation solver" submenu in `ModelWidget`'s Fit menu picks the equilibrium-concentration solver (LevMar/Newton vs. legacy BFGS) for the reaction-driven `*_any` models; writes the `SpeciationSolver` optimizer-config key (greyed out for closed-form models via `UsesSpeciationEngine()`, re-fits on select). `AbstractModel::setOptimizerConfig` is now virtual so the models push the method into their `SpeciationEngine`; `OptimizerWidget::Config()` round-trips the key.
- 2026-07-12: ✅ **Model series visible on load** — `ModelElement::DisableSignal` passed 0/1 to `LineSeries::showLine(int)`, which only shows on `== Qt::Checked (2)`, so fit + error line-series stayed hidden until a manual toggle (a real `stateChanged` delivers 2). Fixed by passing a `bool` (`state != 0`) → the `showLine(bool)` overload. `modelelement.cpp:251`.
- 2026-07-12: ✅ **`nmr_any` MC → add-model crash fixed** in core (`DataClassPrivate` `m_info` ownership); see AIChangelog / `SESSION_HANDOFF.md`.
- 2026-07-13: **`SpeciesEditorWidget` (type 5) removed** — the `*_any` models are now defined solely through the reaction-equation editor (type 6); the legacy grid/species-editor path is gone.
- 2026-07-13: ✅ **Fit-solver (LevMar/VarPro) selectable in the GUI** — `ModelWidget`'s Fit menu lists checkable LevMar/VarPro entries after a separator (VarPro greyed out when `!SupportsVarPro()`); writes the `FitSolver` optimizer-config key, which MC/CV/RA inherit via `Clone()`. `OptimizerWidget::Config()` now round-trips `FitSolver` (+ a combo in its General tab) instead of dropping the key on Apply.
- 2026-07-13: ✅ **Preset submenus for `itc_any` + new `fl_any`** — `MDHDockTitleBar::attachPresets` (`modeldataholder.cpp`) gained a `maxComponents` filter (`ReactionParser::Parse`, drops empty sections): `itc_any` gets a preset submenu limited to 2-component presets (ITC is host/guest from protocol); the new reaction-based `fl_any` gets the full N-component preset submenu (incl. three-component systems). `nmr_any`/`uvvis_any` unchanged.
- 2026-07-11: **Reaction-equation editor** (`widgets/reactioneditorwidget`) — live-parsed reaction text (arrow syntax) with component/species preview, wired as `PrepareBox` type 6.
- 2026-07-11: **Dynamic model widget** (`widgets/dynamicmodelwidget`) — scalable "Parameter Table" tab (QTableView over Global/Local tables) added in `modelwidget` for models with `UseDynamicParameterWidget()` (the `*_any` models); additive, classic view unchanged.
- Modern Qt6 migration completed
- Enhanced chart visualization capabilities
- Improved responsive layout system
- Better accessibility and keyboard navigation

### Current UI Status
- **Stability**: ✅ All major workflows functional
- **Performance**: ✅ Responsive for typical datasets
- **Usability**: ✅ Intuitive interface design
- **Compatibility**: ✅ Cross-platform deployment working

### Known UI Issues
- **Two windows show the same projects** (regression of the Jan-2025 ProjectManager consolidation).
  `SupraFit::ProjectManager` is a process-wide **singleton**; `ProjectTree::getUnifiedProjectList()`
  (`projecttree.cpp:31`) reads `instance().getLoadedProjectIds()`, and every `SupraFitGui` connects to
  the same singleton's signals (`suprafitgui.cpp:175`), so a second window (`NewWindow()` → `new
  SupraFitGui`) sees all projects and reacts to the other window's loads. Recommended fix: make
  `ProjectManager` instantiable and give each `SupraFitGui` its own instance (keep `instance()` for
  CLI/tests), threading a `ProjectManager*` through `ProjectTree`/`MainWindow`/`ModelDataHolder` (they
  hard-code `instance()` at `mainwindow.cpp:138`, `modeldataholder.cpp:538,857,949,1423`). Keep app-level
  `qApp` properties (threads, settings) shared. See `TECHNICAL_DEBT.md` D9.

### User Experience Priorities
1. Streamlined data import workflows
2. Enhanced real-time visualization during fitting
3. Improved parameter correlation displays
4. Better mobile/tablet compatibility

### Development Guidelines
- Follow Qt6 best practices
- Maintain responsive design principles
- Ensure accessibility compliance
- Provide comprehensive user documentation

### Performance Considerations
- Chart rendering optimized for large datasets
- Asynchronous operations for long-running tasks
- Memory management for visualization components
- Efficient data binding and updates

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks
- refactor the whole user interface, details will follow
  - **Plan the CLI/GUI de-duplication into this refactor** — see the *GUI-Overlap-Map* in
    `TECHNICAL_DEBT.md` §D3: the GUI duplicates job orchestration (vs CLI `PerformeJobs`) and
    results/statistics display (ignores `AnalysisManager`). The UI refactor is the natural lever to
    pull these onto one shared layer (start with a `TaskController` over `JobManager`). Affected CLI
    signatures are tagged `// REFACTOR(D3-…)` (`grep -rn "REFACTOR(D3" src/client`).
- if loading simulation files using DataGenerator, give comprehensive information and execute datageneration and open generated project file
- add widget to generate and control simulation input files
- ✅ **thermogram analysis moved to core** (2026-07-16) — `ItcProcessor` owns the ITC state + net-heat computation; the dialog renders it. C12 (DataTable handed over instead of the string hop) done 2026-07-17. Remaining: the libpeakpick math cleanup (Track B, changes numbers) — see `SESSION_HANDOFF_THERMO.md`.
- projecttree- logik sollte mit core/**Project Analysis Migration** synchronisiert werden
### Vision
<!-- Add long-term architectural goals here -->