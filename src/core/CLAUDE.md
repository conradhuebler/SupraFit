# Core - Fundamental SupraFit Components

## Overview
Core functionality providing the foundation for SupraFit applications. Contains data structures, file handling, mathematical operations, and the complete model system for supramolecular chemistry analysis.

## Core Components

### Data Structures
- **models/dataclass.cpp/h**: Main project container with metadata and system parameters
- **models/datatable.cpp/h**: Core data structure using Eigen::MatrixXd for numerical operations
- **models/AbstractModel.cpp/h**: Base class for all analytical models
  - **models/AbstractModel_serialize.cpp**: split-out JSON (de)serialization TU (`ExportModel`/`ExportStatistic`/`ImportModel`/`LegacyImportModel`, incl. legacy SupraFit 1/2 import); still `AbstractModel::` members, header/facade unchanged

### File Management  
- **filehandler.cpp/h**: File I/O operations with range selection capabilities (Claude Generated)
- **jsonhandler.cpp/h**: JSON serialization and deserialization for project files
- **thermogramhandler.cpp/h**: Specialized thermogram data processing (one per ITC trace)
- **itcprocessor.cpp/h**: GUI-free ITC import orchestrator (Claude Generated) — owns the experiment + optional dilution `ThermogramHandler`, the per-injection volumes and the shared cal→J factor; produces the (volume, net heat) table via the experiment−dilution join. Used by both `FileHandler::ReadITC` and the GUI import dialog. Volume-vector helpers (`injectionCount`/`setUniformInjectionVolume`/`padInjectionVolumes`), coupled `setScalingFactor`, and `setDilutionEnabled` (gates an already-integrated dilution into the join).
- **spectrahandler.cpp/h**: Spectral data import and processing

### Mathematical Operations
- **libmath.cpp/h**: Mathematical utilities and algorithms
  - `SimpsonIntegrate`: exact composite Simpson; panels whose endpoint is non-finite use an open rule (the BC50 integrands are defined on the half-open [0,1) — alpha = x/(1-x) diverges at full saturation). Pinned by `test_quadrature`; do not "simplify" the endpoint handling away.
- **minimizer.cpp/h**: Optimization algorithms for parameter fitting
- **equil.cpp/h**: Equilibrium calculations for supramolecular systems
- **concentrationalpolynomial.cpp/h**: Concentration-based polynomial calculations

### Analysis Tools
- **analyse.cpp / analyse_format.cpp / analyse.h**: **JSON-based statistical analysis API (Claude Generated)** — split (2026): `analyse.cpp` = JSON compute (`Calculate*Metrics`, `Extract*`), `analyse_format.cpp` = string/HTML formatting (`Compare*`, `AnalyseReductionAnalysis`, `FormatStatisticsString`); `analyse.h` is the shared facade declaring both.
  - **Core JSON methods**: `CalculateAICMetrics()`, `CalculateMCMetrics()`, `CalculateCVMetrics()`, `CalculateReductionMetrics()`
  - **Extended JSON methods**: `CalculateWGSMetrics()`, `CalculateModelComparisonMetrics()`, `CalculateFastConfidenceMetrics()`, `CalculateGlobalSearchMetrics()`
  - **ML feature extraction**: `ExtractModelMLFeatures()` for standardized training data
  - **Percentile-based confidence intervals**: Monte Carlo analysis with 2.5%/97.5% quantiles
  - **Method coverage**: All 7 post-processing methods (MonteCarlo, WeakenedGridSearch, ModelComparison, CrossValidation, Reduction, FastConfidence, GlobalSearch)
  - **Legacy string-based functions**: `CompareAIC()`, `CompareMC()`, `CompareCV()`, `AnalyseReductionAnalysis()` for backward compatibility
  - **Human-readable formatting**: `FormatStatisticsString()` converts JSON results to console/HTML format
- **pythonbridge.cpp/h**: Python integration for extended functionality
- **toolset.cpp/h**: General utility functions (conversion/math/thermo); `namespace ToolSet` split across `toolset.cpp` (core + conversion), `toolset_io.cpp` (file loaders), `toolset_print.cpp` (`Print`), `toolset_statistics.cpp` (histograms/entropy/confidence/boxplots/model-parameter stats) — shared `toolset.h` facade

## Model System

### Titration Models
Comprehensive support for analytical chemistry techniques:

