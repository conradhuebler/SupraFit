<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - record of the multiplet signals of the NMR import
-->

# NMR import: multiplet signals (2026-11-21)

Plan: [roadmap/input_data_editing.md](../roadmap/input_data_editing.md). Branch `feature/input-data-editing`.
Status: 🤖 AI-generated, ⚙️ machine-tested (synthetic spreads, the operator's two JEOL series, offscreen renders). No click-through by a human, no real screen.
Figures are the **worst case over the stated spread** (debug build), printed by `test_nmrmultiplet` / `test_jeol` with `qInfo` and pinned there.

## Problem and design

A multiplet (d, t, dd) is one nucleus; its fast-exchange shift is the **centre** of the group. Following single lines fails when the centre moves more than a line spacing per step (lines lost or swapped). An `NmrSignal` can now be a multiplet: `type` Multiplet, `halfSpan` (lines within ± h of the centre), `pattern` (hint only). Rules in the class description of `NmrHandler`, step 3b:

- Region [min(last, prediction) - (h + w), max(...) + (h + w)]; candidates = peaks there (window rule of step 3a with the window threshold). Every run of candidates within 2h is a group; maxima below `multipletRipple()` (5 %) x the highest line of the run (ripple) or with a prominence below 6 σ (noise on a line) do not count. Height for the ripple and prominence for the noise: barely resolved lines of a dd keep their height but have a shallow dip (a prominence-only rule dropped them on the real H7/H4 and lost the group).
- Reference spectrum: the lines within ± h of the defined position. Later: cost |c - prediction|/w + |ln(share/share_last)| + |n - n_last| + |spread - spread_last|/h; share = height of the group / highest group of the region (the intensity of whole experiments jumps up to 8x: the absolute height rejected the real H7 in 3 spectra), J does not change, so a group with a foreign line or without one of its own costs more. MinHeight applies to the share.
- Conflict: a shared line, interleaving spans, or a line of one multiplet within ± h of the other's centre (two multiplets that took each other's lines).
- Centre estimators: (a) centroid above the chord of the region over [outer lines ∓ e], (b) midpoint of the outer lines, (c) height-weighted mean of the lines; lineshape fit: area-weighted mean of the fitted lines of the group (one line per line, shared width with "Shared"). **Default (b)**: smallest errors in every synthetic class and against the operator's MestReNova centres (tables below). Its weakness: a partly resolved outer pair moves it by up to half the pair spacing; (a) is less sensitive to that but biased by asymmetric intensities and neighbour tails.
- `pickMultiplets`: single linkage of the reference lines (gap ≤ J), ripple and lines < 6 σ dropped and the groups formed again; h = spread/2 + clamp(spread/4, 0.1 J, 0.5 J).
- Single peaks: code path and numbers unchanged (NmrHandlerTest, LineshapeFitTest, the JEOL real pins: qInfo output identical except times); JSON keys `Type`/`HalfSpan`/`Pattern`, `MultipletCentre`/`MultipletRipple` only with a multiplet.

## Synthetic spread (`test_nmrmultiplet`, 1458 cases, 10 steps, 1:1 lg K 3.5)

d / t / dd (second J 0.45 J) at J 3/8/15 Hz (500 MHz), largest step of the centre 0.2/1/3 x the closest line spacing, Lorentz FWHM 2/6/20 points (0.0002 ppm grid), noise 0/2/5 % of the highest line (smoothing step for noise), neighbour d (J 7 Hz) moving in parallel at none/1.25/2.5/5 mean spans (at least 2.5 steps away), roof 0.3 (intensities bent towards the neighbour, positions unchanged) at 1.25/2.5. Classes: separated (912: no neighbour or its lines ≥ 3 FWHM away), close neighbour (60), roof (486). Lineshape fit on the corners (J 3/15, step 0.2/3, FWHM 2/20, noise 0/5 %: 288 cases). lg K only where the shift moves ≥ 3 FWHM.

