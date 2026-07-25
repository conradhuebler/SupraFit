"""Post-processing job builders — produce the controller JSON the CLI's PostFitAnalysis consumes.

Controller keys mirror the `*ConfigBlock` templates in src/capabilities/jobmanager.h. Method ids are
`SupraFit::Method` (src/global.h:146-154): MonteCarlo=1, WeakenedGridSearch=2, ModelComparison=3,
CrossValidation=4, Reduction=5, FastConfidence=6, GlobalSearch=7. Claude Generated.
"""

from __future__ import annotations

from typing import Iterable

# Method ids (src/global.h enum SupraFit::Method)
MC = 1
WEAKENED_GRID_SEARCH = 2
MODEL_COMPARISON = 3
CROSS_VALIDATION = 4
REDUCTION = 5
FAST_CONFIDENCE = 6
GLOBAL_SEARCH = 7

_METHOD_NAMES = {
    MC: "MonteCarlo",
    WEAKENED_GRID_SEARCH: "WeakenedGridSearch",
    MODEL_COMPARISON: "ModelComparison",
    CROSS_VALIDATION: "CrossValidation",
    REDUCTION: "Reduction",
    FAST_CONFIDENCE: "FastConfidence",
    GLOBAL_SEARCH: "GlobalSearch",
}


def method_name(method_id: int) -> str:
    return _METHOD_NAMES.get(method_id, f"Method{method_id}")


def _optstr(values: Iterable[float] | None) -> str:
    """Encode a float list as the C++ QVector<double>-from-string form: "[a b c]...".
    The CLI's ToolSet::String2DoubleVec parses whitespace-separated doubles, brackets optional."""
    if values is None:
        return ""
    return " ".join(repr(float(v)) for v in values)


def monte_carlo(
    steps: int = 500,
    variance_source="SEy",
    confidence: float = 95.0,
    variance: float | None = None,
    original_data: bool = False,
    independent_row_variance: Iterable[float] | None = None,
    plot_bins: int = 50,
    store_raw: bool = True,
    lightweight: bool = False,
    seed: int | None = None,
) -> dict:
    """Monte Carlo controller (MonteCarloConfigBlock, jobmanager.h:97-130).

    `variance_source` is "custom"/"SEy"/"sigma"/"bootstrap" (or the int 1-4)."""
    src_map = {"custom": 1, "SEy": 2, "sigma": 3, "bootstrap": 4}
    if isinstance(variance_source, str):
        vs = src_map.get(variance_source, 2)
    else:
        vs = int(variance_source)
    cfg = {
        "Method": MC,
        "MaxSteps": int(steps),
        "VarianceSource": vs,
        "confidence": float(confidence),
        "OriginalData": bool(original_data),
        "PlotBins": int(plot_bins),
        "StoreRaw": bool(store_raw),
        "LightWeight": bool(lightweight),
    }
    if variance is not None:
        cfg["Variance"] = float(variance)
    irv = _optstr(independent_row_variance)
    if irv:
        cfg["IndependentRowVariance"] = irv
    if seed is not None:
        cfg["RandomSeed"] = int(seed)
    return cfg


def cross_validation(
    cv_type="L0O",
    steps: int = 0,
    x: int = 3,
    algorithm: int = 2,
    plot_bins: int = 50,
    store_raw: bool = True,
    lightweight: bool = False,
) -> dict:
    """Cross-validation controller (ResampleConfigBlock, Method=4). cv_type: L0O/L2O/CXO (or 1-3)."""
    type_map = {"L0O": 1, "L2O": 2, "CXO": 3}
    if isinstance(cv_type, int):
        cxo = cv_type
    elif cv_type in type_map:
        cxo = type_map[cv_type]
    else:
        cxo = int(cv_type)
    return {
        "Method": CROSS_VALIDATION,
        "CXO": int(cxo),
        "X": int(x),
        "MaxSteps": int(steps),
        "Algorithm": int(algorithm),
        "PlotBins": int(plot_bins),
        "StoreRaw": bool(store_raw),
        "LightWeight": bool(lightweight),
    }


