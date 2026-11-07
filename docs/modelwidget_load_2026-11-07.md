# Model pane construction while a project opens (2026-11-07)

Measured with `src/tests/benchmark_projectload` (release, offscreen, wall time from `LoadFile` until the windows are built
and rendered, min of 2 runs per file; sections summed with `LoadTiming`, min of 2 runs). Corpus: 90 project files of the
compat corpus, plus the user file `alle_full.suprafit` (15 windows, 24 models) in a run of its own.

## Totals

| file | windows/models | before [ms] | after [ms] |
|---|---|---|---|
| 012_Druckmessung.suprafit | 11/20 | 2973 | 2933 |
| 026_stoichio_pub.suprafit | 4/16 | 2167 | 1790 |
| 018_all.suprafit | 24/32 | 1617 | 1499 |
| 031_all_bisher.suprafit | 3/12 | 1504 | 1224 |
| 040_Simulation_test.suprafit | 5/16 | 1396 | 1132 |
| 027_ITC_test.suprafit | 5/20 | 1385 | 1096 |
| sum over the corpus (90 files) | | 40124 | 35567 (-11 %) |
| alle_full.suprafit | 15/24 | 1080 | 1032 (-4 %) |

## Where the time of `ModelDataHolder::ActiveModel` goes (sum over corpus + user file, 498 model panes)

| section | before [ms] | after [ms] | user file before / after |
|---|---|---|---|
| `mdh.ActiveModel` | 18278 | 16700 | 528 / 508 |
| `modelwidget.ctor` | 11160 | 10077 | 268 / 258 |
| `mdh.addModelsTab` (re-polish under the style sheet of the tabs) | 6270 | 5956 | 238 / 230 |
| `mw.StatisticDialog` (6 analysis tabs, F quantiles) | 4552 | 3382 | 120 / 70 |
| `mw.StatisticsPage` (scroll area polishes the dialog) | 624 | 1711 | 32 / 74 |
| `mw.StatisticWidget` | 1993 | 1211 | 17 / 18 |
| `mw.series scroll` (data series) | 805 | 781 | 8 / 8 |
| `mw.results box` | 480 | 466 | 20 / 20 |
| `mw.ModelChartWidgets` | 464 | 441 | 0 / 0 |
| `mdh.chart.addModel` | 526 | 487 | 12 / 11 |

The `ModelInfo()` text and the second fit-results update ran partly outside these scopes (timer after the constructor),
which is why the totals fall more than the sections.

## Changes (savings over corpus + user file, estimated from the sections)

- `StatisticWidget::Update` writes `ModelInfo()` (BC50 and the like, ~1.3 ms per call) once instead of twice: ~1.5 s.
- The recalculation the `ModelWidget` constructor announces no longer schedules a second `StatisticWidget::Update`
  (written by its constructor just before; a recalculation during construction still does): ~1 s.
- `StatisticDialog` sets the minimum width from the size hint of all tabs on its first show as a window, not in the
  constructor: an embedded dialog had it reset to 0 right after. The polish moves to the scroll area of the page (net ~0.3 s).
- `setStyleSheet` of the parameter boxes, the convergence label, the series cards and the colour buttons only when the
  sheet changes (`SetStyleSheetIfChanged`): every call re-polishes, also with the same sheet (after each recalculation).

## What did not go in, and why

- **Style sheet of the model tabs on the tab bar instead of the tab widget** (saves `addModelsTab`, ~6 s): pixels change.
  The pages below are painted by the style sheet style (spin boxes, radio rows and the like get other metrics); without
  the inherited sheet the pane shifts by a few pixels.
- **Building the pane in place** (created inside the tab stack, frame first, pages attached before their scroll areas
  polish them, so each widget is polished once): the first splitter sizes and the outer scroll range differ (sizes are
  taken while parts are still empty), and every reparenting of a polished part under the styled stack polishes it again
  (+28 % when only half done).
- **Lazy pages** (Statistics, Charts, Detailed): the stack of the page list takes its size hint from all pages, and the
  layout tests look into the statistic dialog right after the pane is shown.
- F quantiles (`Fisher_Dist::finv`, ~2.7 s, once per parameter/degree-of-freedom pair): the fit results need the same
  value; making the integration faster would be in `external/fisher_dist`.

## How identical output was shown

`SUPRAFIT_LOAD_GRAB=<dir> SUPRAFIT_LOAD_GRAB_PAGES=1 benchmark_projectload <files>` saves the tab widget of every
project window with every tab and every page of every model pane (824 PNG for 8 files: the 2 reference fixtures
`simulated_2_1_1_1.json`, `simulated_1_1.json`, five corpus files, `alle_full.suprafit`); light and dark
(`SUPRAFIT_LOAD_APPEARANCE=2`). Before and after: 0 of 824 images differ (RGB compare), master against itself also 0.
The log (times, ids) and the charts (axes fitted on a timer) are left out of the images; the load progress delay is
set very high while grabbing, since a slower run gets a status bar of another height.
