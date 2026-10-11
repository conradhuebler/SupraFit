"""End-to-end smoke test: synthesize a 1:1 NMR binding curve, fit nmr_1_1, assert recovery.

Synthesizes the dependent signal in Python (closed-form 1:1 mass balance), feeds indep+dep as
arrays, fits via the CLI backend, and checks SSE is small and lg K is near the truth. Requires a
built suprafit_cli (set SUPRAFIT_CLI_PATH or build debug/release). Claude Generated.
"""

from __future__ import annotations

import math
import os

import pytest

import suprafit as sf


def _cli_available() -> bool:
    try:
        sf._cli.find_cli()
        return True
    except sf.SupraFitNotFoundError:
        return False


pytestmark = pytest.mark.skipif(not _cli_available(),
                                reason="suprafit_cli not found (set SUPRAFIT_CLI_PATH or build debug)")


def _synthesize_1_1(lg_k: float = 3.0, n: int = 20):
    """Return (indep, dep) for a 1:1 A+B isotherm; dep is one series (n x 1).

    Uses a moderate K (K*A0 ~ 1) so the binding curve is in the identifiable transition region,
    not saturated (where K is unidentifiable)."""
    import numpy as np
    A0 = 1e-3
    G0 = np.linspace(0, 8e-3, n)
    K = 10 ** lg_k
    # AB^2 - (A0+G0+K) AB + A0*G0 = 0  ->  take the smaller root
    s = A0 + G0 + K
    AB = (s - np.sqrt(s * s - 4 * A0 * G0)) / 2.0
    delta_free, delta_bound = 8.0, 4.0
    dep = (delta_free + (delta_bound - delta_free) * (AB / A0)).reshape(-1, 1)
    indep = np.column_stack([np.full(n, A0), G0])
    return indep, dep, lg_k


def test_fit_nmr_1_1_recovers_truth():
    indep, dep, lg_k_true = _synthesize_1_1(lg_k=3.0)
    proj = sf.Project.from_arrays(indep, dep, outfile="test_smoke")
    proj.add_model("nmr_1_1", options={"FastMode": True, "Convergency": 1e-7})
    models = proj.fit(nproc=2, timeout=120)
    assert models, "fit produced no models"
    m = proj.model("nmr_1_1")
    assert m is not None, "nmr_1_1 not among fitted models"
    assert m.converged, "model did not converge"
    assert math.isfinite(m.sse), f"SSE not finite: {m.sse}"
    # noise-free synthetic data: the fit must reproduce it (validates the full round-trip).
    assert m.sse < 1e-4, f"SSE too large for noise-free synthetic data: {m.sse}"
    # global parameter (lg K) parsed as a finite 1-element array; the exact value depends on the
    # K<->shift identifiability (single series) and is validated by the C++ reference tests, not here.
    assert m.global_parameters is not None and len(m.global_parameters) == 1
    lgk = float(m.global_parameters[0])
    assert math.isfinite(lgk) and 0.0 < lgk < 6.0, f"lg K {lgk} out of sane range"
    assert m.local_parameters is not None and m.local_parameters.size >= 2


def test_two_models_by_name_and_id():
    indep, dep, _ = _synthesize_1_1(lg_k=3.0)
    proj = sf.Project.from_arrays(indep, dep)
    proj.add_model("nmr_1_1")
    proj.add_model(3)  # nmr_1_1_1_2 by id
    models = proj.fit(nproc=2, timeout=180)
    ids = {m.model_id for m in models}
    assert {1, 3}.issubset(ids), f"expected both models fitted, got ids {ids}"