| method | separated: open cells, worst / mean centre error, worst \|Δ lg K\| | close: open cells, worst | roof: open cells, mean |
|---|---|---|---|
| centroid (a) | 0 of 15810, 7.62 / 0.231 FWHM, 0.342 | 18 of 1200, 1.80 FWHM | 552 of 9720, 3.16 FWHM |
| **outer midpoint (b)** | **0, 2.10 / 0.027 FWHM, 0.257** | **20, 2.03 FWHM** | **112, 0.10 FWHM** |
| line mean (c) | 0, 9.31 / 0.161 FWHM, 0.307 | 18, 1.79 FWHM | 444, 3.51 FWHM |
| lineshape fit | 39 of 3080, 13.4 / 0.359 FWHM, 0.821 | 67 of 280, 2.21 FWHM | 167 of 1920, 3.57 FWHM |
| one signal per line (old) | 34.7 % of the line cells lost, 9.1 % swapped (355 of 912 cases clean) | 27.6 % / 6.3 % | 29.5 % / 8.3 % |

- The worst |Δ lg K| of (b) (0.257) is t J 3 Hz, FWHM 20 points, noise 5 %: unresolved lines, shift 6 FWHM; noise-limited, not a tracking error. The worst centre errors are 2-point lines at 5 % noise (outer line at the detection limit).
- Strong coupling (exact AB, J 7 Hz, Δν/J 1.5-10): (b) lies outside the shift by C - Δν/2 (+1.06 Hz at 1.5, +0.18 Hz at 10), (a)/(c) inside by Δν/2 - Δν²/4C (-0.88 / -0.17 Hz); measured within 0.031 Hz of the formulas. A moving from Δν/J 2 to 10: |Δ lg K| 0.007 (a), 0.009 (b), 0.008 (c).
- Empty windows: 0 of 1600 multiplet cells Ok (noise 1/5 %, flat or beside a 200x line, FWHM 2/20 points, h 0.005/0.02 ppm, 10 seeds); `pickMultiplets` on noise: 0 in 80 spectra. 3 % sinusoidal ripple over a d, t, dd: one signal per peak 9 signals, grouped exactly 3 multiplets with 2/3/4 lines.
- Pins: separated 0 open, worst < 2.6 FWHM, mean < 0.04 FWHM, |Δ lg K| < 0.32; close ≤ 25 open, < 2.6 FWHM; roof ≤ 140 open, |Δ lg K| < 0.6; fit ≤ 50 open; AB formulas < 0.1 Hz; false multiplets 0.

## Real series (`test_jeol`, `realSeriesMultiplets*`)

No helpers: LB 0.3 Hz (default of the series) and 1 Hz, no steps (no Normalise, Range, Blank), search-window threshold, MinHeight 0.3; referencing to CHCl3 7.26 kept. By hand: one multiplet per reference column around the main lines of spectrum 1 (≥ 45 % of the highest within ± 12 Hz, h = spread/2 + 1.5 Hz, window 0.02 ppm; H2/H4/H7 dd, H6 t, L s). Automatic: `pickMultiplets` 5.25-9.05 ppm, the multiplet nearest each reference centre.

| series | LB | signals | cells | mean / worst \|δ - ref\| [ppm] | worst [FWHM] | lg K (ref. refitted) |
|---|---|---|---|---|---|---|
| T1 | 0.3 | by hand (b) | 75/75 | 3.5e-4 / 2.2e-3 | 0.67 | 2.646 (2.688) |
| T1 | 0.3 | automatic J ≤ 10 Hz (b) | 75/75 | 3.2e-4 / 2.2e-3 | 0.67 | 2.638 |
| T1 | 0.3 | automatic J ≤ 20 Hz (b) | 75/75 | 1.3e-2 / 7.0e-2 | 22 | 2.579 (H6 inside a 6-line aromatic group) |
| T1 | 1.0 | by hand (b) | 75/75 | 2.0e-4 / 2.0e-3 | 0.36 | 2.670 |
| T5 | 0.3 | by hand and automatic J ≤ 10/20 Hz (b) | 60/60 | 1.0e-4 / 8.1e-4 | 0.22 | 2.893 (2.898) |
| T5 | 1.0 | by hand (b) | 60/60 | 4.1e-4 / 1.7e-3 | 0.28 | 2.920 |
| before: LB 3 Hz, one signal per resolved line, Normalise + Range + PickPeaks | | | T1 75, T5 60 | T1 2.3e-4 / 1.7e-3, T5 1.9e-4 / 5.9e-4 | | 2.669 / 2.892 |

