<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - record of work packages WP0-WP5 of roadmap/input_data_editing.md
-->

# Input data editing: trace pipeline, NMR import, lineshape fit (2026-11-08)

Plan and decisions: [roadmap/input_data_editing.md](../roadmap/input_data_editing.md). Branch `feature/input-data-editing`.
Status of everything below: 🤖 AI-generated, ⚙️ machine-tested (synthetic data, offscreen renders). No click-through by a human, no real NMR data yet.
Accuracy figures are the **worst case over the stated spread**, printed by the tests with `qInfo` and pinned there.

## Layout after the branch

| Layer | Where | Role |
|---|---|---|
| `XYTrace`, `TraceSet`, `TraceOps`, `TraceJson`, `TraceIO` | `src/core/trace/` | raw curve, ordered series, non-destructive step pipeline, lossless JSON, ASCII/CSV readers |
| `LineshapeFit` | `src/core/trace/` | model-free pseudo-Voigt fit (single spectrum or series) |
| `SpectraHandler`, `NmrHandler`, `ThermogramHandler`/`ItcProcessor` | `src/core/` | UV-Vis spectra, NMR titration, ITC; each builds a `DataTable` |
| `TraceEditorWidget` | `src/ui/widgets/` | chart, range, editable step list; renders only, the host stores the list |
| `SpectraImport`, `NmrImport`, `Thermogram` | `src/ui/dialogs/` | dialogs of the three paths (Thermogram keeps its own chart) |

## WP0 - spectra bugs (`SpectraHandler`)
- Fixed: the last stored spectrum was dropped on load (`i < count`, keys are 1..Count); `XEnd` was read from `XStart`; `VarCovarSelect` dereferenced `end()` (returned nothing with spectra loaded in the debug build).
- The fixed `/1000` of `CompileSimpleTable` is the stored setting `IntensityDivisor` (default 1000, old files unchanged).
- Checked and left: `PrepareMatrix` uses `x_min`/`x_max` on purpose (common range of all spectra; the selected range is applied in `VarCovarSelect`).
- libpeakpick faults (`X(-1)`, `baseline.h:484`, `nxlinregress.h:56`, `getRangedSpectrum` on an empty vector) were **not** touched: they move thermogram numbers (Track B, TODO.md).

## WP1 - trace building blocks
- `XYTrace`: ascending or descending axis, nearest index by binary search (ties to the lower index), interpolation never extrapolates (NaN outside), exact at a node.
- `TraceSet`: stable ids, `commonRange`, `resampleToCommonAxis` (spectra of another sampling are interpolated, not dropped).
- `TraceOps`: Range, Savitzky-Golay, Normalise, polynomial Baseline (excluded ranges), PickPeaks, later ShiftAxis; JSON `TraceOpList` with `"Version"`, unknown types and newer versions refused; own Eigen code, `PeakPick::spectrum` is not used.
- Raw vectors in JSON: shortest round-trip digits as a blank-separated string (old SupraFit still reads it; the old 6-digit strings and JSON arrays are read too). About 20.5 characters per full-precision value.
- `ToolSet::LoadTableFile` skips empty lines: a table file without final newline keeps its last spectrum.
- `VarCovarSelect`: stops when candidates run out, reaches every x of equal variance, picks the cluster member of largest variance.
- Worst cases (`test_trace`): peak picking over sigma 1-8, step 0.5, sub-grid offsets - position 0.0059 sigma, height 0.40 %, area 0.059 %; Savitzky-Golay cubic exact to 3.9e-14, area change 1.5e-6 noise-free (1.5e-3 at 1 % noise), peak height 1.2 %; baseline |da| 1.3e-5 noise-free, with noise 0.16 and |db| 7.8e-4 per unit of noise; resampling a Gaussian band (step 8 to 4) 0.062 against the bound h²/8·|f''| = 0.08.

## WP2 - `TraceEditorWidget` and the spectra dialog
- Chart (raw or processed), range by Start/End or right-button drag (descending ppm axis supported), step list (add, remove, move, switch off, parameter form, core messages for refused parameters), PickPeaks markers.
- `SpectraHandler` applies a `TraceOpList` (key `TraceOps`, only written when not empty) before table, matrix and VarCovar; raw spectra unchanged. `TraceOp::enabled` (`"Enabled": false`).
- Fixed on the way: a table file is read again without raw data (`TableFile`; the spectra came back empty before); the dialog hands over its spectra without a concentration table and keeps the selected range; spectra list colours; x lines after zoom; dialog fits 1366x768.
- `ImportData` needs no Spectrum branch: Edit Data opens the dialog directly.

## WP3 - NMR titration import (`NmrHandler`, `NmrImport`)
- Storage: a Spectrum project with `"Profile": "nmr"` (no new `DataType`); the spectra keys stay those of `SpectraHandler`, so `SpectraHandler::LoadData` reads them (tested; not tried with an old binary). Settings stored: TraceOps, ReferenceOffset, ReferenceSpectrum, Window, MinHeight, Signals, Manual, Excluded; without raw data the files are read again (`Sources`).
- Tracking (fast exchange, one averaged signal moves): signals are defined in a reference spectrum; per spectrum a peak is looked for within ±window around the last position **or** the linear prediction; candidates below `MinHeight` (0.3) times the last height are dropped; choice by cost |δ - prediction|/w + |ln(height / last height)|. No candidate: `missing` (NaN, never interpolated); two signals on one peak: `conflict`; manual cells feed the tracking. `shiftTable`/`importTable` refuse a table with open cells and list them (10 + "... and N more").
- Why the cost function: the plain "nearest peak to the prediction" followed noise maxima at 5 % noise and lost signals after large first steps.
- Worst case over 192 synthetic 1:1 titrations (lg K 3.5, FWHM 0.002-0.02 ppm = 2-20 points, Lorentz and pseudo-Voigt η 0.5, noise 0-5 % of the height, 10/20 steps, neighbour line 0.015-0.05 ppm); every cell tracked: position 0.237 FWHM (0.0047 ppm), |Δ lg K| 0.0085, limiting shifts 0.0022 ppm.
- Detection threshold per search window (option, 2026-11-20): [nmr_window_threshold_2026-11-20.md](nmr_window_threshold_2026-11-20.md).

