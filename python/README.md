# suprafit — Python control interface

Drive SupraFit's model fitting and statistical post-processing from Python. The default backend
shells out to the `suprafit_cli` executable over JSON. An in-process **native backend** (pybind11)
is a drop-in via `suprafit.set_backend("native")` — same scripts, no subprocess/temp files. The
native backend covers **fitting and post-processing** (Monte Carlo, cross-validation, …) for
array/file data; the equation/model data generators remain CLI-only. See
`roadmap/python_interface.md`.

## Native backend (optional, faster)

Build the in-process module once, then switch backend — nothing else in your script changes:

```bash
cmake -S . -B build-pybind -DSUPRAFIT_PYBIND=ON -DPython_EXECUTABLE=$(which python)
cmake --build build-pybind --target suprafit_pycore -j4   # writes python/suprafit/_core*.so
```
```python
import suprafit as sf
sf.set_backend("native")     # in-process; falls back to a clear error if _core isn't built
```

Copyright (C) 2016 - 2026 Conrad Hübler. Claude Generated.

## Requirements

- A built `suprafit_cli` — set `SUPRAFIT_CLI_PATH` to its path, or build it in the repo
  (`cd debug && make suprafit_cli -j4`) and the wrapper auto-discovers `debug/bin/linux/`.
- Python ≥ 3.9. NumPy is optional (results fall back to lists without it).

## Install (editable)

```bash
cd python
pip install -e .[test]
```

## Quick start

```python
import suprafit as sf

# build a project from numpy arrays (indep: rows x n_vars, dep: rows x n_series)
import numpy as np
indep = np.column_stack([np.full(20, 1e-3), np.linspace(0, 4e-3, 20)])
dep   = np.loadtxt("signal.dat").reshape(20, 2)
proj = sf.Project.from_arrays(indep, dep)

# queue models (by snake_case name or integer id) and fit
proj.add_model("nmr_1_1")
proj.add_model("nmr_1_1_1_2", options={"FastMode": True})
proj.fit(nproc=4)

m = proj.model("nmr_1_1")
print(m.sse, m.global_parameters)        # SSE + lg K (NumPy array)
print(m.aic, m.aicc, m.standard_error)   # scalar fit statistics (None if absent in the JSON)
print(m.features())                      # flat scalar+parameter dict, ready for scikit-learn

# post-processing
proj.monte_carlo(steps=500, variance_source="SEy", seed=42)
proj.cross_validation(cv_type="L0O")
proj.fit(nproc=4)
mc = m.statistics["MonteCarlo"][0]        # list of result blocks (one per stored run)

# a feature table over all fitted models (needs pandas; else use [m.features() for m in proj.models])
df = proj.results_frame()                 # one row per model: scalars + flattened parameters
```

Scalar accessors on each `Model`: `sse`, `sae`, `aic`, `aicc`, `standard_error`, `mean_error`,
`variance`, `valid`, `converged` (any not present in the project JSON read back as `None`/`False`).

Other constructors: `Project.from_file("data.dat", indep_cols=slice(0,2), dep_cols=slice(2,9))`
and `Project.from_equations("0.001|(X-1)*1e-4", n_points=10, n_vars=2)`.

## Model names

`add_model` accepts the snake_case names in `suprafit.MODELS` (e.g. `nmr_1_1`, `nmr_1_1_1_2`,
`nmr_2_1_1_1_1_2`, `nmr_any`, `itc_1_1`, `uv_vis_1_1`, `uvvis_any`, ...) or the integer ids from
`enum SupraFit::Model` in `src/global.h`.

## Tests

```bash
cd python && pytest
```

## Scope (Phase 1)

In scope: build/load a project, add models, fit, run the seven post-processing methods, read back
parameters/statistics/scalar fit metrics as dicts/NumPy arrays, and assemble flat feature
tables (`Model.features()` / `Project.results_frame()`). ML feature vectors are assembled
Python-side from the fitted model — the CLI's `-ml-features` file is only a slimmed project and is
intentionally not read. Out of scope here: scripted models via Python, GUI/chart automation, wheel
packaging with bundled Qt, and the in-process native backend (`sf.set_backend("native")`, Phase 2 —
see `roadmap/python_interface.md`).