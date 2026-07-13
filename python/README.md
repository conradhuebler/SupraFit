# suprafit — Python control interface

Drive SupraFit's model fitting and statistical post-processing from Python. This Phase-1 release
shells out to the `suprafit_cli` executable over JSON; a native pybind11 backend is a planned
drop-in (`suprafit.set_backend("native")`) that won't change scripts.

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

# post-processing
proj.monte_carlo(steps=500, variance_source="SEy", seed=42)
proj.cross_validation(cv_type="L0O")
proj.fit(nproc=4)
mc = m.statistics["MonteCarlo"][0]        # list of result blocks (one per stored run)
```

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
parameters/statistics as dicts/NumPy arrays. Out of scope here: scripted models via Python, GUI/chart
automation, wheel packaging with bundled Qt (see `roadmap/python_interface.md`).