#### NMR Titrations
- **nmr_1_1_Model**: 1:1 binding NMR titrations
- **nmr_1_1_1_2_Model**: 1:1 and 1:2 competitive binding
- **nmr_2_1_1_1_Model**: 2:1 binding with multiple series
- **nmr_any_Model**: General n:m binding models

#### ITC (Isothermal Titration Calorimetry)
- **itc_1_1_Model**: 1:1 binding enthalpy measurements
- **itc_1_2_Model**, **itc_2_1_Model**, **itc_2_2_Model**: Complex binding stoichiometries
- **itc_any_Model**: General binding models
- **itc_n_1_1_Model**, **itc_n_1_2_Model**: Multi-site binding

#### Fluorescence Spectroscopy
- **fl_1_1_Model**: 1:1 binding fluorescence changes
- **fl_1_1_1_2_Model**: Competitive binding with fluorescence
- **fl_2_1_1_1_Model**: Complex binding patterns

#### UV-Vis Spectroscopy
- **uv_vis_1_1_Model**: 1:1 binding UV-Vis changes
- **uv_vis_2_1_1_1_1_2_Model**: Complex multi-equilibrium systems

### Kinetics Models
- **monomolecularmodel**: First-order kinetics
- **bimolecularmodel**: Second-order kinetics  
- **mm_model**: Michaelis-Menten enzyme kinetics
- **flexmolecularmodel**: Flexible kinetic models

### Thermodynamics Models
- **arrhenius**: Arrhenius equation for temperature dependence
- **eyring**: Eyring equation for reaction kinetics
- **bet**: BET isotherm for surface adsorption

### Scripting Integration (user-defined models)
- **scriptingengine.h**: backend interface — compile once, bind names to stable slots, `evaluate()`; `MakeScriptingEngine()` picks the backend from the model `Engine` field (default ExprTk)
- **exprtkinterpreter.h**: `ExprTkEngine` (active default, fast slot binding) + the engine factory; `CollectSymbols()` enumerates an equation's free variables so the GUI can derive the parameter declaration — register the primitive library + `add_constants()` first, an unknown function makes the whole collection pass fail
- **scriptmodel.cpp/h**: `ScriptModel` (id 100); equation under JSON key `Equation` (legacy `ChaiScript` still read); locals bound per series, real multi-series
- **chaiinterpreter/pymodelinterpreter/dukmodelinterpreter**: optional backends (flags `_Models`/`_Python`/`Use_Duktape`); not yet ported to `ScriptingEngine` — see `roadmap/scriptmodel_performance.md`

## Key Features

### Enhanced File Handling (Claude Generated)
Precise data range extraction from source files:

```cpp
// Extract specific data ranges
QJsonObject getDataRange(int startRow, int endRow, int startCol, int endCol);

// Range parameters for flexible loading
int m_start_row, m_end_row, m_start_col, m_end_col;
```

### DataClass Capabilities
- Project metadata management
- Independent/dependent data table handling
- Model association and parameter storage
- JSON serialization for project persistence

### DataTable Features  
- Eigen::MatrixXd integration for high-performance numerical operations
- Data validation and consistency checking
- Export/import functionality
- Statistical operations and noise generation

## Model Integration

### Model Creation Pattern
```cpp
// Standard model creation via UI approach
QSharedPointer<AbstractModel> model = AddModel(modelId, dataClass);

// Model calculation and result extraction
model->Calculate();
DataTable* results = model->ModelTable();
```

### Parameter Management
```cpp
// Global parameters (stability constants)
model->setGlobalParameter(value, index);

// Local parameters (chemical shifts, extinction coefficients)
model->setLocalParameter(value, seriesIndex, parameterIndex);
```

## Current Implementation Status

### ✅ Enhanced Features
- **File Range Loading**: Precise row/column selection for modular data generation
- **Model Integration**: Direct access to titration models for data generation
- **Memory Safety**: Improved pointer management and validation
- **JSON Compatibility**: Enhanced serialization for complex data structures

### 🔧 Core Functionality
- Complete model library for supramolecular chemistry
- High-performance numerical operations via Eigen
- Flexible scripting integration via the `ScriptingEngine` interface (ExprTk default; Chai/Python/Duktape optional)
- Comprehensive file format support

## Dependencies
- **Qt6**: Core, Qml modules
- **Eigen**: Matrix operations (via libpeakpick)
- **ChaiScript**: Scripting support
- **Python**: Optional interpreter integration
- **ExprTk**: Mathematical expression parsing

## Usage Patterns

