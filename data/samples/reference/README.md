# Reference fixtures for real-measuring regression tests

These are **trusted oracles**: SupraFit projects analysed with a stable, known-good version,
used by `src/tests/test_reference_projects.cpp` (`ReferenceProjectsTest`) to assert that the
current code reproduces the same numbers — actual fitted constants and post-processing
statistics, not exit codes or output substrings.

## Files
- `simulated_*.json` — the PeerJ Monte-Carlo paper datasets (Hübler C. 2022, PeerJ Analytical
  Chemistry 4:e23). Each fits all four NMR stoichiometries (1:1, 1:1/1:2, 2:1/1:1, 2:1/1:1/1:2)
  to one simulated experiment. **Slimmed**: fit parameters + per-method *summary* statistics
  (boxplot mean/median/quantiles/whiskers/stddev + confidence interval) are kept; the raw
  Monte-Carlo / cross-validation clouds and histogram bins are dropped (~22 MB → ~265 KB each).
- `make_reference_fixtures.py` — regenerates the slim `.json` from the full `.suprafit` originals.

The small `data/samples/NMR titrations/projects.suprafit` (4 single-model projects, stable
commit `8af629a`) is also consumed by the same test.

## Adding a new reference
1. In a **stable** SupraFit version, load the data, fit the model(s), and run the post-processing
   you want covered (Monte Carlo, Cross-Validation, Reduction Analysis). Save the project.
2. Slim it: `python3 make_reference_fixtures.py <dir-with-the-suprafit-file>` (extend the `FILES`
   map for a new name), or drop a small `.json`/`.suprafit` here directly.
3. `ReferenceProjectsTest` auto-discovers fixtures listed in `test_reference_projects.cpp`.

## Tolerance policy (per operator)
- Fitted parameters (lg K), SSE, and **Cross-Validation** results: tight / exact (deterministic).
- **Monte Carlo**: compared within tolerance (stochastic resampling).
- **Reduction Analysis**: exact.
