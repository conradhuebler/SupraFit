<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated
-->

# Work Package: ScriptModel Performance & Cleanup

## Context

`ScriptModel` (`src/core/models/scriptmodel.{h,cpp}`) lets users define an
arbitrary fit model from a text equation plus a declared set of global
parameters, local parameters and independent-input columns. It is created by
the factory overload `CreateModel(const QJsonObject&, QWeakPointer<DataClass>)`
(`src/core/models/models.cpp:118`) and driven, like every model, through
`AbstractModel::Calculate()` → `ScriptModel::CalculateVariables()`.

Four interpreter backends exist in the tree:

| Backend | File | Build flag | Default | State |
|---|---|---|---|---|
| ExprTk | `exprtkinterpreter.{h,cpp}` | always compiled | **de-facto active** | compiled-once, fast |
| ChaiScript | `chaiinterpreter.{h,cpp}` | `_Models` (ON) | off-path | slow, per-point string eval |
| Duktape (JS) | `dukmodelinterpreter.{h,cpp}` | `Use_Duktape` (OFF) | off | stub/experimental |
| Python | `pymodelinterpreter.{h,cpp}` | `_Python` (OFF) | off | stub, "not working yet" |
| QJSEngine (JS) | inline in `scriptmodel.cpp` | Qt Qml | off-path | per-point string eval |

Despite the JSON key being named `ChaiScript_Json`, the live evaluation path
(`CalculateVariables`) uses **ExprTk**; the ChaiScript/JS/Python routines are
dead or commented out. The same equation feature is mirrored, independently, in
`DataGenerator::Evaluate()` (`src/capabilities/datagenerator.cpp:53`) which uses
**QJSEngine** to build independent-variable columns.

## Goal

Make scripted models fast enough to be a first-class fit target (thousands of
`Calculate()` calls per fit × Monte-Carlo/CV repeats), fix the correctness gaps,
and collapse five half-built backends down to one supported, documented engine.
This document is a plan only — no production code is changed here.

## Current State (per backend)

### ExprTk — the active path
- `ExprTkInterpreter::PrepareFormula()` (`exprtkinterpreter.h:14`) builds a
  `symbol_table`, allocates one `double` per input name and per global name,
  registers them, and `parser.compile()`s the expression **once**. Gated by
  `ScriptModel::m_formula_prepared` so compile happens once per model. Good.
- Per `Calculate()`: `setGlobal()` writes global params into the bound doubles
  (single row, `matrix(0,i)`). Correct and cheap.
- Per data point (`CalculateVariables`, `scriptmodel.cpp:366-384`):
  1. allocates a `QVector<QPair<QString,double>> x_values` (heap alloc + QString
     copies) for every point,
  2. `evaluate()` does a `std::map<QString,double*>::find` (QString hash/compare,
     tree walk) for **each** input to locate the bound double,
  3. then `m_expression.value()` — the only genuinely fast part.
- Local parameters are **never bound**: `PrepareFormula` only registers input +
  global names; `m_local_variables` stays empty; `setLocal()` loops over an
  empty map (`exprtkinterpreter.h:55`). So any `B1…` local term silently
  evaluates to its default and does not participate in the fit.
- The `series` loop recomputes identical values `SeriesCount()` times because
  nothing series-dependent is bound.

### ChaiScript
- `ChaiInterpreter::Evaluate(const char*)` re-`eval()`s a **string** each call —
  parse + interpret every point. The legacy drivers `CalculateChaiV1/V2` also do
  per-point `QString::replace()` of every variable name into the source string.
  This is the classic "ChaiScript is slow" pattern, compounded by string surgery
  in the inner loop. Globals are pushed via `add_global_const`/`set_global`.
- Reached only from commented-out code; `CalculateChai()` body is disabled.
- Drags in the large `external/ChaiScript` dependency and the unused
  `CalculateThread`/`CxxThreadPool` machinery (`scriptmodel.h:37-66`).

### Duktape / Python
- Both compiled out by default. Duktape `Update()`/`Evaluate()` push and
  `duk_peval()` a string per call; the driver is hardcoded (`vmax*S/(Km+S)`),
  clearly experimental. Python init deletes/sets globals via `PyRun_SimpleString`
  and calls a `Calculate(i,j)` function per point; header comment says "not
  working yet".

### QJSEngine
- `CalculateQJSEngine()` and `PrintOutIndependent()` construct a `QJSEngine`,
  substitute variables by `QString::replace`, and `engine.evaluate(string)` per
  point. `PrintOutIndependent` at least caches results in `m_x_printout`.
