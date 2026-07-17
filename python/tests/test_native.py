"""Native (pybind11) backend: in-process fit parity against the trusted reference oracle.

Requires the compiled `suprafit._core` module (built with `cmake -DSUPRAFIT_PYBIND=ON`); the whole
module is skipped when it is absent, so the pure-Python test run is unaffected.

The native backend fits in-process; on the reference dataset it must recover exactly the fitted
constants the reference stores (the same numbers the CLI backend produces — the two are
interchangeable). Post-fit statistics are Phase 3, so requesting them must fail cleanly rather than
crash. Claude Generated.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest

np = pytest.importorskip("numpy")
pytest.importorskip("suprafit._core", reason="native module not built (cmake -DSUPRAFIT_PYBIND=ON)")

import suprafit as sf

_REF = (Path(__file__).resolve().parents[2]
        / "data" / "samples" / "reference" / "simulated_1_1.json")

pytestmark = pytest.mark.skipif(not _REF.exists(), reason=f"reference fixture missing: {_REF}")


def _table(block: dict):
    """Reconstruct a 2D array from a reference DataTable JSON block (space-separated row strings)."""
    data = block["data"]
    rows = sorted(data.keys(), key=int)
    return np.array([[float(x) for x in data[k].split()] for k in rows])


@pytest.fixture(scope="module")
def reference_arrays():
    d = json.loads(_REF.read_text())["data"]
    return _table(d["independent"]), _table(d["dependent"])


def test_native_fit_recovers_reference(reference_arrays):
    indep, dep = reference_arrays
    sf.set_backend("native")
    try:
        proj = sf.Project.from_arrays(indep, dep)
        proj.add_model("nmr_1_1")
        proj.fit(nproc=2)
        m = proj.model("nmr_1_1")
    finally:
        sf.set_backend("cli")
    assert m is not None
    # The reference stores lg K = 2.8957 and SSE = 0.012710165623140661 for the 1:1 model.
    assert [float(v) for v in m.global_parameters] == pytest.approx([2.8957], rel=1e-5)
    assert m.sse == pytest.approx(0.012710165623140661, rel=1e-6)
    assert m.converged is True


def test_native_matches_cli_backend(reference_arrays):
    if not _cli_available():
        pytest.skip("suprafit_cli not found (set SUPRAFIT_CLI_PATH or build debug)")
    indep, dep = reference_arrays

    def fit(backend: str):
        sf.set_backend(backend)
        try:
            proj = sf.Project.from_arrays(indep, dep)
            proj.add_model("nmr_1_1")
            proj.fit(nproc=2)
            return proj.model("nmr_1_1")
        finally:
            sf.set_backend("cli")

    native, cli = fit("native"), fit("cli")
    assert native.sse == pytest.approx(cli.sse, rel=1e-6)
    assert np.allclose(np.asarray(native.global_parameters),
                       np.asarray(cli.global_parameters), rtol=1e-6)
    assert np.allclose(np.asarray(native.local_parameters),
                       np.asarray(cli.local_parameters), rtol=1e-6)


def test_native_postprocessing_fails_cleanly(reference_arrays):
    indep, dep = reference_arrays
    sf.set_backend("native")
    try:
        proj = sf.Project.from_arrays(indep, dep)
        proj.add_model("nmr_1_1")
        proj.monte_carlo(steps=20)
        with pytest.raises(NotImplementedError, match="Phase 3"):
            proj.fit(nproc=2)
    finally:
        sf.set_backend("cli")


def _cli_available() -> bool:
    try:
        sf._cli.find_cli()
        return True
    except sf.SupraFitNotFoundError:
        return False