## WP4 - lineshape fit
- Pseudo-Voigt with a common FWHM, Gauss and Lorentz part each area-normalised (area exact), optional constant/linear baseline; damped Gauss-Newton with analytic Jacobian, bounds by projection/active set (no NaN), fixed parameters, covariance (J^T J)^-1 s² with rank check (`Degenerate` instead of NaN errors). Series fit with shared width/η per signal (`shared`); J^T J built per block because the stacked dense J exceeded the 300 s test watchdog.
- `NmrHandler` mode "Lineshape fit": groups of overlapping signals, neighbour lines from peak picking, conflict cells get one line each so the fit can separate them, `fit failed` instead of a silent fallback (the peak-picking δ only on request). Only the settings are stored (`"LineshapeFit"`); results are recomputed on load (bit-identical, tested).
- Checks: Jacobian vs numerical differences ≤ 8.3e-8; area vs Simpson 2.7e-14; η = 0 and 1 exact.
- Single line, 270 cases (2-20 points/FWHM, noise 0-5 %, η 0-1), worst case: position 0.058 FWHM (peak picking 0.109); area 26 % and width 21 % relative, both at 2 points/FWHM and 5 % noise.
- Two lines, worst position error in FWHM (only cases where peak picking resolves both lines):

| spacing | fit | peak picking | peak picking does not resolve |
|---|---|---|---|
| 1.5 FWHM | 0.092 | 0.146 | - |
| 1 FWHM | 0.179 | 0.938 | 28 of 72 |
| 0.75 FWHM | 0.342 | 0.669 | - |
| 0.5 FWHM | 0.24 | 0.316 | 58 of 72 |

  2 of 432 fits end `Degenerate` (3 points/FWHM, 5 % noise, spacing ≤ 0.75 FWHM).
- σ(δ) coverage over 400 noise realisations: single line 0.680 within 1σ / 0.953 within 2σ, pair 0.708 / 0.948. Shared vs free width: rms δ error 0.021 vs 0.038 FWHM.
- End to end (96 titrations, neighbour 0.75-1.5 FWHM): fit position 0.073 FWHM, lg K 0.0027 (pinned); peak picking 0.185 FWHM, 0.0056.
- At 0.5 FWHM the tracking gives the signal the neighbour's maximum and the fit inherits the wrong start: free mode returns no table in 4 of 24 cases, 1 wrong by 12 FWHM (peak picking: one 6.3 FWHM outlier). Not pinned. Feeding the fit positions back into the tracking made it worse and was reverted.

## WP5 - thermogram (behaviour preserving)
- `ThermogramHandler` holds the raw curve as `XYTrace`; libpeakpick gets a `PeakPick::spectrum` built from it; chart series are computed on request; `StoreRawData` is read in one place (`setStoreRawData`); the stored `"Thermogram"` x/y were rounded to 6 digits and are lossless now (old blocks load with the same doubles).
- `ThermogramWidget::setUi()` (~530 lines) split into six steps; dead code removed (`Thermogram::LoadITCFile`, about 30 unused members).
- **Constructor crash fixed**: the dialog seeded the scaling factor before its table and series existed (since C9, July 2026); the call moved to the end of `setUi()`. Reproduced on the old code by the agent only (SIGSEGV in `QXYSeries::clear`), not by hand.
- Numbers pinned: `test_thermogram_characterization` (golden file written from the old implementation, 10 cases, relative 1e-9: integrals raw and scaled, peak indices, baseline coefficients, series fingerprints, fit parameters, net heat, calibration ratio), `testAbsoluteIntegralsPinned` unchanged; offscreen `test_thermogram_dialog`.
- Not done on purpose: integration/baseline on `TraceOps` (changes numbers, Track B); chart on `TraceEditorWidget` (the editor always selects horizontally and rebuilds series on every update; the thermogram zooms by rectangle, selects only in peak mode, keeps its series and needs keys, double click and scale - the editor would need a "selection off" mode, stable series hooks and signals for double click/keys).

## Not verified
Clicks (right-drag, double click, Add menu, tick boxes, form entry), file and directory dialogs, drag and drop of the concentration table, table export, Edit Data in the running program, the whole click list for the thermogram in TODO.md, real TopSpin/MestReNova exports (the totxt header is rebuilt from memory), real noise (renders use a sinus "noise"), release-build timing, old SupraFit binaries with NMR projects.

## Tests added
`test_spectrahandler` (SpectraHandlerTest), `test_trace` (TraceTest), `test_traceeditor` (TraceEditorTest, offscreen), `test_nmrhandler` (NmrHandlerTest, ~50 s), `test_nmrimport` (NmrImportTest, offscreen), `test_lineshapefit` (LineshapeFitTest), `test_thermogram_characterization`, `test_thermogram_dialog` (ThermogramDialogTest, offscreen).
Full `ctest` (without `ComprehensiveRealData`, which does not finish within 580 s): the red ones are the same before and after the branch - `ItcAny` (aborts in `PeakPick::LeastSquares`, `mathhelper.h:262`), `DataGeneration`, `ReferenceProjects`, `LandscapeSearch` (timeout), `NeuralNetwork` (ctest artefact, green alone).
