<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - record of the JEOL Delta import of roadmap/input_data_editing.md
-->

# JEOL Delta import: reader, FID processing, validation (2026-11-20)

Plan: [roadmap/input_data_editing.md](../roadmap/input_data_editing.md). Branch `feature/input-data-editing`.
Status: 🤖 AI-generated, ⚙️ machine-tested (synthetic files, the operator's 30 real files, offscreen renders). No click-through by a human.
Accuracy figures are the **worst case over the stated spread**, printed by the tests (`test_jeol`, `test_nmrimport`) with `qInfo` and pinned there.

## Code

| Part | Where | Role |
|---|---|---|
| `JeolReader` | `src/core/trace/jeolreader.*` | header, parameter records, 1D data section; every offset checked against the file size |
| `Fid`, `FidSettings`, `FidProcessing` | `src/core/trace/fidprocessing.*` | apodization, zero filling, radix-2 FFT, group delay, phase, ACME auto phase, ppm axis |
| `NmrHandler` | `src/core/nmrhandler.*` | a `.jdf` is recognised by its magic; FIDs kept, spectra re-transformed when the settings change |
| `NmrImport` | `src/ui/dialogs/nmrimport.*` | panel "Processing (FID)", ending `.jdf`, file filter, natural order of "Add files" |

## Format facts (from the 30 files of SB18, JNM-ECZ500R, Delta 5.2/5.3.3)

- Header 1360 bytes, **always big-endian**. Byte 8 is the byte order of the **parameter and data sections** (0 big, 1 little); all real files have 1 and little-endian sections. Bytes 9-11: version 1 / 2.
- Byte 12 = 1 dimension; byte 14 = data type (upper 2 bits, 0 = float64) and format (lower 6 bits, 1 = 1D); byte 24 axis type 3 = complex; bytes 32/33 axis unit (0x01, 28 = seconds): **time domain, FIDs**.
- Title at 48 (124 bytes); points at 176 (65536 for T1, 131072 for T5, other dimensions 1); valid range 208/240; axis start/stop 272/336 (0 to acquisition time); base frequency 1064 (500.1599 MHz); parameter section 1212/1216 (start 1360); data section 1284 (start 16384) / 1288 (uint64 length = 2 x points x 8); context 1296/1304 (9184 / 10304 bytes after the data: the experiment source text, `single_pulse.jxp`, not used); annotation 1308/1316; total size 1320 (uint64 = file length).
- Data: all real parts, then all imaginary parts (not interleaved).
- Parameter section: 16-byte header (record size 64, low index 0, high index 200/220, total size), then 64-byte records: 4 class bytes, unit scaler (2), five units (2 bytes: SI prefix nibble, power nibble, unit code), value (16 bytes), value type (4: 0 text, 1 int32, 2 double, 3 complex, 4 infinity), name (28 bytes, blank padded). Prefix nibble p scales by 10^(-3p) (`x90` = 6 with p = 2: 6 µs). Unit codes seen: 4 °C, 6 degree, 13 Hz, 24 %, 26 ppm, 28 s, 31 T, 35 dB. `X_SWEEP` is a double in T1 and an int32 in T5.
- Used parameters: `X_FREQ` (Hz), `X_SWEEP` (Hz, T1 9384.38 = 18.76 ppm, T5 6250), `X_OFFSET` (ppm of the carrier, 5 / 4.5), `X_ACQ_DURATION`, `ACQ_DELAY`, `SCANS`, `solvent` (CHLOROFORM-D), `temp_get` (19.1-20.6 °C), `DIGITAL_FILTER` (TRUE), `orders` ("2 54 73" / "2 28 73"), `factors` ("8 2" / "4 2").
- **Sign of the frequency**: JEOL stores the complex conjugate of the convention exp(+2πiνt) with δ increasing with ν. Without conjugation CHCl3 appears at 2.75 ppm; with it at 7.249 ppm, water at 1.554 ppm.

## Digital filter (group delay)

- About 20 points before the signal starts (|FID| rises at point 20). Delay of a cascade of linear-phase FIR stages in output points: `½ Σ_i (order_i - 1) / Π_{j≥i} factor_j` = **19.656** (T1) and **19.688** (T5); applied as the exact fractional time shift exp(+2πiν·GD/sweep) after the FFT.
- Residual first-order phase found by the automatic phase on every real spectrum (ph1 across the full width): T1 -7° to -36° (mean ≈ -24°, = -0.067 points), T5 -103.6° to -107.6° (≈ -0.29 points). Constant within a series, so the formula is close but not exact; `ACQ_DELAY` (0.062 / 0.125 points) does not explain both. Left to the automatic ph1 (open item in TODO.md).
- ph0 differs strongly between the experiments of one series (T1 -106° to -34°, T5 -121° to +93°): a common phase cannot phase the series, hence **automatic phase per spectrum is the default** (`FidSettings::autoPhase = true`); "Auto phase" (reference spectrum, common values) and ph0/ph1 by hand remain.

## Decisions

- FFT: own radix-2 (decimation in time, twiddles computed directly). The spectrum is zero-filled to a power of two anyway, so no Bluestein/mixed radix is needed.
- Automatic phase: ACME (Chen et al., J. Magn. Reson. 158 (2002) 164): entropy of the normalised first derivative + 100 · Σ(negative intensity / max|S|)². Coarse grid ph0 10° x ph1 30° (±180°) on a spectrum reduced to ≤ 8192 points by block sums, then Nelder-Mead (restarted once) from the 4 lowest grid minima on a spectrum reduced to ≤ 65536 points, the winner polished on the full spectrum; ph1 kept within ±180° by a quadratic wall. The rotation exp(iθ_k) is a recurrence (exact every 256 points), not a cos/sin per point. The penalty weight is not critical: 10 to 10000 give the same worst case (5.7°), 1 gives 7.7°.
- Persistence: `"FidProcessing"` (settings with `"Version"`) only when the series holds a FID; with `StoreRawData` also `"Fid"` (complex points, lossless) next to `"Data"` (the transformed spectra, readable without FID support); without raw data the `.jdf` files are read again. Projects without the keys are unchanged (tested).

## Tests and numbers (release build unless noted)

- Reader: both byte orders bit-identical values, float32, truncation every 97 bytes, wrong magic, offsets/lengths beyond the file (up to 2^64), 4·10^9 points, 2D/other formats/axis types refused with "not supported"; 3000 random corruptions of header and parameters: 447 refused with a message, none crashed.
- FFT vs direct DFT, n = 1..4096: worst 2.1e-16 relative to Σ|f|, round trip 4.5e-16.
- Line position (parabola of the maximum, LB 0.3 Hz) over 288 lines per factor (N 8192/32768, T2 0.1-1 s, noise 0-5 %, with/without delay, phase): zero fill x2 worst **0.113 FWHM** (0.070 Hz, at T2 1 s with a truncated 1.3 s FID), x1 worst 0.32 FWHM (under-sampled lines). Pinned 0.15 / 0.4.
- Automatic phase over 54 synthetic spectra (1-8 lines, noise 0-2 %, ph0 ±180°, ph1 ±90°): worst phase error **at a line 5.7°** (pinned 7°); ph0/ph1 separately up to 16°/46° where few lines leave them exchangeable. A phase error φ moves a Lorentzian maximum by about φ/4 FWHM (5.7° → 0.025 FWHM). ~400 cost evaluations, 31 ms per spectrum (16384 points).
- Real series: all 30 files read, same points within a series; 15 FIDs of T5 (131072 points, zero filled to 262144) read and transformed in 1.75 s (release, auto phase each), T1 1.5 s; debug build 36 s / 32 s.

## Validation against the operator's evaluation (MestReNova shifts in `SB18_T1_T5_ObDGlc_CDCl3_20C.suprafit`)

- The reference reports **multiplet centres** (H2, H4, H7 are doublets of doublets, H6 a triplet, L a singlet); referencing **CHCl3 = 7.26 ppm** reproduces the singlet L within 0.1-0.4·10^-3 ppm. The spectrometer axis puts CHCl3 at 7.2488 (T1) / 7.2478 (T5); one offset per series (from spectrum 1) leaves CHCl3 of the other spectra within 0.08-0.3·10^-3 ppm.
- Setup of the test (`realSeriesAgainstReference`): auto phase per spectrum, LB 3 Hz (merges the small couplings; at 0.3-1 Hz the lines of a dd are 1.5-5·10^-3 ppm apart and the tracking loses or swaps them), steps Normalise (largest magnitude in 7.2-7.3 ppm: the intensities of the experiments differ up to 8x, the tracking compares heights), Range 5.25-9.05, PickPeaks absolute 2e-5; one signal per resolved line of each multiplet of spectrum 1 (window 0.9 x line spacing, at most 0.004 ppm); centre = mean of the lines.
- Result, every cell tracked (0 missing/conflict), |δ - reference|:

| series | method | cells | mean | worst | lg K (reference refitted / stored) |
|---|---|---|---|---|---|
| T1 | peak picking | 75 | 2.3e-4 ppm | 1.7e-3 ppm (step 3, H7) | 2.669 (2.688 / 2.688) |
| T1 | lineshape fit | 75 | 3.1e-4 ppm | 1.6e-3 ppm | 2.699 |
| T5 | peak picking | 60 | 1.9e-4 ppm | 5.9e-4 ppm | 2.892 (2.898 / 2.898) |
| T5 | lineshape fit | 60 | 1.3e-4 ppm | 4.7e-4 ppm | 2.894 |

  Mean fitted FWHM at LB 3 Hz ≈ 0.010 ppm (merged components), so the worst is 0.17 FWHM, the mean 0.02-0.03 FWHM; against the natural width at LB 0.3 Hz (0.0033 ppm) the worst is 0.5 FWHM. The T1 worst cell is the reference's own outlier: MestReNova H7 of step 3 (7.3178) lies below steps 2 and 4, ours is monotone. |Δ lg K| ≤ 0.019 (T1) and 0.007 (T5), within the 95 % interval of the stored fit (T5 2.856-2.940). Over LB 2-3 Hz the same: worst 1.9e-3 ppm, lg K within 0.021.
- Pinned: worst shift < 2.5e-3 ppm, |Δ lg K| < 0.03.

## Limits and not verified

- Only 1D FIDs; processed JEOL spectra (frequency axis), 2D and other data types are refused. Big-endian sections only synthetic.
- Multiplets: the tracking follows single lines; lines closer than the step of a titration are lost or swapped (LB 0.3 Hz: 54-127 open cells). A multiplet signal (centre of a group of lines) is not there.
- Offscreen renders do not show series above 5000 points (the chart draws those with OpenGL); the full real spectra (15 x 262144 points) were not seen drawn, only a range of < 5000 points. Interactive speed with such series not measured.
- Not checked: clicks, file and directory dialogs, the natural order of "Add files" (sorted in the click handler), the dialog on a real screen, old SupraFit binaries with FID projects, project files with `"Fid"` (about 2 x 20 characters per point and spectrum: tens of MB for 15 FIDs of 131072 points).