### Data Loading with Range Selection
```cpp
FileHandler handler(filename);
handler.LoadFile();
QJsonObject data = handler.getDataRange(startRow, endRow, startCol, endCol);
```

### Model-Based Calculations
```cpp
DataClass* data = new DataClass();
data->setIndependentTable(independentData);
QSharedPointer<AbstractModel> model = AddModel(modelId, data);
model->Calculate();
```

---

## Variable Section (Short-term information, regularly updated)

### Recent Changes

### Current Status
- JSON serialization stable and reliable
- No memory leaks or crashes identified

### Known Issues

#### **ProjectManager Architecture Decision (January 2025)**
- **UUID System**: DataClass containers have project UUIDs, AbstractModel instances inherit this context but don't have independent UUIDs
- **Model Multiplicity**: Multiple models of the same type (e.g., "¹H 1:1-Model") within a project is **correct behavior** for comparative analysis
- **Data Inheritance**: All models within a DataClass share the same project UUID, enabling proper project-model relationships
- **Memory Management**: WeakPointer pattern implemented between ProjectManager and GUI to prevent reference cycles during application shutdown
- **Model Deletion**: Fixed CloseAllForced() index bounds and RemoveTab() use-after-free issues for stable model lifecycle management
- **Status**: **ARCHITECTURE STABLE** - ProjectManager fully integrated with both CLI and GUI frontends

### Performance Notes
- Eigen operations provide excellent numerical performance
- File I/O optimized for large datasets
- Model calculations scale efficiently with data size

### Testing Status
- DataTable tests: 18/25 passing (some edge cases)
- DataClass tests: Most functionality working (one crash test)
- File handling: ✅ All range selection tests passing

---

## Instructions Block (Operator-Defined Tasks and Vision)

### BC50 open items (opened 2026-07-21, after the `SimpsonIntegrate` fix `164505f7`)

Context: the quadrature bug masked how these integrals behave at full saturation
(`alpha = x/(1-x)`, x -> 1). `BC50` itself is sound; the breakdown quantities are not.

- 🔥 **`IItoI::Format_BC50` reports divergent integrals as numbers.** In `ABPair` the free host `A`
  is x-INDEPENDENT, so `B = alpha/const ~ 1/(1-x)`: `int B dx` diverges logarithmically. BC(B)₀,
  BC(AB)₀, BC(A2B)₀, BC(A0)₀, BC(B0)₀ are therefore set purely by where the quadrature truncates —
  all five moved by the identical factor 1.2738 (+27.4 %) when the tiling was fixed. Decide what
  these should be (finite upper saturation? a different definition?) or stop reporting them.
- **`ItoII::Format_BC50` same family, but integrable** (`B ~ (1-x)^-0.5`), so the integrals exist;
  they shifted 0.3-1.0 %. The open endpoint rule only reaches ~sqrt(h) accuracy there.
- ✅ **Substitution x = 1 - t² implemented** (2026-07-21, `BC50::IntegrateSaturation`, all 27 call
  sites). Relative error 1e-8 -> 1e-13 against an independent reference. **Open lever:** the
  substituted integrand hits machine precision already at delta = 1e-3 (10x fewer panels) and
  6e-10 at 1e-2 (100x fewer, still 80x better than the old direct integration). Default left at
  1e-4 because relaxing it changes numbers — but BC50 runs once per resampled model, so this is
  the cheapest remaining speedup of the Monte-Carlo confidence text.
- 🔥 **`IItoII::Format_BC50` hand-rolls its own quadrature** (`bc50.cpp`, the `increments` loop)
  and still carries the overlap defect fixed in `SimpsonIntegrate` plus an OpenMP pessimisation.
  Not ported because its cross-terms (A2B, AB2, A0, B0) are an approximation of their own — they
  multiply Simpson-weighted sums rather than integrating the product — which needs a modelling
  decision first.
- **Give the 12 `x/(1-x)` integrands their analytic endpoint limits** instead of relying on the
  non-finite fallback (`ItoII::BC50_Y` -> 0, `ItoII::ABFunction` -> b11/(2·b12), `AFunction` -> 0).
- **`IItoII::BC50_A0_X` runs a 150-iteration fixed-point loop per evaluation** with no convergence
  check on exit — expensive (it is called ~10⁴× per BC50) and silently returns the last iterate.
