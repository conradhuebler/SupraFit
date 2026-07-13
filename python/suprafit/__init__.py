"""SupraFit — Python control interface (JSON/CLI backend).

Drive SupraFit's fitting + statistical post-processing from Python:

    import suprafit as sf
    proj = sf.Project.from_equations("0.001|(X-1)*1e-4", n_points=10, n_vars=2)
    proj.add_model("nmr_1_1")
    proj.fit()
    m = proj.models[0]
    m.sse, m.global_parameters

This Phase-1 release shells out to `suprafit_cli` (set SUPRAFIT_CLI_PATH or build debug/release).
A native pybind11 backend is a planned Phase-2 drop-in via `sf.set_backend("native")`.

Copyright (C) 2016 - 2026 Conrad Hübler. Claude Generated.
"""

from __future__ import annotations

from ._backend import Backend, CLIBackend, NativeBackend, get_backend, set_backend
from ._jobs import (
    cross_validation,
    fast_confidence,
    global_search,
    method_name,
    model_comparison,
    monte_carlo,
    reduction,
    weakened_grid_search,
)
from ._models import ID_TO_NAME, MODELS, model_id, model_name
from ._project import Project
from ._results import Model
from .errors import (
    CLIExecutionError,
    ModelNameError,
    ResultParseError,
    SupraFitError,
    SupraFitNotFoundError,
)

__version__ = "0.1.0"
__all__ = [
    "Project",
    "Model",
    "MODELS",
    "ID_TO_NAME",
    "model_id",
    "model_name",
    "monte_carlo",
    "cross_validation",
    "reduction",
    "weakened_grid_search",
    "model_comparison",
    "fast_confidence",
    "global_search",
    "method_name",
    "Backend",
    "CLIBackend",
    "NativeBackend",
    "get_backend",
    "set_backend",
    "SupraFitError",
    "SupraFitNotFoundError",
    "ModelNameError",
    "CLIExecutionError",
    "ResultParseError",
    "__version__",
]