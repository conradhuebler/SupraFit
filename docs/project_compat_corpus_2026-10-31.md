# Project files of older versions: what opens, what is reproduced (2026-10-31)

Claude Generated. Branch `feature/project-load-diagnostics`. Tool: `test_project_compat` (corpus mode); the corpus is the operator's own project files and stays out of the repository.

## Method

829 project files (`.suprafit`, `.jdat`) on the operator's disks were listed by layout and version; 90 of them, one per combination of data version, model version and model number
(`SupraFit` 1599 ... 2004, single and multi-project), were loaded through `ProjectManager::loadProject`. `SUPRAFIT_PROJECT_CORPUS=<dir> test_project_compat corpus` loads every file below a directory;
`SUPRAFIT_PROJECT_FILE=<file> test_project_compat oneFile` loads one and prints one result line (a driver that runs file by file survives a crash);
`SUPRAFIT_PROJECT_ROWS=1` adds one line per model: stored sum of squares, sum of squares after loading, spread of the data.

## What did not open before (found with the corpus, fixed)

| Layout | Files | Cause | Now |
|---|---|---|---|
| several projects (`project_0`, `project_1`, ...) | 138 of 829 | `validateProjectJson` knew only `format_version` 2.0: "not a SupraFit project" | read project by project, issues carry the key (`project_3/model_1`) |
| data of SupraFit 1.x/2.x (`concentrations`, `signals`) | oldest ones | `DataClass(json)` reads only the current tables: empty data, every model "fails" | `LegacyImportData` for files without `independent` |
| model by name (`"model": "1:1-Model"`), model blocks without a version | 1599/1600 | `toInt()` of a string, current `ImportModel` | `Name2Model`, `LegacyImportModel` for blocks older than 2002 (its name check no longer stops block-form files) |
| `ModelDefinition` with plain values (`"GlobalParameterSize": 2`) | script models 2002/2003 | `toObject()` of a number is empty: the definition was lost, `GlobalTable()` null, **crash** in `ImportModel` | plain values are wrapped as `{"value": ...}`; a model without tables is refused, not dereferenced |
| `nmr_any` etc. defined by the grid `MaxA`/`MaxB` | 2003 | the grid was removed in July 2026 ("no stored project used it") | the grid is written out as reactions in the old order (`LegacyReactionText`), said in the report |
| file name decides the format (`path.contains("json")`) | any path with "json" in a directory name | `JsonHandler::LoadFile` | format from the content |

## What is reproduced

Loaded fit against stored fit (sum of squares, tolerance 1e-3 of max(stored, 1e-6 x spread of the data)), 432 models of the 90 files, after the fixes above:

| File version | same | lower | worse |
|---|---|---|---|
| before 1600 | 1 | 22 | 0 |
| 1600-1609 | 75 | 15 | 31 |
| 1700 | 33 | 15 | 31 |
| 2001 | 0 | 0 | 4 |
| 2002 | 8 | 0 | 3 |
| 2003 | 39 | 5 | 32 |
| 2004 | 83 | 8 | 27 |
| all | 239 | 65 | 128 |

"Lower": the model finds a better sum of squares from the stored parameters (linear parameters are projected out); noted in the report. "Worse" is a warning in the report ("Read, but the fit of the file is not reproduced").
Current-format files (2003/2004) that are **worse** point to defects, not to age; the families seen: fluorescence 20-23 and UV-Vis 30-32 (the stored local table has 5 columns, the model 4), kinetic models 107/108 of 2003 (up to 1e8 times the stored SSE),
`TianModel` 109, ITC 10-13, script models with ChaiScript equations (`var x = ...`, `def Calculate(int i, int j)`; ExprTk cannot compile them: "The model equation could not be compiled").
Open items are in `TODO.md` ("Older project files").