- ✅ **Literature cross-check RESOLVED (2026-07-21).** Recomputing the operator's Jan-2016 table
  (`Zusammenfassungen/Januar 2016`) with today's code reproduces his independent octave reference
  on **every** row; an independent high-accuracy Python integration (substitution x = 1-t², which
  removes the endpoint singularity entirely) agrees to 6-7 digits. So SupraFit's numerics were and
  are sound. Findings, quantified — note the published constants limit what is testable at all:
  - The **"deutlich kleineren" 1:1/1:2 values are a DEFINITIONAL difference**, not numerics:
    `BC50 = int [A] dx` vs. Roelens `1/BC50 = 2 int 1/BC50 dx`, ratio 1.056-1.097. The report
    itself already derives this.
  - **A systematic calculator error at the 0.5-0.9 % level is EXCLUDED.** For 1:1, BC50 = 1/beta11
    is an exact identity, so the published pairs can be inverted: 940 µM -> lg beta 3.02687 (rounds
    to the published 3.03) and 4173 µM -> 2.37955 (rounds to 2.380). Both round-trip. The 3-decimal
    row is the discriminating one: a +0.72 % bias equals 0.0031 in lg beta, 6x its rounding
    half-width, yet its inverted value differs by only 0.00045.
  - **Where the constants are precise enough to test, a real ~0.1 % difference remains**: the
    8.3424/16.911 row (4-5 decimals, rounding only ±0.047 %) deviates by -0.080 %. All larger
    apparent deviations sit inside the ±1.15 % that 2-decimal lg beta permits and are therefore
    uninformative.
  - Do NOT argue from the published BC50 error bars (an earlier version of this note did): they
    propagate the beta uncertainty, they say nothing about whether two calculators agree on
    identical input.
  - Side observation: row `3.03 ± 0.01 / 940 ± 3 µM` is internally inconsistent — ±0.01 in lg beta
    implies ±21.6 µM, not ±3. The 2.380/4173 row propagates correctly (±28.8 vs ±28 published).
  - The table's last cell `1.8277e-4` is a factor-10 typo; the mantissa matches to 5 digits.
- **Which BC50 definition should be reported** is therefore still open, and it is a scientific
  choice, not a bug: `ItoII::BC50` implements Roelens (matches literature), while
  `Format_BC50`'s BC(A)₀ is the alternative definition. Both are shown side by side today.
- ✅ **`*_any` models dispatch on their stoichiometry** (2026-07-21, `BC50::Classify` /
  `FromSpeciation`); they used to report the 1:1 formula regardless, up to 273 % wrong.
- **A general BC50 over the speciation is feasible** — the construction behind the hardcoded
  formulas is stoichiometry-independent and was derived + verified numerically: x is the bound
  fraction of the host (α = x/(1-x) = bound/free host), the seemingly arbitrary
  `A = 1/(β11+2β12·B)` is exactly the condition *50 % of the guest is bound*, and BC50(x) = the
  total host concentration. Only the free concentrations and the stoichiometry matrix are needed,
  both of which `SpeciationEngine` provides — so a 2-component version is a 2x2 solve per
  quadrature point. **Blocked on a definition for ≥3 components:** "50 % of the guest" needs a
  designated receptor/substrate pair and a rule for the remaining components.

### Future Tasks (Restructured 2025-01-28)

