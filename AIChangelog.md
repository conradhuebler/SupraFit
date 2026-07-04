# AI-assisted Changelog

One line per significant AI-assisted improvement (newest first).

- 2026-07-04: Split god-object `core/toolset.cpp` (1407→827): file-I/O loaders → `toolset_io.cpp`, `Print` formatting → `toolset_print.cpp`; facade `toolset.h` + namespaces unchanged, reference test green (science unchanged).
- 2026-07-04: Added the first real-measuring regression tests (`test_reference_projects`): reconstruct + re-fit trusted PeerJ-paper `.suprafit` projects and assert known lg K / SSE reproduce exactly (20 model/dataset combos), plus CV/RA post-processing reproduction for well-determined parameters (paper-methodology aware: wrong models' divergent statistics are reported, not asserted) — the D1 safety net that guards the Dx refactorings.
- 2026-07-04: De-inlined `core/bc50.h` into `bc50.cpp` (~37 solver bodies out of a header pulled into 22 TUs); declarations-only header, namespaces/signatures unchanged, verbatim move.

- 2026-07-04: CuteChart — integrated `ChartConfiguration` as the owner of the chart config state (current/pending/last JSON + export presets) via raw accessors; `ChartView` delegates storage and keeps the dialog-driven Apply/Ignore/Revert application logic (pure state relocation, behaviour-preserving). All three decomposition components are now genuinely used.
- 2026-07-04: CuteChart — completed the stalled ChartView decomposition: `ChartExporter` and `ChartAxisManager` are now genuinely wired into `ChartView` via delegation (faithful 1:1 ports; nice-numbers aligned to Qt `applyNiceNumbers`), public API unchanged (no external break); deleted dead `chartview_pimpl.*` and `chartview_refactored.h` (the latter declared a second conflicting `ChartView`).
- 2026-07-04: CuteChart — unified the series visibility API: `LineSeries`/`ScatterSeries::showLine()` both emit `visibilityChangeRequested(bool)` and the live `ChartWrapper` connects it as the single visibility authority (fixes checkbox toggling that the half-migrated `series.cpp` change had turned into a no-op).
- 2026-07-04: Split god-object `core/analyse.cpp` into a JSON compute TU (`analyse.cpp`) and a string/HTML formatting TU (`analyse_format.cpp`), facade `analyse.h` unchanged — verbatim move, behaviour-preserving (verified via golden `-x` diff).
