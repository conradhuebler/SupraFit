"""Post-processing test: fit + Monte Carlo + Cross-Validation, assert the statistics dict shape.

Uses synthesized 1:1 NMR array data (a dependent table is required; `from_equations` alone does not
provide one). Claude Generated."""

from __future__ import annotations

import pytest

import suprafit as sf
from suprafit._jobs import CROSS_VALIDATION, MC, method_name


def _cli_available() -> bool:
    try:
        sf._cli.find_cli()
        return True
    except sf.SupraFitNotFoundError:
        return False


pytestmark = pytest.mark.skipif(not _cli_available(),
                                reason="suprafit_cli not found (set SUPRAFIT_CLI_PATH or build debug)")


def _synthesize_1_1(lg_k=3.0, n=12):
    import numpy as np
    A0 = 1e-3
    G0 = np.linspace(0, 8e-3, n)
    K = 10 ** lg_k
    s = A0 + G0 + K
    AB = (s - np.sqrt(s * s - 4 * A0 * G0)) / 2.0
    dep = (8.0 + (4.0 - 8.0) * (AB / A0)).reshape(-1, 1)
    indep = np.column_stack([np.full(n, A0), G0])
    return indep, dep


def test_monte_carlo_and_cross_validation():
    indep, dep = _synthesize_1_1()
    proj = sf.Project.from_arrays(indep, dep)
    proj.add_model("nmr_1_1", options={"FastMode": True, "Convergency": 1e-7})
    proj.monte_carlo(steps=20, variance_source="SEy", seed=42, store_raw=True, lightweight=False)
    proj.cross_validation(cv_type="L0O", steps=0)
    models = proj.fit(nproc=2, timeout=240)
    m = proj.model("nmr_1_1")
    assert m is not None
    stats = m.statistics
    assert method_name(MC) in stats, f"no MonteCarlo results; have {list(stats)}"
    mc = stats[method_name(MC)]
    assert len(mc) >= 1
    params = mc[0]["parameters"]
    assert params, "MonteCarlo produced no per-parameter entries"
    # MC per-param blocks carry a confidence{lower,upper,error}
    any_param = next(iter(params.values()))
    assert "confidence" in any_param, f"MC param missing confidence: {any_param}"
    cv = stats.get(method_name(CROSS_VALIDATION))
    assert cv, f"no CrossValidation results; have {list(stats)}"
    # CV per-param blocks carry a boxplot (8 keys) but no confidence
    cv_param = next(iter(cv[0]["parameters"].values()))
    assert "boxplot" in cv_param, f"CV param missing boxplot: {cv_param}"
    bp = cv_param["boxplot"]
    assert {"mean", "stddev", "median"}.issubset(bp), f"boxplot missing keys: {bp}"