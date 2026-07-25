<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: Python Scripting Interface for SupraFit

**Status:** ADD (proposal) · **Owner:** Conrad Hübler · **Author note:** Claude Generated

## Context

SupraFit is a C++/Qt6 supramolecular-chemistry fitting framework (`libcore`, `libmodels`,
`suprafit_cli`, `suprafit` GUI). All model I/O, configuration, and statistics already flow
through `QJsonObject`, so the framework is effectively a JSON-driven engine with a thin C++ API
on top. Users increasingly want to drive fits, batch-generate data, run post-processing, and
extract ML features from Python (NumPy/pandas/scikit-learn/PyTorch ecosystems) instead of hand-
writing JSON configs for the CLI.

## Goal

Let a Python user, in a normal script/notebook:
1. build or load a project (independent/dependent data),
2. create a model by id, set global/local parameters and options,
3. run an initial guess and a non-linear fit,
4. run statistical post-processing (Monte Carlo, cross-validation, grid search, model comparison),
5. read back results and standardized ML feature vectors — as Python dicts / NumPy arrays.

Non-goal (first release): re-implement the GUI, expose chart widgets, or support live callbacks.

## Current State

### What exists today (both gated behind CMake option `_Python`, default **OFF**)

| Component | File | What it does | Verdict |
|-----------|------|--------------|---------|
| ctypes file loader | `src/core/pythonbridge.{h,cpp}` | `extern "C" LoadFile(const char*)` opens a `.suprafit`/`.json` and returns its JSON as a `CharPtrWrapper` blob; `Release()` frees it. Driven from Python via `ctypes` (`src/python/main.py`). | Proof-of-concept only. **Read-only**, one function. Has bugs: `ret->len = size*2` but only `size` bytes are `malloc`'d (buffer overrun); `python_cli` test (`src/python/main.cpp`) passes a dangling `.c_str()`. |
| Embedded-interpreter model | `src/core/models/pymodelinterpreter.{cpp,h}` | Wraps CPython (`Py_Initialize`, `PyRun_SimpleString`, `PyObject_CallObject`) so a user Python expression acts as a *model* backend, evaluated per data point `Calculate(i,j)`. Sibling to ChaiScript/ExprTk interpreters. | A **scripted-model plugin**, not a control API. Very slow (string-interpolates every matrix cell into Python), uses pre-3.11 init patterns, no error handling. |

### Build wiring (`CMakeLists.txt`, `global_config.h.in`)
- Option `_Python` OFF by default; when ON: `find_package(PythonLibs)` (legacy/deprecated
  FindPythonLibs, **not** modern `find_package(Python3 COMPONENTS Development)`), promotes `core`
  and `models` to **SHARED** libraries, builds a `pythonbridge` shared lib and a `python_cli`
  demo exe. `pymodelinterpreter.cpp` is always compiled but its body is `#ifdef _Python`.
- `#cmakedefine _Python` in `global_config.h.in`; currently `#undef` in the active debug build.

### Verdict on current state
There is **no real binding**. Nothing lets Python create a `DataClass`, call `CreateModel`, fit,
or run statistics. The two artefacts are (1) a read-only ctypes demo and (2) a scripting backend
that goes the *other* direction (Python inside a model). A usable interface must be built new.

### C++ surface a binding must expose (already JSON-centric)

