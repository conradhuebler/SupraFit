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


def test_native_ml_features(reference_arrays):
    """The native backend attaches a standardized C++ ML feature vector (roadmap goal #5)."""
    indep, dep = reference_arrays
    sf.set_backend("native")
    try:
        proj = sf.Project.from_arrays(indep, dep)
        proj.add_model("nmr_1_1")
        proj.fit(nproc=2)
        m = proj.model("nmr_1_1")
    finally:
        sf.set_backend("cli")
    f = m.ml_features
    assert isinstance(f, dict)
    # structural + error features, all consistent with the fit
    for key in ("sse", "rmse", "reduced_chi_squared", "aic", "aicc", "degrees_of_freedom",
                "complexity_ratio", "datapoints", "series_count", "total_parameters"):
        assert key in f, f"ml_features missing {key}"
    assert f["sse"] == pytest.approx(m.sse, rel=1e-9)
    assert f["rmse"] == pytest.approx((f["sse"] / f["datapoints"]) ** 0.5, rel=1e-9)
    assert f["reduced_chi_squared"] == pytest.approx(f["sse"] / f["degrees_of_freedom"], rel=1e-9)
    assert f["total_parameters"] == f["global_parameters"] + f["local_parameters"]
    assert f["series_count"] == 7


def test_native_model_tables(reference_arrays):
    """The native backend returns the fitted signal (calculated curve) and residuals as 2D arrays."""
    indep, dep = reference_arrays
    sf.set_backend("native")
    try:
        proj = sf.Project.from_arrays(indep, dep)
        proj.add_model("nmr_1_1")
        proj.fit(nproc=2)
        m = proj.model("nmr_1_1")
    finally:
        sf.set_backend("cli")
    sig, err = m.model_signal, m.model_error
    assert sig is not None and err is not None
    assert sig.shape == dep.shape == err.shape
    # residuals are dependent - fitted signal, summing (near, to data precision) to the SSE
    assert np.allclose(err, dep - sig, atol=1e-9)
    assert float(np.sum(err ** 2)) == pytest.approx(m.sse, rel=1e-3)


def test_native_postprocessing_in_process(reference_arrays):
    """Monte Carlo + cross-validation run in-process via the native backend (Phase 3).

    The statistics engine divides by the app-wide "threads" property; the module must set it (as the
    CLI does) or JobManager SIGFPEs. This exercises that path end-to-end."""
    indep, dep = reference_arrays
    sf.set_backend("native")
    try:
        proj = sf.Project.from_arrays(indep, dep)
        proj.add_model("nmr_1_1")
        proj.monte_carlo(steps=100, variance_source="SEy", seed=42)
        proj.cross_validation(cv_type="L0O")
        proj.fit(nproc=4)
        m = proj.model("nmr_1_1")
    finally:
        sf.set_backend("cli")
    assert m is not None
    assert "MonteCarlo" in m.statistics and "CrossValidation" in m.statistics
    # MC gives a 95% confidence interval bracketing the fitted lg K (~2.8957); stochastic -> loose.
    mc = m.statistics["MonteCarlo"][0]["parameters"]["0"]
    lo, hi = mc["confidence"]["lower"], mc["confidence"]["upper"]
    assert lo < 2.8957 < hi
    assert hi - lo == pytest.approx(0.07, abs=0.05)
    # CV is (near) deterministic here; its boxplot mean sits on the fitted value.
    cv = m.statistics["CrossValidation"][0]["parameters"]["0"]
    assert cv["boxplot"]["mean"] == pytest.approx(2.8957, abs=1e-2)


def test_native_generate_independent():
    """The equation data generator produces the expected independent grid (rows x variables)."""
    grid = np.asarray(sf.generate_independent("0.001|(X-1)*1e-4", 20))
    assert grid.shape == (20, 2)
    # X is the 1-based row index: column 0 is constant 0.001, column 1 is (X-1)*1e-4.
    assert np.allclose(grid[:, 0], 0.001)
    assert grid[0, 1] == pytest.approx(0.0)
    assert grid[5, 1] == pytest.approx(5e-4)


def test_native_live_model(reference_arrays):
    """The low-level live model handle fits the reference data and honours set_global."""
    indep, dep = reference_arrays
    m = sf.native_model("nmr_1_1", indep, dep)  # snake_case name accepted
    m.initial_guess()
    m.fit()
    assert m.converged() is True
    assert m.sse() == pytest.approx(0.012710165623140661, rel=1e-6)
    assert np.asarray(m.global_parameters()).ravel()[0] == pytest.approx(2.8957, rel=1e-5)
    assert np.asarray(m.model_signal()).shape == dep.shape
    # a low-level verb takes effect immediately
    m.set_global(1.0, 0)
    assert np.asarray(m.global_parameters()).ravel()[0] == pytest.approx(1.0)
    # and the fit re-converges to the optimum from the new start
    m.fit()
    assert np.asarray(m.global_parameters()).ravel()[0] == pytest.approx(2.8957, rel=1e-5)


