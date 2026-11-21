<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - record of the detection threshold per search window of the NMR tracking
-->

# NMR tracking: detection threshold per search window (2026-11-20)

Plan: [roadmap/input_data_editing.md](../roadmap/input_data_editing.md). Branch `feature/input-data-editing`.
Status: 🤖 AI-generated, ⚙️ machine-tested (synthetic spreads, the operator's two real JEOL series, offscreen renders). No click-through by a human.
Figures are the **worst case over the stated spread**, printed by `test_nmrhandler` / `test_jeol` with `qInfo` and pinned there (debug build).

## Problem and option

The default peak picking of the tracking (no PickPeaks step) keeps maxima above min + 0.05 (max - min) of the **whole** spectrum: next to TMS or CHCl3 small signals stay below it and every cell is "missing".
`NmrThresholdScope` (`"ThresholdScope"`, written only for `Window`) chooses the reference: `Spectrum` (core default, projects without the key, all earlier numbers bit-identical) or `Window`. The dialog offers "Detection threshold" (whole spectrum / search window) and starts a new series with "search window". A PickPeaks step keeps its own threshold in both modes.

## Rule (class description of `NmrHandler`, step 3a)

- Region per signal and spectrum: [min(last, prediction) - w, max(last, prediction) + w]; σ = `TraceOps::noiseLevel` of the processed spectrum: 1.4826 · median over blocks (max(64, n/32) points) of the MAD after removing a least-squares parabola per block (robust against lines and wings; first differences would underestimate apodized, correlated noise).
- A local maximum is a candidate if **h_chord ≥ max(0.05 H, 6 σ)** or **h_prom ≥ max(0.05 H, 10 σ)**: h_chord = height above the straight line through the medians of the outer tenths of the region widened by w on both sides; h_prom = height above the higher of the lowest points on either side (walked until a higher point); H = largest of both in the region.
- Why two measures: the chord keeps a line overlapping a neighbour (small prominence) but lies above the convex wing of a strong line; the prominence keeps a line on such a wing. A noise maximum is about 3-4 σ above the chord and 5-7 σ above its side minima, hence 6 σ / 10 σ (Gaussian: P(> 6 σ) ≈ 1e-9 per point).
- MinHeight, the cost |δ - prediction|/w + |ln h/h_last| and conflict/missing are unchanged.
- First versions failed in the measurement and were changed: chord without the widening (a peak at the region edge raised its own baseline: H1 lost after a fast step), chord only (signals on a 200x wing lost), σ from the plain block MAD (the slope of a broad wing counted as noise).

## Synthetic measurements (`test_nmrhandler`)

| test | spread | whole spectrum | search window |
|---|---|---|---|
| dominant standard | 216 cases: 20/200/1000 x, as narrow as the lines or 0.03 ppm wide, at 7.97 (edge) or 7.78 ppm (0.18 ppm from H1), FWHM 0.002-0.02 ppm, noise 0-5 % of the small lines, neighbour 0.015/0.05 ppm, 10 steps | 525 of 6480 host cells (8.1 %), no case complete | 206 complete; worst 0.195 FWHM (0.0027 ppm), \|Δ lg K\| 0.0158, limiting shifts 0.0020 ppm |
| no standard (endToEndSpread spread, default picking) | 192 cases | 192 complete, 0.167 FWHM, \|Δ lg K\| 0.0082 | identical |
| empty windows | vanished signal + signal where no line is, flat and on a 200x wing, noise 0-5 %, FWHM 0.002-0.02 ppm, 10 seeds each, MinHeight 0.3 and 0 | noise peaks taken in 1328 of 11400 windows (11.6 %; MinHeight 0: 13.6 %) | **0 of 11400** (0 of 5700 with MinHeight 0); 0 of 15600 host cells lost |
| rules | crossing (conflict), vanishing, jump of 2.4 windows in step 1, reference in the middle; noise 0/1/2 % | | same status in every cell, noise-free also the same δ |

- The 10 incomplete window cases: H1 starts 6-9 standard widths from a 1000x line (steep wing): a shoulder without a maximum, or a prominence below 10 σ at 5 % noise. Pinned: at most 10, only this class.
- Pins: position 0.25 FWHM, \|Δ lg K\| 0.025, limiting shifts 0.003 ppm, 0 false peaks; the `Spectrum` pins of endToEndSpread are unchanged (0.237 FWHM, 0.0085, 0.0022 ppm measured again).
- Seen once during development with a 1000x wing (not in the final spread): after a signal vanished, its linear prediction ran onto the standard itself. That is the existing tracking rule (no upper height limit), not the threshold.

## Real series (`test_jeol`, `realSeriesWindowThreshold`)

LB 3 Hz, one signal per resolved multiplet line (as before), **no steps** (no Normalise to CHCl3, no Range, no PickPeaks); referencing to CHCl3 7.26 kept.

| series | threshold | MinHeight | multiplet cells | mean / worst \|δ - ref\| | lg K (ref. refitted) |
|---|---|---|---|---|---|
| T1 | whole spectrum | 0.3 | 0 of 75 | - | - |
| T1 | search window | 0.3 | 75 of 75 | 2.3e-4 / 1.7e-3 ppm (lineshape fit 3.1e-4 / 1.6e-3 = 0.16 FWHM) | 2.669 (2.688) |
| T5 | whole spectrum | 0.3 | 10 of 60 | - | - |
| T5 | search window | 0.3 | 44 of 60 | 1.7e-4 / 5.4e-4 ppm | - |
| T5 | search window | 0.1 | 60 of 60 | 1.9e-4 / 5.9e-4 ppm (fit 1.3e-4 / 4.7e-4 = 0.05 FWHM) | 2.892 (2.898) |

- With the window threshold and no steps the numbers equal those of the earlier setup with Normalise + Range + PickPeaks (docs/jeol_import_2026-11-20.md).
- The 16 open T5 cells come from MinHeight, not from the threshold: without the normalisation a line's height jumps up to 8x between the experiments. Pinned: MinHeight 0.3 at most 16 open, MinHeight 0.1 none; worst < 2.5e-3 ppm, \|Δ lg K\| < 0.03.
- Time of `track()` (debug build, whole spectra of 262144 points): peak picking 1.0 s (T1) / 2.1 s (T5), lineshape fit 1.6 / 3.1 s; with the earlier Range step 0.5 / 1.1 and 0.8 / 1.8 s. The rule for neighbour lines of the fit runs only near the tracked signals (over all maxima the fit took 4.7 / 8.2 s, same numbers).
- Not measured: LB 0.3-1 Hz without steps (`JEOL_EXPLORE=1` runs it), one signal per multiplet centre.

## Recommendation

`Window` was at least as good in every measurement (identical without a standard, far better with one, no false peaks). Switching the core default would change pinned numbers and old projects, so it is left to the operator; the dialog already starts new series with it.