| Concern | Key entry points |
|---------|------------------|
| Project / data | `DataClass(QObject*)`, `DataClass(const QJsonObject&, int type)`, `setIndependentTable/setDependentTable(DataTable*)`, `ImportData/ExportData` (QJsonObject). `DataTable` wraps `Eigen::MatrixXd`. |
| Model creation | `CreateModel(int model, QPointer<DataClass>)` factory (`src/core/models/models.{h,cpp}`); model ids in `enum SupraFit::Model` (`src/global.h`). |
| Parameters / options | `setGlobalParameter(v,i)`, `setLocalParameter(v,series,param)`, `GlobalParameter()/LocalParameter()` (DataTable), `setOptions(QJsonObject)`, `DefineModel(QJsonObject)`. |
| Fit | `model->InitialGuess()`; then `Minimizer{setModel, Minimize(), Parameter()}` **or** `NonLinearFitThread{setModel, run(), Converged(), ConvergedParameter()}`; apply via `ImportModel(json)` + `Calculate()`. (`src/core/minimizer.h`) |
| Scalar results | `SSE()`, `SEy()`, `GetAIC()`, `GetAICc()`, `GetRSquared()`, `GetChiSquare()`, `ModelTable()`, `ErrorTable()`, `ExportModel(stats,locked)`. (`AbstractModel.h`) |
| Post-processing | `JobManager{setModel, AddSingleJob(QJsonObject), RunJobs()}` with `*ConfigBlock` templates (`src/capabilities/jobmanager.h`); results land in the model (`UpdateStatistic`, `getStatistic(Method,idx)`, `ExportStatistic`). |
| Analysis / ML | `StatisticTool::Calculate{AIC,MC,CV,Reduction,...}Metrics(...)`, `ExtractModelMLFeatures(model)`, `FormatStatisticsString(...)` (`src/core/analyse.h`); `MLFeatureExtractor` (`src/capabilities/mlfeatureextractor.h`). |
| Project I/O | `ProjectManager::instance()` singleton: `loadProject`, `saveProject`, `createProjectFromJson`, `getProjectAsJson`, `getProjectModels`. |
| Runtime prerequisite | A `QCoreApplication` instance is required (Qt object machinery + `QThreadPool` for MC/jobs; `DataGenerator` needs `QJSEngine` from Qt Qml). The basic fit path is **synchronous** (`NonLinearFitThread::run()` is called directly — no event loop needed). |

**Key lever:** everything important is already `QJsonObject`. A binding can move most state as
Python `dict` <-> JSON and expose only a handful of verbs, keeping the C++/Python boundary tiny.

## Options

| # | Option | Effort | Build / packaging impact | Maintainability | Unlocks | Main risks |
|---|--------|--------|--------------------------|-----------------|---------|-----------|
| **A** | **pybind11 module** `import suprafit` binding `libcore`/`libmodels` | **High** (2–4 wk MVP→full) | New `pybind11` submodule; `core`/`models` already build SHARED under `_Python`. Wheel must bundle Qt6 Core+Qml (+ libstdc++) → `auditwheel`/`delvecto`; or ship source build. | Good long-term, but binding tracks C++ API churn; needs a stable façade to avoid re-work. | Full in-process control: data as NumPy, fit, stats, ML features, no subprocess/file round-trips. Fastest at runtime; enables notebooks. | Qt-in-wheel packaging is the hard part; `QCoreApplication` lifetime/threads; ABI/Qt version pinning; ships GPL in a wheel. |
| **B** | **Embed Python**, extend `pythonbridge`/`pymodelinterpreter` | Medium | Keeps `find_package(Python)`; SupraFit stays the host process, Python is a guest. | Two interpreters' lifecycles; the existing bridge is buggy and would need a rewrite. | Scripting *inside* SupraFit runs (user Python snippets as models/hooks). Does **not** give a normal `import suprafit` workflow. | Wrong direction for the stated goal (user wants to drive SupraFit *from* Python, not run Python *inside* a SupraFit run); GIL/threading with Qt; poor notebook UX. |
| **C** | **Thin Python package** wrapping `suprafit_cli` + JSON | **Low** (2–4 days MVP) | Zero C++/build change. Pure-Python wheel (`pip install`, no Qt in wheel — needs `suprafit_cli` on PATH). | Very easy; decoupled from C++ internals (only depends on JSON schema + CLI flags, both already documented). | Build projects, run generate/fit/analyse pipelines, parse result JSON into dicts/pandas. Reuses the already-tested CLI path. | Subprocess + temp-file overhead (bad for large MC sweeps / per-iteration loops); limited to what the CLI exposes; no live object access. |

Notes:
- Option A's runtime need (a single process-wide `QCoreApplication`) is easily handled by
  constructing one lazily on `import` and tearing it down `atexit`.
- Option C rides the existing, tested `suprafit_cli` JSON contract (`JSON_datastruture.md`,
  `src/client/usage_example.md`, `src/client/CLAUDE.md`) — near-zero risk, immediate value.
- Options are **complementary**: C ships value now and doubles as the acceptance oracle for A;
  B stays a separate, orthogonal feature (user scripts as models) and is out of scope here.

## Recommendation

**Adopt C first, then A. Retire/quarantine B's bridge code.**

1. Ship **Option C** (`suprafit` pure-Python package, subprocess + JSON) as the MVP. It delivers
   the full generate→fit→analyse→features workflow in days, with no build risk, and its outputs
   become the golden reference for validating A.
