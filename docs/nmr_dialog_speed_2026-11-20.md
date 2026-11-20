<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - record of the field report on the NMR import dialog with real JEOL series
-->

# NMR import dialog: field report with JEOL series, causes and measurements (2026-11-20)

Plan: [roadmap/input_data_editing.md](../roadmap/input_data_editing.md). Branch `feature/input-data-editing`.
Status: 🤖 AI-generated, ⚙️ machine-tested (offscreen, the operator's T1/T5 series, synthetic series). No click-through by a human, no real screen.

## Report and findings

| # | report | finding | change | test |
|---|---|---|---|---|
| 1 | the shown window does not limit the automatic peak picking | confirmed: `pickSignals` searched the whole reference spectrum in both thresholds; T5, Start/End 6.5..9.5: 1 of 2 (whole spectrum) and 125 of 163 (search window) signals outside | `pickSignals(window, error, from, to)`; the dialog passes Start..End within the visible x axis (`pickRange`) | `pickSignalsInARange` (same peaks bit for bit as the whole pick, filtered), `pickInTheShownRange`, `realSeriesPickAndChart` (T5: 0 outside) |
| 2 | double-click finds signals around the clicked ppm | the signal was put exactly at the click; the tracking then took the nearest peak in the window, so list (click) and table (peak) differed | `NmrHandler::addSignalAtPeak`: the signal moves onto the peak the tracking takes for it in the reference spectrum; no free peak or a peak of another signal (conflict): stays at the click. "Add" keeps the typed value | `addSignalAtThePeak`, `doubleClickAddsSignal` (now expects the peak) |
| 3 | everything is very slow | confirmed, table below: every refresh rebuilt 15 chart series of 262144 points (a series costs CuteChart ≈ 8 ms plus its points), every retrack processed all spectra again, the FIDs were transformed one after the other | processing cache in `NmrHandler`, display envelope and reused series in `TraceEditorWidget`, reused marker/fit series in `NmrImport`, FIDs in parallel (OpenMP), LB/ph0/ph1 debounced (300 ms, only when the last transform took longer) | `processingIsCached` (cells equal a new handler bit for bit), `displayEnvelope`; NmrHandler/Jeol/LineshapeFit qInfo output identical to the commit before |
| 4 | the list of found peaks is hard to relate to the spectrum | no numbers or marks in the chart (only grey position lines) | numbered labels (= row of the signal table) at each signal's peak of the reference spectrum, rows of up to 4 so they do not overlap; selected row highlighted and brought into view; click on a label selects the row; tooltips with ppm and height. Decision: the signal table with its ticks is the candidate list (no extra dialog step) | `signalLabelsAndSelection` |
| 5 | wheel scaling disables the stack offset, reloading needed | confirmed: the wheel made the axes the user's view (`captureView`), a later stack change no longer rescaled y and moved the spectra out of the frame | the wheel keeps the axes as an automatic view; a stack change while scaled sets y to the nice range of the stack at factor 1; after a zoom the axes stay and the spectra move inside them | `stackAfterWheel` (5 cases, real wheel/drag), `realSeriesPickAndChart` (T5: every level in the frame) |
| 6 | the table does not match the peaks in the spectrum | not confirmed for the drawn spectra: every tracked δ lies within 0.31 px of the drawn peak (8 cases: stack, x offset, reference offset, both directions, zoom, envelope; T5: the drawn line reaches every tracked peak within 1 px, whole axis and 7.5..9.0). Other causes found: (a) double-click = click position (2); (b) with an x offset the signal lines and the double-click used display x, the reference spectrum is drawn shifted; (c) the pick of 163 mostly invisible candidates (1); (d) the signal list shows the search centre of the reference spectrum, the table the tracked δ per spectrum (by design) | (a) snap, (b) lines/labels at δ + x offset of the reference, double-click minus it, note "x axis: bottom trace", (c) range, (d) row tooltip names the reference peak | `tableMatchesTheChart` (8 rows), `realSeriesPickAndChart` |

## Times (offscreen, 1366x768, `benchmark_nmrimport T5`, run 1 of 3; runs 2-3 start with the remembered options of run 1 and are not comparable)

T5 = 15 FIDs, 131072 points, zero-filled 262144, auto phase each. Times until the event queue is empty (offscreen paint included; a real screen paints more).

| action | release before | release after | debug before | debug after |
|---|---|---|---|---|
| load and transform | 2542 | 414 | 40877 | 3904 |
| reference offset | 672 | 72 | 1916 | 511 |
| Start / End | 9 / 10 | 4 / 4 | 174 / 158 | 83 / 58 |
| Pick, whole spectrum | 194 | 45 | 1773 | 558 |
| Pick, search window | 9433 (163 signals) | 434 (38) | 31990 | 3491 |
| refresh, 6 signals | 1084 | 147 | 4664 | 410 |
| double-click (adds a signal) | 512 | 147 | 3165 | 548 |
| remove signal | 460 | 78 | 3074 | 258 |
| stack offset 50 % | 287 | 49 | 993 | 78 |
| wheel, one notch | 238 | 13 | 920 | 14 |
| stack offset after the wheel | 245 | 16 | 931 | 16 |
| Shift from: lineshape fit | 1517 | 1197 | 28755 | 27034 |
| Shift from: peak picking | 322 | 26 | 2953 | 201 |
| LB 2 Hz (FID transform) | 2572 | 265 | 41974 | 5314 |

T1 (65536 points, zero-filled 131072), release after: load 522, pick search window 552 (39 signals), refresh 125, wheel 10, lineshape fit 514 ms.

- Worst parameter change without FID transform and fit, release: 147 ms (refresh, double-click); pick ≤ 552 ms (T1). Not reached: the lineshape fit (1.2 s T5 release, 27 s debug) runs again on every retrack; its numbers must not change, so it was not touched (TODO).
- Parts before → after (release T5): `TraceEditorWidget::setTraces` 689 → 32 ms, `NmrHandler::track` with 6 signals 78 → 15 ms.
- No background thread: the cache makes `NmrHandler` mutable state, and the remaining long step (lineshape fit) would need a copy of the handler per run.

## Display envelope

A trace of more than 4000 points (`kDisplayPointLimit`) is drawn from the points in the visible x range plus one neighbour per side; above 4000 of them from the lowest and highest point of each pixel column (≤ 2000 columns, so ≤ 4002 points, below CuteChart's OpenGL switch at 5000). Extrema keep their x and y; tables, tracking and fit use every point. Recomputed after a zoom, a host x range or a resize; the indices are cached per range and x shift, so the wheel and y offsets only map them. Side effect: offscreen grabs now show the whole real series (OpenGL series stayed empty).

## Not verified

Real screen painting and its speed, real clicks and wheel, touchpads, HiDPI (device pixel ratio enters the column count), labels in crowded multiplets (they stack in 4 rows, then overlap), T1/T5 in the debug build of the GUI on the operator's machine.