def test_native_generate_dependent(reference_arrays):
    """Deterministic ground-truth generation: params -> data -> a fit recovers the params."""
    indep, dep = reference_arrays
    m = sf.native_model("nmr_1_1", indep, dep)
    m.initial_guess()
    m.fit()
    g = np.asarray(m.global_parameters()).ravel()
    local = np.asarray(m.local_parameters())

    # noise-free: the generated data is exactly the model, so a refit recovers it with ~0 SSE
    gen = np.asarray(sf.generate_dependent("nmr_1_1", indep, g, local, noise_std=0.0))
    assert gen.shape == dep.shape
    m2 = sf.native_model("nmr_1_1", indep, gen)
    m2.initial_guess()
    m2.fit()
    assert np.asarray(m2.global_parameters()).ravel()[0] == pytest.approx(g[0], rel=1e-4)
    assert m2.sse() < 1e-12

    # noise is applied and reproducible via the seed
    a = np.asarray(sf.generate_dependent("nmr_1_1", indep, g, local, noise_std=0.01, seed=7))
    b = np.asarray(sf.generate_dependent("nmr_1_1", indep, g, local, noise_std=0.01, seed=7))
    assert np.allclose(a, b)
    assert not np.allclose(a, gen)


def test_native_itc_model():
    """ITC models are usable via system parameters: generate a heat curve, then refit recovers it.

    ITC models need the experiment setup (cell/syringe concentration, cell volume, temperature) as
    system parameters; without them the heat is identically zero. This drives that whole path."""
    n = 16
    indep = np.full((n, 1), 8.0)  # µL per injection
    sysp = {"cell_volume": 202.8, "cell_concentration": 0.5,
            "syringe_concentration": 5.0, "temperature": 298}
    gen = sf.native_model("itc_1_1", indep, np.zeros((n, 1)), system_parameters=sysp)
    gen.set_global(4.5, 0)                       # lg K
    for p, v in enumerate([-38000.0, 0.0, 1.0, 1.0]):  # dH, m, n, fx
        gen.set_local(v, 0, p)
    gen.calculate()
    heat = np.asarray(gen.model_signal())
    assert np.any(np.abs(heat) > 1.0), "ITC heat is zero — system parameters did not load"

    fit = sf.native_model("itc_1_1", indep, heat, system_parameters=sysp)
    fit.initial_guess()
    fit.fit()
    assert fit.converged() is True
    assert fit.sse() < 1e-6
    assert np.asarray(fit.global_parameters()).ravel()[0] == pytest.approx(4.5, rel=1e-4)
    assert np.asarray(fit.local_parameters()).ravel()[0] == pytest.approx(-38000.0, rel=1e-3)


def test_project_itc_native():
    """ITC fits run through the transparent Project + native backend when system parameters are given."""
    n = 16
    indep = np.full((n, 1), 8.0)
    sysp = {"cell_volume": 202.8, "cell_concentration": 0.5,
            "syringe_concentration": 5.0, "temperature": 298}
    gen = sf.native_model("itc_1_1", indep, np.zeros((n, 1)), system_parameters=sysp)
    gen.set_global(4.5, 0)
    for p, v in enumerate([-38000.0, 0.0, 1.0, 1.0]):
        gen.set_local(v, 0, p)
    gen.calculate()
    heat = np.asarray(gen.model_signal())

    sf.set_backend("native")
    try:
        proj = sf.Project.from_arrays(indep, heat, system_parameters=sysp)
        proj.add_model("itc_1_1")
        proj.fit(nproc=2)
        m = proj.model("itc_1_1")
    finally:
        sf.set_backend("cli")
    # system parameters set on the shared DataClass propagate to the models fitModelsToData creates
    assert m is not None
    assert m.converged is True
    assert m.sse < 1e-6
    assert np.asarray(m.global_parameters).ravel()[0] == pytest.approx(4.5, rel=1e-4)


def test_read_itc_and_fit():
    """A real .itc thermogram reads into (volumes, heats) + system parameters and fits an ITC model."""
    itc = Path(__file__).resolve().parents[2] / "data" / "samples" / "itc" / "sample.itc"
    if not itc.exists():
        pytest.skip(f"sample .itc missing: {itc}")
    r = sf.read_itc(str(itc))
    indep, dep = r["independent"], r["dependent"]
    assert indep.shape[0] == dep.shape[0] > 0
    assert indep.shape[1] == 1 and dep.shape[1] == 1
    assert np.any(np.abs(dep) > 1.0)  # real integrated heats, not zeros
    sysp = dict(r["system_parameters"])
    assert "cell_volume" in sysp and "temperature" in sysp
    # the .itc file carries the instrument setup but not the sample concentrations; add them to fit
    sysp["cell_concentration"] = 0.5
    sysp["syringe_concentration"] = 5.0
    m = sf.native_model("itc_1_1", indep, dep, system_parameters=sysp)
    m.initial_guess()
    m.fit()
    assert m.converged() is True
    assert np.isfinite(m.sse())


def _cli_available() -> bool:
    try:
        sf._cli.find_cli()
        return True
    except sf.SupraFitNotFoundError:
        return False
