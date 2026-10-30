<!--
Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document
-->

# Work Package: Statistic dialogs and Scan in the style of the Landscape tab

**Status:** WIP (S1 ADDED) · **Owner:** Conrad Hübler · **Author note:** Claude Generated · branch `feature/statistic-dialogs`

## Goal

Bring the analysis dialogs of the model pane (Cross Validation/Reduction, Monte Carlo, Weakened Grid Search, Model Comparison) and the
Scan (Global Search) to the pattern the Landscape tab set: **few fields in sight, the rest behind `..more..`, a stage text and a bar that
says what happens, nothing in the log or notifications but one summary line**, no overlap, nothing stretched over the page.

## Decisions (operator, 2026-10-30)

- **Scan becomes a page** of the Statistics list (an analysis next to the others), no window of its own in the model pane.
- **`..more..` replaces `advanced_ui`** in these dialogs (the global setting stays for the other places: toolbar, simulation, thermogram).
- **Project-wide dialog: all parameters.** It has no model to pick parameters from; Grid Search and Model Comparison take every parameter
  of every model (global and local; a choice restricts them to the global ones).

## Findings (survey 2026-10-30, renders with a two-constant `nmr_any` model)

| Dialog | Finding |
|---|---|
| Cross Validation / Reduction | rows stretched over the page height; strategy box behind `advanced_ui` |
| Monte Carlo | rows stretched; sigma field read-only unless "from user input"; one row per input variable |
| Weakened Grid Search | about twelve fields at once (parameters, interval, F value, step factor, max steps, convergence and three counters) |
| Model Comparison | **overlap**: the label "Choose parameter to be tested:" lies on the last parameter check box; unlabeled scaling spin beside each parameter; a group with one field |
| Scan | one group box per parameter (start/end/step, two check boxes), local parameters in a tab per series, own window with its own bar without text; defaults value/5 .. value*5 are meaningless for log constants |
| Project dialog | Grid Search and Model Comparison show an empty parameter area (no model) and still offer Calculate |
| All | `std::cout` in Monte Carlo, Cross Validation and the Scan; Grid Search counts its bar per parameter side only; the stage texts of Cross Validation go through `Message`; the parameter list, confidence and F value are built twice (Grid Search and Model Comparison), four radio buttons (SSE, SEy, chi^2, sigma) are built twice and always hidden (`extended_f_test`) |

## Work packages (one buildable commit each)

| WP | Content | Test |
|---|---|---|
| S1 | `ParameterSelector` and one confidence line shared by Grid Search and Model Comparison; both tabs rebuilt (in sight: parameters, confidence, start; behind `..more..`: step and stopping settings, store, box scaling); the overlap fixed; the dead radio buttons removed; project dialog without a model: all parameters, resolved per model when the job runs (`AllParameters`) | render of both tabs without overlap; job lists equal the old ones for a model; project job reaches every model |
| S2 | Monte Carlo and Cross Validation tabs rebuilt (no stretching, strategy and input scatter behind `..more..`, `advanced_ui` no longer read there) | render, controller keys unchanged |
| S3 | `ScanWidget` (table: vary / name / value / start / end / step / optimise, locals behind `..more..`, count of calculations, defaults +-3 for `lg` constants) as the analysis "Scan" of the Statistics list; `AdvancedSearch` only wraps it for the meta model; "Global Search..." opens that page | job equals the old one, page selected by the menu entry, window test replaced |
| S4 | `Stage` and bar units for every analysis (Monte Carlo, Cross Validation, Grid Search with step counters, Model Comparison, Scan), `std::cout` removed | stages in order, bar ends at its maximum, log gets one line |
| S5 | `docs/ASSISTANT_HELP.md`, changelog, CLAUDE.md, vault | `AgentHelpTest` |

## Constraints

- Controller keys of the jobs stay as they are (agent tools and stored results read them, `src/agent/postfitjobs.*`).
- Labels quoted in the help change with the dialogs: help and tests are edited in the same commit.
- A widget gets its parent before `setVisible(true)` (window flicker, test `noStrayTopLevelWindows`).
