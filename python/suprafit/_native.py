"""Thin Python wrappers over the native `suprafit._core` module's low-level surface.

These need the compiled pybind11 module (`cmake -DSUPRAFIT_PYBIND=ON`); without it they raise a
clear NotImplementedError. The heavy-weight, backend-transparent path is `Project` +
`set_backend("native")`; the helpers here expose the interactive verbs and data generation directly.
Claude Generated.
"""

from __future__ import annotations

from . import _models


def _require_core():
    try:
        import suprafit._core as _core
        return _core
    except ImportError as e:
        raise NotImplementedError(
            "This needs the native module suprafit._core, which is not built. Build it with "
            "`cmake -DSUPRAFIT_PYBIND=ON` (see roadmap/python_interface.md Phase 2)."
        ) from e


def generate_independent(equations: str, datapoints: int):
    """Generate an independent data table (rows x variables) from the CLI equation generator.

    `equations` is pipe-separated, one expression per variable, with `X` the 1-based row index,
    e.g. `generate_independent("0.001|(X-1)*1e-4", 20)`. Returns a NumPy array. Claude Generated."""
    return _require_core().generate_independent(str(equations), int(datapoints))


def native_model(model, independent, dependent):
    """Create a live in-process model handle (native backend) for interactive use.

    `model` is a snake_case name (e.g. "nmr_1_1") or an integer id; `independent`/`dependent` are
    array-likes. The returned object exposes `set_global(value, index)`, `set_local(value, series,
    param)`, `initial_guess()`, `fit()`, and getters `sse()`, `aic()`, `aicc()`, `converged()`,
    `global_parameters()`, `local_parameters()`, `model_signal()`, `export_json()`. Requires NumPy.
    Claude Generated."""
    import numpy as np
    core = _require_core()
    mid = _models.model_id(model)
    return core.Model(mid,
                      np.ascontiguousarray(independent, dtype=float),
                      np.ascontiguousarray(dependent, dtype=float))
