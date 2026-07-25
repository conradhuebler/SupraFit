"""Parse the output project JSON (the `suprafit_cli -o` result) into Python objects.

Output shape (confirmed from data/samples/reference/simulated_1_1.json): the top level maps
`model_1..N` -> {model, name, SSE, data:{globalParameter, localParameter, methods}}. The
`globalParameter.data["0"]` value is a space-separated string of lg K; `methods` is keyed by an
arbitrary index whose `controller.method` (int) identifies the post-processing kind.

Method ids: MonteCarlo=1, WeakenedGridSearch=2, ModelComparison=3, CrossValidation=4,
Reduction=5, FastConfidence=6, GlobalSearch=7 (src/global.h:146-154). Claude Generated.
"""

from __future__ import annotations

from typing import Any

try:
    import numpy as np
    _HAVE_NUMPY = True
except ImportError:
    _HAVE_NUMPY = False

from . import _jobs
from .errors import ResultParseError


def _as_array(values):
    """Return a numpy array if numpy is present, else a plain list."""
    if _HAVE_NUMPY:
        return np.asarray(values, dtype=float)
    return [float(v) for v in values]


def _num_or_none(value):
    """Coerce a scalar JSON value to float, or None if absent/unparseable. Claude Generated."""
    if value is None:
        return None
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def _parse_matrix(values):
    """A JSON list-of-rows -> 2D numpy array (or a list of float rows without numpy). None if absent.
    Claude Generated."""
    if not values:
        return None
    if _HAVE_NUMPY:
        return np.asarray(values, dtype=float)
    return [[float(x) for x in row] for row in values]


def _parse_global_parameters(gp_block: dict | None):
    """globalParameter is {data: {"0": "2.8957 4.6"}, ...} -> a 1D array of lg K values."""
    if not gp_block:
        return None
    data = gp_block.get("data", {})
    if not data:
        return None
    # keys are row indices; concatenate (one row in practice for global params)
    parts: list[float] = []
    for k in sorted(data, key=lambda s: int(s) if str(s).lstrip("-").isdigit() else 0):
        row = data[k]
        if isinstance(row, str):
            parts.extend(float(x) for x in row.split())
        elif isinstance(row, (list, tuple)):
            parts.extend(float(x) for x in row)
        else:
            parts.append(float(row))
    return _as_array(parts)


def _parse_local_parameters(lp_block: dict | None):
    """localParameter is {data: {"0": "6.5 6.0", "1": "6.3 2.3"}, ...} -> 2D array (series x params)."""
    if not lp_block:
        return None
    data = lp_block.get("data", {})
    if not data:
        return None
    rows = []
    for k in sorted(data, key=lambda s: int(s) if str(s).lstrip("-").isdigit() else 0):
        row = data[k]
        if isinstance(row, str):
            rows.append([float(x) for x in row.split()])
        elif isinstance(row, (list, tuple)):
            rows.append([float(x) for x in row])
        else:
            rows.append([float(row)])
    return _as_array(rows)


def _parse_methods(methods: dict | None) -> dict:
    """Group a model's `data.methods` by post-processing kind. Dispatches on `controller.method`."""
    out: dict[str, list[dict]] = {}
    if not methods:
        return out
    for _idx, block in methods.items():
        if not isinstance(block, dict):
            continue
        controller = block.get("controller", {})
        method = controller.get("method", controller.get("Method"))
        if method is None:
            continue
        name = _jobs.method_name(int(method))
        out.setdefault(name, []).append(_parse_method_block(block, int(method)))
    return out


def _parse_method_block(block: dict, method: int) -> dict:
    """Extract the per-parameter boxplot/confidence/histogram data from a method block."""
    params: dict[str, dict] = {}
    histogram: dict[str, list[float]] | None = None
    for key, val in block.items():
        if key == "controller":
            continue
        if not isinstance(val, dict):
            continue
        if key in ("x", "y") and isinstance(val, str):
            histogram = histogram or {}
            histogram[key] = [float(x) for x in val.split()]
            continue
        entry: dict[str, Any] = {"index": val.get("index"), "name": val.get("name"),
                                "type": val.get("type"), "value": val.get("value")}
        if "boxplot" in val:
            entry["boxplot"] = val["boxplot"]
        if "confidence" in val:
            entry["confidence"] = val["confidence"]
        if "data" in val and isinstance(val["data"], dict):
            d = val["data"]
            xs = d.get("x"); ys = d.get("y")
            if isinstance(xs, str) and isinstance(ys, str):
                entry["histogram"] = {"x": [float(a) for a in xs.split()],
                                      "y": [float(b) for b in ys.split()]}
        params[key] = entry
    result: dict[str, Any] = {"parameters": params}
    if histogram is not None:
        result["histogram"] = histogram
    return result


