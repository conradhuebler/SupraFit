# core/trace - GUI-free trace building blocks

Basis for UV-Vis, NMR and thermogram import (`ThermogramHandler` holds an `XYTrace`, its maths stays libpeakpick until Track B); plan: [roadmap/input_data_editing.md](../../../roadmap/input_data_editing.md). Tests: `test_trace` (TraceTest).

## Layout
- **xytrace.h/cpp**: `XYTrace` value type (x, y, name, source, axis label/unit); valid = equal length, finite, x strictly monotone ascending OR descending (NMR ppm)
- **traceset.h/cpp**: `TraceSet` ordered traces with stable ids (QUuid unless given); `commonRange`, `hasCommonAxis`, `resampleToCommonAxis`
- **traceops.h/cpp**: algorithms in `namespace TraceOps` (trapezoid, Savitzky-Golay, polynomial baseline, normalise, pickPeaks) + parameter objects `RangeOp`/`SmoothOp`/`NormaliseOp`/`BaselineOp`/`PickPeaksOp` + pipeline `TraceOpList`
- **tracejson.h/cpp**: lossless vector persistence (`vectorToString`/`vectorFromJson`)
- **traceio.h/cpp**: ASCII/CSV readers (BOM, CRLF, decimal comma, tab/;/,/blank, header lines, matrix x + n columns, TopSpin totxt LEFT/RIGHT); `ToolSet::LoadXYFile`/`LoadCSVFile` stay for UV-Vis
- `ShiftAxisOp` (`"ShiftAxis"`, `Offset`): x += offset, NMR referencing
- **jeolreader.h/cpp**: JEOL Delta `.jdf` (1D only): header always big-endian, byte 8 = order of parameter/data sections; 64-byte parameter records; real section then imaginary section; layout in the header comment, findings [docs/jeol_import_2026-11-20.md](../../../docs/jeol_import_2026-11-20.md)
- **fidprocessing.h/cpp**: `Fid` (convention exp(+2πiνt), δ ascending with ν), `FidSettings` (JSON with `"Version"`), apodization, zero filling, radix-2 FFT, filter group delay as exact phase ramp, ACME auto phase, descending ppm trace; tests `test_jeol` (JeolTest)
- **lineshapefit.h/cpp** (WP4): pseudo-Voigt (common FWHM, area exact) + baseline; own damped Gauss-Newton, analytic Jacobian, bounds by projection/active set, fixed parameters, covariance with rank check; `fitSeries` stacks blocks, `shared` index = one width/η over blocks; tests `test_lineshapefit` (LineshapeFitTest)

## Rules
- Raw data are never modified: `TraceOpList::apply(const XYTrace&)` works on a copy
- Errors are returned (`bool` / empty trace + `QString* error`), never thrown; unknown op types are an error
- Interpolation never extrapolates: NaN outside [xMin, xMax]; restrict to `TraceSet::commonRange` first
- Interpolation at a node returns the node value bit for bit (resampling on the same axis changes nothing)
- Raw vectors in JSON: blank-separated string, shortest round-trip digits (`FloatingPointShortest`), still readable by old SupraFit; reading also takes the old 6-digit strings and JSON arrays
- Pipeline JSON carries `"Version"`; a newer version is refused with a message
- A switched-off step (`TraceOp::enabled = false`) stays in the list and its JSON (`"Enabled": false`, written only then); `apply` skips it
- Do not route new code through `PeakPick::spectrum` (out-of-range reads, endless `XtoIndex`); the algorithms here are own Eigen code
- Accuracy claims live in `test_trace` as worst case over a spread of widths/noise (printed with `qInfo`)
- Binary readers check every header offset/length against the file size before use; a header value never sizes an allocation alone (`JeolReader::MaxFileSize`)
- JEOL stores the conjugate of the `Fid` convention: `FidProcessing::fromJeol` conjugates; a new instrument format converts in its own `from...`
- FFT sizes are powers of two only (zero filling pads anyway); add Bluestein only if a caller needs exact other lengths
- `LineshapeFit` never returns NaN parameters: bad input is `Invalid`, singular J^T J is `Degenerate` (errors NaN); J^T J is accumulated per block, never from the stacked J (series of 20 spectra stay fast)
