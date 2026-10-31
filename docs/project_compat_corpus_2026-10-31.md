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

Loaded fit against stored fit (sum of squares, tolerance 1e-3 of max(stored, 1e-6 x spread of the data)), 432 models of the 90 files:

| File version | same | lower | worse |
|---|---|---|---|
| before 1600 | 1 | 22 | 0 |
| 1600-1609 | 95 | 20 | 6 |
| 1700 | 73 | 1 | 5 |
| 2001 | 4 | 0 | 0 |
| 2002 | 10 | 0 | 1 |
| 2003 | 67 | 5 | 4 |
| 2004 | 96 | 1 | 21 |
| all | 346 | 49 | 37 |

(First pass of the same corpus, before the fixes below: 239 / 65 / 128.) "Lower": the model finds a better sum of squares from the stored parameters (linear parameters are projected out); noted in the
report. "Worse" is a warning in the report ("Read, but the fit of the file is not reproduced").

## Why models did not reproduce, and what was done

| Family | Cause | Now |
|---|---|---|
| bimolecular kinetics 107 (2003) | files before the scale factor q have four parameters, the model has five, the fifth was 0 | `AdaptStoredModel`: q = 1 (the older formula equals the current one with q = 1); exact |
| flexible kinetics 108 (2003) | another parameterisation until September 2022 (`(cA0-cAeq)[(cA0-cAeq)^(n-1) k t (n-1) + 1]^(1/(n-1)) + cAeq`), no version bump | option "Formula" (`current`, `2022-08`), chosen at import by which gives the stored sum of squares; exact (0.0019221040623138 = stored) |
| ITC 10-13 | **the check marks of the data rows were lost on every load**: `DataClass::ImportData` sets the data range, which marks every row as used | `ImportData` keeps the stored marks of the cells (a first injection left out of the fit stays left out); all ITC models of 1605-2001 reproduce |
| ITC, older files | `Cell Volume constant` (system parameter 5) is `null` in files before 2001: the fit is the one with the volume not constant | `AbstractItcModel::AdaptStoredModel` tries both, keeps the one that matches |
| fits that left out the first points | the stored SSE is over the rows after the first one or two | after loading, up to four first rows are unchecked one by one; kept only if more stored fits come out to 1e-6 (noted in the report) |
| meta models in multi-project files | the Open path never rebuilt them (they became empty projects) | reported as left out per meta model; rebuilding them is GUI work (`SupraFitGui::LoadMetaModels`) |

## What is still not reproduced (causes known or probable)

- NMR 2:1/1:1 (12 models, 1-10 %, one file 2.6x): stored by the earlier speciation solver; the closed-form solver of today gives the exact value (`TODO.md`, "Regenerate the 2:1/1:1 reference data").
- Fluorescence 20-23, UV-Vis 32 (2003/2004): the models were reworked in September 2024 (new local columns, options "Host only", "Guest (dynamic)"; before, `value = (host_0*L0 && i == 0) + ...` used a logical AND); the old numbers are not reproducible by a mapping.
- Script models with ChaiScript equations (`var x = ...`, `def Calculate(int i, int j)`): ExprTk cannot compile them ("The model equation could not be compiled").
- `TianModel` 109 (2023-04 file, before the model of May 2023), ITC 11/12 of 1605/1609 (4 models), `itc_n_1_1` 15 (1).
- NMR 1:1/1:1.2 UV/VIS option (2002, `044_LegacyTest`): the method "UV/VIS" of the NMR models was dead code since 2022; the file's numbers belong to the UV-Vis model, whose parameter layout differs (not mapped).