- `DataGenerator::Evaluate()` uses the same pattern for independent-column
  generation (one-shot, not in the fit loop).

## Performance Findings

1. **Compile-once is already done for ExprTk** — the biggest classic mistake
   (recompiling per point) is avoided on the live path. The legacy
   `CalculateExprTk()` (`scriptmodel.cpp:581`) does the opposite: it calls
   `parser.compile()` inside the point loop — keep it as a warning, not a model.
2. **Per-point overhead is allocation + map lookup, not evaluation.** For every
   point we allocate a `QVector` and do N `std::map<QString,double*>` lookups
   (N = input count). For a 50-point × 4-series fit that is ~200 heap allocs and
   ~200·N QString-keyed tree lookups **per residual evaluation**, and an LM fit
   does dozens–hundreds of those, times MC/CV repeats. This dominates the ExprTk
   `value()` cost for small equations.
3. **Redundant series recomputation.** With no series-dependent binding, the
   `series` loop multiplies work by `SeriesCount()` for identical results.
4. **Local parameters don't work at all** (correctness bug, and it also means
   the fitter optimizes local parameters that have zero effect → wasted DoF).
5. **Backend selection is a fiction.** `DefineModel()` hardcodes `m_chai=true`,
   `m_python=false`; `CalculateVariables()` ignores those flags and always runs
   ExprTk. The `ChaiScript` JSON key is a misnomer for "equation text".
6. **No threading on the active path** despite `SupportThreads()==true` and a
   fully built (but unused) `CalculateThread`/pool. The double loop is serial.
7. **Dead weight**: ChaiScript/Duktape/Python/QJSEngine code, the
   `CalculateThread` class, and `external/ChaiScript` are compiled in for a path
   nothing calls (ChaiScript via `_Models=ON`).

### Rough backend ranking (per single point eval, small math expr)
`ExprTk (bound doubles) ≪ ExprTk (current, map+alloc) < QJSEngine ≈ Duktape ≪ ChaiScript (string eval) ; Python ≈ IPC-bound`.
ExprTk with precomputed bindings is expected to be **1–2 orders of magnitude**
faster than any string-eval backend and several× faster than the current ExprTk
path once allocation/lookup is removed.

## Improvement Proposals (ranked)

### P1 — Precomputed input bindings (kill per-point alloc + map lookup)
- **What**: In `PrepareFormula`, after registering inputs, store an ordered
  `std::vector<double*> m_x_by_column` (index = input column). Add
  `evaluateFast()` that takes the raw column values (or reads them via a stride)
  and writes directly into `*m_x_by_column[k]`, then returns `m_expression.value()`.
  In `CalculateVariables`, drop the `QVector<QPair>` and write
  `IndependentModel()->data(i,k)` straight into the bound doubles.
- **Speedup**: eliminates 1 heap alloc + N QString map lookups per point;
  expected **3–10×** on per-point overhead for typical small equations.
- **Scope**: `exprtkinterpreter.h` (+small), `scriptmodel.cpp:CalculateVariables`.
- **Effort**: S. **Risk**: Low (pure micro-optimization, same numerics).

### P2 — Wire local parameters + de-duplicate the series loop
- **What**: Register local-parameter names in `PrepareFormula`; implement
  `setLocal(matrix, series)` to write the series row into bound doubles; move
  `setLocal` inside the series loop. If a model declares **no** locals, compute
  the point column once and broadcast to all series (skip the redundant recompute).
- **Speedup**: correctness fix; for no-local models saves ~`(SeriesCount-1)×`
  work; for local models keeps cost but makes results correct.
- **Scope**: `exprtkinterpreter.{h}`, `scriptmodel.cpp`, `DefineModel` (local
  name plumbing already present).
- **Effort**: M. **Risk**: Medium — changes fit semantics; validate against a
  reference project before/after (fits with locals will change).

### P3 — Make backend selection real and default to ExprTk
- **What**: Add an explicit `"Engine"` field (default `exprtk`) to the model
  JSON; route `CalculateVariables` on it. Rename/alias the misleading
  `ChaiScript` key to `Equation` (keep read-compat for old projects).
- **Speedup**: none directly; removes confusion and lets P5 (removal) proceed.
- **Scope**: `scriptmodel.{h,cpp}`, JSON docs.
- **Effort**: S. **Risk**: Low (additive, backward-compatible read).

### P4 — Threaded / batched evaluation over the data table
- **What**: For large `DataPoints × SeriesCount`, split rows across
  `CxxThreadPool` with per-thread `ExprTkInterpreter` copies (ExprTk expressions
  are cheap to hold per thread; do **not** share one symbol_table across
  threads). Reuse the existing (now ExprTk-based) `CalculateThread` shell.