#### **✅ COMPLETED TASKS**:
0. **ProjectManager Implementation** (Task #0) - ✅ **COMPLETED January 2025**
   - Thread-safe singleton ProjectManager for centralized project management
   - Eliminates CLI-GUI code duplication in project handling
   - UUID-based project identification and caching system
   - Signal-based notifications for GUI Model-View integration
   - **Location**: `src/core/projectmanager.h/cpp`
   - **✅ FULL INTEGRATION COMPLETE**: CLI-GUI consolidation fully implemented
   - **✅ GUI Integration**: Projects loaded via ProjectManager now visible and functional in GUI
   - **✅ Legacy Code Eliminated**: `m_project_list` dependencies completely removed
   - **✅ Architecture Unified**: Single UUID-based system throughout application

#### **🔥 HIGH PRIORITY** - Implement immediately:
1. **Statistical Analysis JSON API** (Task #1)
   - NOT: Extract data from strings
   - INSTEAD: Implement JSON-based statistical methods (`getStatisticsJSON()`)
   - String output should be based on JSON (`formatStatisticsString(json)`)
   - Implement: AIC, SSE, R², parameter uncertainty, prediction variance
   - Goal: Unified statistical analysis API for all client applications

2. **Pre-compiled Headers Implementation** (Task #2)
   - Add PCH support for faster compilation
   - `#include "pch.h"` with Qt6/Eigen/fmt headers
   - Significantly reduced build times during development

3. **ML Pipeline Performance Optimization** (Task #3)
   - Accelerate data generation for productive use
   - Optimize DataGenerator algorithms
   - Focus on speed, not parallelization

#### **⚡ MEDIUM PRIORITY**:
4. **Project Analysis Migration**: Move analysis from src/client/suprafit_cli.cpp to core-libs
5. **Scripted Models**: Improve functionality and integration

#### **📋 LOW PRIORITY** - Long-term:
- ✅ Refactor bc50 code, no more inlining (de-inlined into bc50.cpp)
- Refactor optimizer logic: **A/B/C + seed-unify + converged-fix done** (2026-07-05) — deleted dead LeastSquaresRookfighter; renamed `OptimizeParameters()`→`CollectOptimizationParameters()`; cleaned/documented `eigen_levenberg.cpp`; retired `BisectParameter` so `NewtonRoot` is the single seed heuristic; fixed the `converged` flag to use the real stop criteria instead of `iter<MaxIter`.
- ✅ **Concentration (speciation) solver** (2026-07-10, renamed 2026-07-19) — `concentrationsolver.{h,cpp}` (was `BFGSConcentrationSolver`; default method is damped Newton with analytic Hessian, BFGS is legacy), general convex log-space speciation (Musketeer, DOI 10.1039/d4sc03354j); handles self-aggregation. Tests: `test_concentrationsolver`, `test_nmr_selfaggregation`.
- ✅ **Speciation solver 12-27x faster** (2026-07-11) — `concentrationsolver` now uses a damped Levenberg-Marquardt **Newton** method with the analytic Hessian (kept the class name); precomputed log(β), allocation-free `Objective`. 7-8 iterations (was 54-146), uniform 1e-12 accuracy (the old BFGS stalled on host-excess 1:1). Perf tool: `src/tests/benchmark_speciation` (manual, not a ctest).
- ✅ **N-component equilibrium models + reaction editor** (2026-07-11) — `reactionparser.{h,cpp}` (free-text reaction equations → components + stoichiometry) and `speciationengine.{h,cpp}` (reaction system + BFGS solver) generalise the `*_any` titration models to arbitrary components. `AbstractTitrationModel` carries `m_component_count`/`InitialConcentration(i,c)`/dynamic `InputParameterSize()`; `nmr_any`+`uvvis_any` are N-component, `itc_any` is 2-component-from-protocol with arbitrary species. GUI: reaction-editor `PrepareBox` type 6 (`ui/widgets/reactioneditorwidget`). `Reactions` is the sole definition path — the legacy `MaxA/MaxB/MaxSelfA/Species` grid and its `SpeciesEditorWidget` (type 5) were removed (2026-07-13); an empty `Reactions` field now leaves the model undefined. Tests: `test_reactionparser`, `test_nmr_ncomponent`, `test_uvvis_any`, `test_itc_any`, `test_nmr_any_equivalence`.
- ✅ **`fl_any` fluorescence model** (2026-07-13) — `fluorescence/fl_any_Model.{h,cpp}` is the fluorescence counterpart of `uvvis_any`: BFGS speciation from `Reactions`, signal = linear `Σ c·φ` (per-species fluorescence coefficients, the form `fl_1_1_1_2`/`fl_2_1_1_1` use, generalised to N components); `SupportsVarPro()`, `UseDynamicParameterWidget()`. Registered `fl_any=36`. Test `test_fl_any_equivalence` (signal linearity + VarPro recovers truth).
- ✅ **Runtime citations** (2026-07-11) — `citations.{h,cpp}` + `AbstractModel::CitationKeys()`/`CitationBlock()`; BFGS-driven models cite Musketeer alongside SupraFit in `ModelInfo()`.
- ✅ **Dynamic model-parameter widget** (2026-07-11) — `ui/widgets/dynamicmodelwidget` adds a scalable "Parameter Table" tab (QTableView over `GlobalTable()`/`LocalTable()`, species transposed to rows) for models opting in via `AbstractModel::UseDynamicParameterWidget()` (the `*_any` models). Additive: the classic per-parameter view is unchanged. Read-only first iteration; editing/enable-checkboxes TBD.
### Vision
- **LLM Support for SupraFit**: Based on parsed projects, evaluation should be possible in natural language using local LLMs (if connected). Context information and specialist publication knowledge should be provided or passed on as needed. 