def reduction(
    runtype: int = 1,
    steps: int = 0,
    plot_bins: int = 50,
    store_raw: bool = True,
    lightweight: bool = False,
) -> dict:
    """Parameter-reduction controller (ResampleConfigBlock, Method=5). runtype: 1 backward/2 forward/3 both."""
    return {
        "Method": REDUCTION,
        "ReductionRuntype": int(runtype),
        "MaxSteps": int(steps),
        "PlotBins": int(plot_bins),
        "StoreRaw": bool(store_raw),
        "LightWeight": bool(lightweight),
    }


def weakened_grid_search(
    max_steps: int = 100,
    max_parameter: float = 10.0,
    confidence: float = 95.0,
    f_value: float = 0.05,
    parameter_index: int = 0,
    error_convergency: float = 1e-4,
    store_raw: bool = True,
    lightweight: bool = False,
) -> dict:
    """Weakened grid search (GridSearchConfigBlock, Method=2). parameter_index: 0 SSE/1 SEy/2 Chi2/3 sigma."""
    return {
        "Method": WEAKENED_GRID_SEARCH,
        "MaxSteps": int(max_steps),
        "MaxParameter": float(max_parameter),
        "confidence": float(confidence),
        "f_value": float(f_value),
        "ParameterIndex": int(parameter_index),
        "ErrorConvergency": float(error_convergency),
        "StoreRaw": bool(store_raw),
        "LightWeight": bool(lightweight),
    }


def model_comparison(
    max_steps: int = 100,
    max_parameter: float = 10.0,
    confidence: float = 95.0,
    parameter_index: int = 0,
    f_value: float = 0.05,
    include_series: bool = True,
    store_raw: bool = True,
    lightweight: bool = False,
) -> dict:
    """Model comparison (ModelComparisonConfigBlock, Method=3)."""
    return {
        "Method": MODEL_COMPARISON,
        "MaxSteps": int(max_steps),
        "MaxParameter": float(max_parameter),
        "confidence": float(confidence),
        "ParameterIndex": int(parameter_index),
        "f_value": float(f_value),
        "IncludeSeries": bool(include_series),
        "StoreRaw": bool(store_raw),
        "LightWeight": bool(lightweight),
    }


def fast_confidence(
    max_steps: int = 100,
    max_parameter: float = 10.0,
    confidence: float = 95.0,
    parameter_index: int = 0,
    scaling: int = 1,
    store_raw: bool = True,
    lightweight: bool = False,
) -> dict:
    """Fast confidence (ModelComparisonConfigBlock, Method=6)."""
    return {
        "Method": FAST_CONFIDENCE,
        "MaxSteps": int(max_steps),
        "MaxParameter": float(max_parameter),
        "confidence": float(confidence),
        "ParameterIndex": int(parameter_index),
        "FastConfidenceScaling": int(scaling),
        "StoreRaw": bool(store_raw),
        "LightWeight": bool(lightweight),
    }


def global_search(parameter_count: int, lower: Iterable[float], upper: Iterable[float], max_steps: int = 100) -> dict:
    """Global search (Method=7). Per-parameter bounds; the CLI builds its own controller layout
    (globalsearch.cpp), so this is a minimal, well-formed stand-in for the common case."""
    lo = list(lower)
    hi = list(upper)
    if len(lo) != len(hi) or len(lo) != parameter_count:
        raise ValueError(f"global_search: bounds must match parameter_count={parameter_count}")
    cfg = {"Method": GLOBAL_SEARCH, "ParameterSize": int(parameter_count), "MaxSteps": int(max_steps)}
    for i, (a, b) in enumerate(zip(lo, hi)):
        cfg[str(i)] = [float(a), float(b)]
    return cfg