2. Build **Option A** (pybind11 `suprafit._core`) behind the same Python API, so scripts written
   against C keep working while gaining in-process speed and NumPy data exchange. Expose a small
   **JSON-first façade** (create/import/export/fit/run-jobs take dicts) plus typed convenience
   accessors — this insulates the binding from C++ internal churn (the biggest A risk).
3. Do **not** invest further in Option B for this goal. Fix or delete the buggy `pythonbridge`
   ctypes demo; keep `pymodelinterpreter` only as the (separate) scripted-model backend, modernised
   to Python 3.11+ init when convenient.

Rationale: the codebase is already JSON-native, so a JSON façade makes both C and A cheap and keeps
them API-compatible; C de-risks A by pinning behaviour; A is where notebook-grade performance and
NumPy integration ultimately live.

## Phased Roadmap

### Phase 0 — Façade + reference outputs (0.5 wk)
- **Scope:** Define the Python-facing API (`Project`, `Model`, `fit()`, `run_jobs()`, `features()`)
  as thin wrappers over JSON. Freeze a small set of golden CLI runs (1:1 NMR fit + MC + CV) whose
  JSON outputs become regression fixtures.
- **Files:** new `python/suprafit/` package (docs/spec only), reuse `input/*.json` fixtures.
- **Effort:** 0.5 wk · **Risk:** low.

### Phase 1 — MVP: Option C subprocess wrapper (1 wk) — ✅ DONE + hardened
- **Scope:** `suprafit.cli` runner (locate `suprafit_cli`, run with a temp JSON config, capture
  result JSON), plus loaders that parse project/model JSON into dicts and optional pandas frames.
  Cover: generate data, load project, fit models, run post-processing, extract statistics/ML features.
- **Files (shipped):** `python/suprafit/` package (`__init__.py`, `_cli.py`, `_backend.py`,
  `_config.py`, `_data.py`, `_jobs.py`, `_models.py`, `_project.py`, `_results.py`, `errors.py`),
  `pyproject.toml`; reuses `suprafit_cli` (`-i/-o/-n`, `AddModels`, `PostFitAnalysis`, ML pipeline).
- **Hardening (2026-07-17):** `Model` surfaces scalar fit statistics (`aic`, `aicc`, `sae`,
  `standard_error`, `mean_error`, `variance`, `valid`, plus `sse`/`converged`); added
  `Model.features()` + `Project.results_frame()` (feature table for scikit-learn); parser-only
  golden regression `python/tests/test_reference.py` against `data/samples/reference/simulated_1_1.json`
  (the oracle for Phase 2, runs without a built CLI). Note: real ML feature vectors are assembled
  Python-side — the CLI's `-ml-features` file is only a slimmed project and is intentionally not read.
- **Effort:** ~1 wk · **Risk:** low (subprocess/temp-file overhead only).

### Phase 2 — pybind11 core module, MVP surface (1.5 wk) — ✅ fit path DONE
- **Scope:** `import suprafit._core`; lazy `QCoreApplication`. Bind the JSON façade:
  `create_data(json)`, `create_model(id, data)`, `import_model/export_model(json)`,
  `set_global/set_local`, `initial_guess()`, `fit()` (wrap `NonLinearFitThread::run()` synchronously),
  scalar getters (SSE/AIC/AICc/R²/χ²), and `ModelTable/ErrorTable` as NumPy (Eigen↔NumPy).
- **Delivered (2026-07-17):** new CMake option `SUPRAFIT_PYBIND` builds `suprafit._core` from
  `src/python/bindings/module.cpp`; `external/pybind11` (v3.0.4) submodule added. core/models stay
  **STATIC but PIC** and are linked into the SHARED module (their `CreateModel` cycle is illegal for
  shared libs — this is why the old `_Python=ON` never configured). Resolved a nasty name collision
  between the project's `_Python` option and CMake's FindPython internal `_Python` variable. The
  bound `fit_from_tables(indep, dep, models_json)` builds a DataClass from Eigen matrices and calls
  the same `AnalysisManager::fitModelsToData` the CLI uses, returning the identical project JSON.
  `NativeBackend` is wired so `set_backend("native")` is a transparent drop-in for array/file data;
  it recovers the reference oracle's constants exactly and matches the CLI backend
  (`python/tests/test_native.py`).
