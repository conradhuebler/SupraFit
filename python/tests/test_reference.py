"""Golden-JSON regression: parse a trusted reference project and assert frozen fit values.

This test is the Python-side counterpart of the C++ ``ReferenceProjectsTest``
(``src/tests/test_reference_projects.cpp``) and reuses the same trusted oracle,
``data/samples/reference/simulated_1_1.json`` (PeerJ 2022 dataset, all four NMR stoichiometries).

It runs the *parser only* — no ``suprafit_cli`` needed — so it always executes, and it doubles as
the frozen oracle for the future native (pybind11) backend: when Phase 2 re-runs these fits it must
reproduce these same numbers, then within the tolerance policy documented in
``data/samples/reference/README.md`` (parameters/SSE tight, Monte Carlo within tolerance).
Claude Generated.
"""

from __future__ import annotations

from pathlib import Path

import json

import pytest

from suprafit import _results

_REF = (Path(__file__).resolve().parents[2]
        / "data" / "samples" / "reference" / "simulated_1_1.json")

pytestmark = pytest.mark.skipif(not _REF.exists(),
                                reason=f"reference fixture missing: {_REF}")


@pytest.fixture(scope="module")
def models_by_id() -> dict:
    data = json.loads(_REF.read_text())
    models = _results.parse_project(data)
    assert len(models) == 4, f"expected 4 NMR models, got {len(models)}"
    return {m.model_id: m for m in models}


def test_scalar_statistics_are_surfaced(models_by_id):
    m = models_by_id[1]  # ¹H 1:1-Model
    assert m.sse == pytest.approx(0.012710165623140661, rel=1e-6)
    assert m.sae == pytest.approx(1.095477637259557, rel=1e-6)
    assert m.standard_error == pytest.approx(0.0008081727187875717, rel=1e-6)
    assert m.variance == pytest.approx(9.144004007494936e-05, rel=1e-6)
    assert m.valid is True
    assert m.converged is True
    # AIC is slimmed out of the reference fixture -> None (fresh CLI output carries it).
    assert m.aic is None


def test_fitted_parameters(models_by_id):
    # 1:1 has one global lg K; 1:1/1:2 has two.
    assert [float(v) for v in models_by_id[1].global_parameters] == pytest.approx([2.8957], rel=1e-6)
    assert [float(v) for v in models_by_id[3].global_parameters] == pytest.approx(
        [3.38346, 2.43951], rel=1e-6)
    lp = models_by_id[1].local_parameters
    assert (lp.shape if hasattr(lp, "shape") else (len(lp), len(lp[0]))) == (7, 2)


def test_postprocessing_present_with_summaries(models_by_id):
    stats = models_by_id[1].statistics
    for method in ("MonteCarlo", "CrossValidation", "Reduction", "FastConfidence"):
        assert method in stats, f"missing {method}; have {list(stats)}"

    # Monte Carlo boxplot mean + 95% confidence for lg K₁₁ (frozen; stochastic -> loose tol).
    mc = stats["MonteCarlo"][0]["parameters"]["0"]
    assert mc["boxplot"]["mean"] == pytest.approx(2.8954447899999933, rel=1e-2)
    assert mc["confidence"]["lower"] == pytest.approx(2.862425, rel=1e-2)
    assert mc["confidence"]["upper"] == pytest.approx(2.926665, rel=1e-2)

    # Cross-validation is deterministic per the tolerance policy -> tight.
    cv = stats["CrossValidation"][0]["parameters"]["0"]
    assert cv["boxplot"]["mean"] == pytest.approx(2.895895353973175, rel=1e-6)


def test_features_flatten(models_by_id):
    feats = models_by_id[3].features()  # ¹H 1:1/1:2-Model: 2 globals, 7 series x 3 local params
    assert feats["model_id"] == 3
    assert feats["sse"] == pytest.approx(0.011888130810861736, rel=1e-6)
    assert feats["global_0"] == pytest.approx(3.38346, rel=1e-6)
    assert feats["global_1"] == pytest.approx(2.43951, rel=1e-6)
    assert feats["converged"] is True
    # local_s<series>_p<param> keys for every entry of the (7 x 3) local table
    assert feats["local_s0_p0"] == pytest.approx(
        float(models_by_id[3].local_parameters[0][0]), rel=1e-9)
    assert sum(1 for k in feats if k.startswith("local_s")) == 7 * 3