- Other estimators, by hand at LB 0.3: T1 (a) 9.3e-4 / 3.2e-3, lg K 2.609; (c) 1.2e-3 / 3.4e-3, 2.691; fit 1.4e-3 / 6.2e-3, 2.587. T5 (a) 3.4e-4 / 1.2e-3, 2.880; (c) 2.5e-4 / 1.2e-3, 2.890; fit 59/60 cells.
- T1 worst cell is H7 step 3, the reference's own outlier (as before). T1 |Δ lg K| 0.042 at LB 0.3: H4 and H7 show their inner pairs (1.1-1.4 Hz) as one or two maxima from step to step, which moves the outer midpoint by up to 0.75 Hz; H7 shares its region with lines of other signals. 27-35 of 60-75 cells carry a line-count warning for the same reason (a dd seen as 2-3 lines).
- T1 LB 1 automatic: J ≤ 20 Hz 60/75 cells (H7 picked as a singlet), J ≤ 10 Hz 75/75 but worst 9.5e-3 (printed, not pinned).
- Pick of the field report, T5 7.5-9.0 ppm, window 0.05: one signal per peak 18 signals, 206 conflict cells; grouped (J ≤ 20 Hz) **3 multiplets, 0 conflict, 0 open cells** (LB 1: 11 / 78 → 4 multiplets, 0 conflicts, 8 open: a small group at 7.717).
- Track time (debug, whole spectra, 5 / 4 multiplets): T1 0.27 s, T5 0.43 s with peak picking (one signal per line before: 1.0 / 2.1 s); lineshape fit 4.7 / 8.6 s. The centroid integral is computed for every candidate group only with the centroid estimator (else for the chosen group): this took the tracking from 2.5 / 3.0 s to the numbers above, results identical.
- Pins: by hand and automatic (J ≤ 10 Hz; T5 also 20 Hz) at LB 0.3: every cell, worst < 2.5e-3 ppm, |Δ lg K| < 0.06; LB 1 by hand < 0.03; T5 pick 0 conflict cells.

## Dialog

`benchmark_nmrimport T5` (debug, offscreen, run 1) against the "debug after" column of [nmr_dialog_speed_2026-11-20.md](nmr_dialog_speed_2026-11-20.md): load 3703 (3904), reference offset 502 (511), pick whole spectrum 545 (558), pick search window 3400 (3491), refresh 390 (410), double-click 546 (548), lineshape fit 25901 (27034), LB 2 Hz 5210 (5314) ms: no regression; new "Pick, grouped into multiplets" 3024 ms (6 multiplets in 6.5-9.5 ppm).

Signal table Name / Type (peak, multiplet) / δ / ± / ½ span / Pattern (editable, Type as a choice; a pattern that does not fit the reference line count is marked), "Multiplet from range" (Start..End in view, ≤ 0.3 ppm), group "Group lines into multiplets" with "J ≤" (Hz), "Spectrometer" (MHz from the FIDs), "Ripple below", "Centre" behind "..more.."; labels at the centre with a bracket over the lines. `test_nmrimport` (`multipletSignalsInTheDialog`, renders `renderMultiplets`).

## Limits and not verified

- Pattern fit with a common centre and fixed J over the series (optional): not done.
- Single linkage by J has no notion of the pattern: J ≤ 20 Hz chains the aromatic lines of T1 (7.40-7.51); the user has to lower J or define by range.
- Lineshape fit of multiplets: worse than peak picking on the real series and opens more cells in the synthetic one (lines at the bounds of half the spacing); not recommended for multiplets yet.
- Not checked: clicks, the right drag in a real chart, the dialog on a real screen, speed of the dialog with many multiplets.