- **Speedup**: near-linear in cores for large datasets/ITC; negligible for tiny
  titration sets (guard with a datapoint threshold).
- **Scope**: `scriptmodel.cpp` (`InitialThreads`/`CalculateThread`),
  `exprtkinterpreter`.
- **Effort**: M. **Risk**: Medium (threading correctness, per-thread binding).

### P5 — Remove dead backends
- **What**: Delete ChaiScript, Duktape, Python and inline-QJSEngine evaluation
  paths for ScriptModel, plus the unused `CalculateThread`+ChaiInterpreter shell;
  drop the `_Models`/`Use_Duktape`/`_Python` model-script build options. Keep a
  single QJSEngine use only in `DataGenerator` (or migrate that to ExprTk too).
  Removes the `external/ChaiScript` dependency from the model path.
- **Speedup**: build-time + binary size + maintenance; no runtime change to the
  live path.
- **Scope**: `scriptmodel.{h,cpp}`, `chai/duk/py` interpreters, `CMakeLists.txt`,
  `src/core/CLAUDE.md`.
- **Effort**: M. **Risk**: Low (dead code) — but check no saved project selects a
  non-ExprTk engine (none is serialized today); add a migration note.

### P6 — Optional AST/JIT codegen (long-term, low priority)
- **What**: ExprTk already runs compiled bytecode; a further step is native
  codegen (e.g. an LLVM/asmjit backend or emitting a small C++ functor for
  pure-math models). Only worth it if profiling after P1/P4 still shows the
  interpreter as the bottleneck.
- **Speedup**: possibly 2–5× over ExprTk bytecode for heavy expressions.
- **Effort**: L. **Risk**: High (new dependency, platform/build complexity).
  Recommend **defer** until P1/P4 numbers justify it.

## Recommendation

- **Default and only supported backend: ExprTk.** It compiles once, has no
  per-eval parsing, no GC, is header-only, and is already the live path.
- **Deprecate and remove** ChaiScript (slow, large dep), Duktape (off/stub) and
  Python (off/stub) for scripted models. Keep QJSEngine solely for the
  independent-data generator, or fold that into ExprTk for one engine end-to-end.
- **Do P1 first** (highest value/lowest risk), then **P2** (correctness), **P3**
  (make the choice explicit and safe), then **P5** (cleanup), then **P4**
  (threading). **P6** only if profiling demands it.

## Phased Roadmap

- **Phase 0 — Baseline**: add a micro-benchmark (N points × M series × K
  Calculate calls) for the current ExprTk path; record ms/eval. Document ExprTk
  as the default in `src/core/CLAUDE.md`.
- **Phase 1 — P1**: precomputed input bindings; re-run benchmark (expect 3–10×).
- **Phase 2 — P2**: local-parameter binding + series-loop de-dup; validate fit
  results against a saved reference project (numerics will change for locals).
- **Phase 3 — P3**: explicit `Engine`/`Equation` JSON fields with read-compat.
- **Phase 4 — P5**: delete Chai/Duktape/Python/QJSEngine model paths + unused
  thread shell; drop build options; shrink deps.
- **Phase 5 — P4**: threaded/batched evaluation behind a datapoint threshold.
- **Phase 6 — P6 (optional)**: revisit JIT/codegen only if still bottlenecked.

## Risks

- **Semantic change from P2**: enabling local parameters alters results of any
  existing scripted fit that declared locals (they were previously ignored).
  Requires an explicit reference re-validation and a changelog note.
- **Backward compatibility**: the `ChaiScript` JSON key must stay readable; the
  new `Engine` field must default to ExprTk so old projects load unchanged.
- **Threading (P4)**: symbol tables/expressions are not thread-safe to share;
  each thread needs its own compiled copy — verify and guard small datasets.
- **ExprTk supply chain**: `external/exprtk.hpp` is a vendored ~1.6 MB header
  downloaded at configure time with the hash check commented out
  (`CMakeLists.txt`, see `TECHNICAL_DEBT.md`). Pin it (submodule/verified hash)
  before making it the sole engine.
- **Secondary (out of scope, noted)**: `AbstractModel::SetValue` reallocates
  `m_used_series`/`m_mean_series` every call and overwrites rather than
  accumulates — affects all models, worth a separate look.

---
*Claude Generated — analysis of `src/core/models/scriptmodel.*` and interpreter
backends; planning document only, no production code changed.*