- **Phase 3 post-fit statistics also DONE in-process (2026-07-17):** the earlier SIGFPE was just the
  app-wide `threads` property being unset — the statistics engine divides by it
  (`blocksize = MaxSteps/threads/20`), so an unset 0 crashed; the module now sets it as the CLI does.
  Monte Carlo + cross-validation (and the other methods) run in-process via `fit_from_tables(..., nproc)`
  and reproduce the reference oracle's summaries (`python/tests/test_native.py`).
- **ML features DONE (2026-07-17):** `fit_from_tables` reconstructs each fitted model
  (`JsonHandler::Json2Model`) and attaches `StatisticTool::ExtractModelMLFeatures` as
  `model_export.ml_features`; the reconstructed model's empty error accumulators are backfilled from
  the reliable exported statistics (SSE/AIC/AICc/standard_error). Surfaced as `Model.ml_features`
  (native only; None on the CLI). `python/tests/test_native.py::test_native_ml_features`.
- **ModelTable/ErrorTable as NumPy DONE (2026-07-19):** `fit_from_tables` also returns the fitted
  model signal (`ModelTable()`, reliable after reconstruction) and the residuals (derived as
  `dependent - signal`, since the rebuilt model's `ErrorTable()` is not filled) as row-major arrays;
  surfaced as `Model.model_signal` / `Model.model_error` (2D NumPy). `test_native_model_tables`.
- **Low-level verbs + data generation DONE (2026-07-19):** a `_core.Model` class (exposed as
  `suprafit.native_model(id_or_name, indep, dep)`) gives a live model handle — `set_global`,
  `set_local`, `initial_guess`, `fit` (real Minimizer), and getters `sse/aic/aicc/converged/
  global_parameters/local_parameters/model_signal/export_json` (tables as NumPy). `_core`'s
  `generate_independent(equations, datapoints)` (exposed as `suprafit.generate_independent`) drives
  the CLI equation generator. `python/tests/test_native.py::{test_native_live_model,
  test_native_generate_independent}`.
- **Dependent-data generation DONE (2026-07-19):** `_core.generate_dependent`
  (`suprafit.generate_dependent(model, indep, global_params, local_params, noise_std, seed)`) builds
  a model at caller-supplied parameters, computes the signal, and adds reproducible i.i.d. Gaussian
  noise — deterministic **ground-truth** generation (draw random parameters in NumPy for random
  datasets, so the truth is always known). Round-trip verified (params → data → refit recovers the
  params at ~0 SSE). `python/tests/test_native.py::test_native_generate_dependent`.
- **ITC models usable (2026-07-19):** ITC models need the experiment setup as *system parameters*
  (CellVolume/CellConcentration/SyringeConcentration/Temperature) or the heat is identically zero.
  `LiveModel` gained `set_system_parameter(index, value)` + `load_system_parameters()` (which calls
  the model's `UpdateParameter()`, reading the values into `m_cell_concentration` etc. — NOT
  `LoadSystemParameter()`, which would reload stored JSON and discard them), plus `calculate()`.
  `suprafit.native_model(..., system_parameters={"cell_volume":…, "cell_concentration":…, …})`
  wires it up by friendly name. Round-trip verified (K/dH → heat → refit recovers them at ~0 SSE);
  `python/tests/test_native.py::test_native_itc_model`.
- **ITC through the transparent Project (2026-07-19):** `fit_from_tables` takes a
  `system_parameters` {index: value} map and sets it on the shared DataClass *before*
  `fitModelsToData` creates its models, so every model inherits the cell/syringe setup;
  `Project.from_arrays(..., system_parameters={...})` (friendly names) threads it through the task
  config → NativeBackend. Verified via Project + native backend (`test_project_itc_native`).
- **Thermogram import DONE (2026-07-19):** `suprafit.read_itc(path)` loads a raw `.itc` trace
  (`ToolSet::LoadITCFile` → `ThermogramHandler` → `ItcProcessor::process()`, integrating over one
  repeating peak rule spanning the trace) and returns `{independent: per-injection volumes,
  dependent: net heats, system_parameters}` (cell volume + temperature from the file metadata, keyed
  by friendly name). Add the sample `cell_concentration`/`syringe_concentration` (the `.itc` rarely
  carries them) and fit. Verified on `data/samples/itc/sample.itc` (61 injections → itc_1_1 fit,
  lg K≈5.0, converged; `test_read_itc_and_fit`). NB: the earlier hang was specific to the degenerate
  12-point `synthetic.itc`, not a code defect — real thermograms integrate fine.
- **Phase 3 is complete.** Remaining across the whole roadmap: Phase 4 (wheel packaging with bundled
  Qt) and Phase 5 (retire/modernise the legacy `pythonbridge` ctypes demo).
- **Files:** `src/python/bindings/module.cpp`; `CMakeLists.txt` (SUPRAFIT_PYBIND option +
  pybind11 subdir + PIC + module target); `external/pybind11` submodule; `python/suprafit/_backend.py`.
- **Effort:** ~1.5 wk · **Risk:** medium (Qt app lifetime, Eigen/NumPy copies, threading).

### Phase 3 — Statistics, jobs & ML features in-process (1 wk)
- **Scope:** Bind `JobManager` (`add_job(dict)`, `run_jobs()`) using the `*ConfigBlock` templates,
  `getStatistic/ExportStatistic`, and `StatisticTool::Calculate*Metrics` / `ExtractModelMLFeatures`.
  Return everything as dicts/NumPy so scikit-learn/PyTorch can consume features directly.
- **Files:** `src/python/bindings/bind_jobs.cpp`, `bind_analysis.cpp`.
- **Effort:** ~1 wk · **Risk:** medium (long-running MC on `QThreadPool`; ensure GIL released).

### Phase 4 — Packaging & parity (1 wk)
- **Scope:** Unify the Phase-1 (C) and Phase-2/3 (A) APIs behind one import so backend is
  swappable. Build wheels: `scikit-build-core` + `auditwheel`/`delvewheel` to vendor Qt6
  Core+Qml; document a source-build fallback. Note GPL implications of shipping binaries.
- **Files:** `python/pyproject.toml`, CI workflow (`.github/workflows/`), `python/README.md`.
- **Effort:** ~1 wk · **Risk:** medium-high (Qt-in-wheel is the classic pain point).

### Phase 5 — Cleanup (0.5 wk)
- Fix or remove the buggy `pythonbridge` ctypes demo (`LoadFile` length/alloc bug, dangling
  `.c_str()` in `python_cli`). Modernise `pymodelinterpreter` init to Py 3.11+ if kept.

**Total:** ~5.5 wk to full parity; usable MVP (Phase 1) in ~1 wk.

## Risks

- **Qt in a wheel (A/Phase 4):** bundling Qt6 Core+Qml + libstdc++ across manylinux/macOS/Windows
  is the dominant risk. Mitigate with `scikit-build-core`+`auditwheel`/`delvewheel`, or ship
  source-build + "bring your own Qt" for v1.
- **`QCoreApplication` singleton / threading:** must exist once per process; MC/jobs use
  `QThreadPool`. Construct lazily on import, release the GIL around C++ calls, join threads before
  teardown.
- **API churn vs. binding drift:** binding directly to internal C++ signatures invites breakage.
  The JSON façade + a stable Python layer is the primary mitigation (and why C ships first).
- **GPL:** SupraFit is GPLv3; distributing binary wheels carries copyleft obligations — document,
  and prefer source builds where policy is unclear.
- **Eigen↔NumPy copies:** large `ModelTable` transfers should avoid needless copies (pybind11
  Eigen support / buffer protocol); measure before optimising.
- **Two code paths (C and A):** keeping them behaviour-identical needs the shared test suite below.

## Verification

- **Golden-JSON regression:** freeze `suprafit_cli` outputs for a 1:1 NMR fit + MC + CV (Phase 0);
  both backends must reproduce fitted parameters, SSE/AIC/AICc/R²/χ², and MC/CV summaries within
  tolerance. Reuse existing fixtures (`input/Reference_4Models.json`, `input/multiple_statistic.json`).
- **Cross-backend parity test:** run the same Python script through Option C and Option A; assert
  equal results (this is why C is built first — it is the oracle for A).
- **C++ ground truth:** align with existing `ctest` suites (`test_bfgs_solver`,
  `test_nmr_selfaggregation`) so Python fits match the C++ reference for the same inputs.
- **Numerical tolerances:** compare parameters/statistics with relative tolerance (e.g. 1e-6 for
  deterministic fit scalars; looser, seed-fixed bounds for Monte Carlo).
- **Smoke/CI:** `pip install` the wheel in a clean container, run the quick-start notebook
  (load → fit → features), and a leak/lifetime check that repeatedly creates+destroys projects to
  confirm `QCoreApplication`/thread teardown is clean.

---
*Claude Generated. This document is a planning artefact only — no production code was modified.*