class Model:
    """A fitted model from the output JSON."""

    def __init__(self, raw: dict):
        self._raw = raw
        self.model_id: int = int(raw.get("model", -1))
        self.name: str = raw.get("name", "")
        # Scalar fit statistics (present in fresh CLI output; some are slimmed out of the reference
        # fixtures, hence the None fallback). Claude Generated.
        self.sse: float = float(raw.get("SSE", float("nan")))
        self.sae = _num_or_none(raw.get("SAE"))
        self.aic = _num_or_none(raw.get("AIC"))
        self.aicc = _num_or_none(raw.get("AICc"))
        self.standard_error = _num_or_none(raw.get("standard_error"))
        self.mean_error = _num_or_none(raw.get("mean_error"))
        self.variance = _num_or_none(raw.get("variance"))
        self.valid: bool = bool(raw.get("valid", False))
        self.converged: bool = bool(raw.get("converged", False))
        self.global_parameters = _parse_global_parameters(raw.get("data", {}).get("globalParameter"))
        self.local_parameters = _parse_local_parameters(raw.get("data", {}).get("localParameter"))
        self.statistics = _parse_methods(raw.get("data", {}).get("methods"))
        # Standardized ML feature vector computed in C++ (StatisticTool::ExtractModelMLFeatures);
        # populated by the native backend (dict), None on the CLI backend. Claude Generated.
        self.ml_features = raw.get("ml_features")
        # Fitted model signal (calculated curve) and residuals as 2D arrays (rows x series);
        # native backend only (not in the CLI's export JSON). Claude Generated.
        self.model_signal = _parse_matrix(raw.get("model_signal"))
        self.model_error = _parse_matrix(raw.get("model_error"))

    @property
    def raw(self) -> dict:
        """The verbatim model entry from the output JSON (for fields this wrapper does not surface)."""
        return self._raw

    def features(self) -> dict:
        """Flatten this fit into a flat scalar feature dict for ML / scikit-learn.

        Combines the scalar fit statistics with the fitted global and local parameters
        (`global_<i>`, `local_s<series>_p<param>`). Missing scalars are None. Feed a list of
        these into `pandas.DataFrame` (or `Project.results_frame()`) to build a feature table.
        Claude Generated."""
        feats: dict[str, Any] = {
            "model_id": self.model_id,
            "sse": self.sse,
            "sae": self.sae,
            "aic": self.aic,
            "aicc": self.aicc,
            "standard_error": self.standard_error,
            "mean_error": self.mean_error,
            "variance": self.variance,
            "converged": self.converged,
            "valid": self.valid,
        }
        if self.global_parameters is not None:
            for i, v in enumerate(self.global_parameters):
                feats[f"global_{i}"] = float(v)
        if self.local_parameters is not None:
            for s, row in enumerate(self.local_parameters):
                for p, v in enumerate(row):
                    feats[f"local_s{s}_p{p}"] = float(v)
        return feats

    def __repr__(self) -> str:
        return f"Model(name={self.name!r}, id={self.model_id}, sse={self.sse:.4g}, converged={self.converged})"


def parse_project(output: dict) -> list[Model]:
    """Extract the `model_*` entries from an output project JSON dict, ordered by index."""
    models: list[tuple[int, Model]] = []
    for key, val in output.items():
        if key.startswith("model_") and isinstance(val, dict):
            idx = int(key.split("_", 1)[1]) if "_" in key else len(models)
            models.append((idx, Model(val)))
    if not models:
        raise ResultParseError(f"no model_* entries in output JSON; top-level keys: {list(output)[:8]}")
    models.sort(key=lambda t: t[0])
    return [m for _i, m in models]