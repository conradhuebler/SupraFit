# core/trace - GUI-free trace building blocks

Basis for UV-Vis, NMR and (WP5) thermogram import; plan: [roadmap/input_data_editing.md](../../../roadmap/input_data_editing.md). Tests: `test_trace` (TraceTest).

## Layout
- **xytrace.h/cpp**: `XYTrace` value type (x, y, name, source, axis label/unit); valid = equal length, finite, x strictly monotone ascending OR descending (NMR ppm)
- **traceset.h/cpp**: `TraceSet` ordered traces with stable ids (QUuid unless given); `commonRange`, `hasCommonAxis`, `resampleToCommonAxis`
- **traceops.h/cpp**: algorithms in `namespace TraceOps` (trapezoid, Savitzky-Golay, polynomial baseline, normalise, pickPeaks) + parameter objects `RangeOp`/`SmoothOp`/`NormaliseOp`/`BaselineOp`/`PickPeaksOp` + pipeline `TraceOpList`
- **tracejson.h/cpp**: lossless vector persistence (`vectorToString`/`vectorFromJson`)

## Rules
- Raw data are never modified: `TraceOpList::apply(const XYTrace&)` works on a copy
- Errors are returned (`bool` / empty trace + `QString* error`), never thrown; unknown op types are an error
- Interpolation never extrapolates: NaN outside [xMin, xMax]; restrict to `TraceSet::commonRange` first
- Interpolation at a node returns the node value bit for bit (resampling on the same axis changes nothing)
- Raw vectors in JSON: blank-separated string, shortest round-trip digits (`FloatingPointShortest`), still readable by old SupraFit; reading also takes the old 6-digit strings and JSON arrays
- Pipeline JSON carries `"Version"`; a newer version is refused with a message
- Do not route new code through `PeakPick::spectrum` (out-of-range reads, endless `XtoIndex`); the algorithms here are own Eigen code
- Accuracy claims live in `test_trace` as worst case over a spread of widths/noise (printed with `qInfo`)
