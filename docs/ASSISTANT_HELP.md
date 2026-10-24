<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated 2026 - written from a read-through of the code (src/ui, src/core, src/agent, src/client);
UI labels are quoted from the tr() strings. Meant to be loaded as extra context into the assistant chat,
so it is kept short and states only what the code confirms.
-->

# SupraFit - how do I ...? (help for the assistant)

SupraFit fits binding models to titration data (NMR, UV/Vis, fluorescence, ITC), Michaelis-Menten and other
kinetics, and gives statistics on the fits. Use this text to tell the user which button or menu does what.
Name labels exactly as written here. If something is not covered, say so instead of guessing.

## 1. The window

- One toolbar. Split buttons (main part = default action, arrow = more): **New Table** (Ctrl+N; arrow: New Window),
  **Open Project** (Ctrl+O; arrow: Open Thermogram, Open Spectra), **Save Project** (Ctrl+S; arrow: Save Project As, Ctrl+Shift+S).
- Three things act on the project on screen: **Edit Data** (button left of the tabs, its arrow offers the Data menu), the menu **Model** in the toolbar, **Evaluation** (button right of the tabs). **Log** (F4) and **Agent** (F5, this chat) toggle docks.
  **SupraFit** menu: Settings, Info, License Info, About Qt, Quit.
- Left: the project tree (right-click: Duplicate, Add Up, Substract, Export, Delete; buttons Export Data, Export Projects, Close All) and the Log.
  Dropping files on the tree opens them. Dragging a model onto another project copies it.
- Centre: per project a **Workspace** dock (tab **Overview** = the data, one closable tab per model) and a **Charts** dock.
  The checkbox in a model tab ("Toggle Series in Charts") also decides which models **Optimise All**, **Analyse and Compare**
  and the project-level **Statistics** use.
- Dialogs that stay windows (Scan, a result opened in a window, the project-level Statistics) open centred on the main window the first time and
  afterwards where you left them, at the size you gave them.
- Charts dock, "Tools and Chart Settings" button: Animation, Chart Theme, Export Diagram (PNG), Plot Settings, Rescale Axis, Lock Scaling.

## 2. Getting data in

Table layouts (first row may be a header, `#` starts a comment, decimal comma is accepted):

| experiment | independent columns | then |
|---|---|---|
| NMR, UV/Vis, fluorescence titration | 2: host (fixed total conc.), guest (titrated total conc.) | one column per signal / wavelength |
| ITC | 1: injected volume | integrated heat per injection |
| Michaelis-Menten | 1: substrate S_0 | rate v |
| reaction models (`*_any`) | one concentration column per component | signals |

- **New Table** opens the dialog "Import Table": paste from a spreadsheet (Ctrl+V replaces, Ctrl+B appends columns, Ctrl+Y prepends columns),
  or **Select file**. Set "# Independent Variable(s):" to the number of leading concentration/x columns. Right-click: Clear Table.
- **Open Project** takes `.suprafit`, `.json`, `.dat`, `.txt`, `.csv`, `.itc`, `.dH` (several files at once). `.dat/.txt/.csv` go through the
  Import Table dialog (separator is detected: `;`, tab/whitespace, or `,` for csv). `.json`, `.suprafit`, `.itc`, `.dH` load directly;
  an `.itc` is integrated with default settings.
- **Thermogram (raw ITC heat curve)**: in the Import Table dialog use the button "Import as Thermogram" (plain x-y file: column 1 time, column 2 heat).
  The "Thermogram Dialog" has tabs Data Table, Experiment, Dilution. Set Cell Volume, Cell Concentration, Syringe Concentration, Temperatur,
  Inject Volume; per thermogram "Start time [s]"/"End time [s]" (click "Click to Select", then double-click in the chart), "Inject Time [s]" with
  **Apply and Update** (finds peaks, fits baseline, integrates), "Peak Ranges" (Custom/Zero/Threshold), Calibration and "Scaling (raw heat unit)".
  Per-injection volumes are editable in the table. The result is a two-column table (volume, net heat).
  Reopen it later with the Edit Data button left of the tabs. The setting "Automatic open Thermogram Dialog" does it automatically.
- **Spectra**: Open Spectra / "Import as Spectras" opens "Spectra Analysis and Import" (Load file, Open Directory).
- **Simulated data**: in New Table tick "Simulate Data with # Data Points" (columns by equation, default `X`; Store/Load Recipe), or in a fitted model's
  tab "Simulated Data" drag an entry (Ideal model, MC with model σfit, MC with model SEy, Bootstrap, MC with user σ) onto the project list.
- **Excluding data**: in the data tab right-click a row of the signal table to toggle it; right-click in the independent table: Begin Data / End Data.
  Per series: checkbox "Include" in the model's "Data series" box (or page). Data tab buttons: Regression (linear regression), Hide Datapoints, Plot X Values, Show Raw Data.
- **Edit Data** button (left of the tabs; its arrow opens the Data menu): **Edit Data** (make tables editable; thermogram/spectra projects reopen their dialog), **Split Data Set** (each signal column becomes its own project).

## 3. Adding a model (the models in the middle of the toolbar and the menu **Model**)

The toolbar has three zones: files on the left (Edit Data and Evaluation sit next to the tabs of the project), the menu Model together with the models you add most in the middle, Log, Agent and the SupraFit menu on the right. The models in the middle only show for a project on screen.
Each model is a button for a stoichiometry; a fresh installation shows 1:1, 2:1 / 1:1, 1:1 / 1:2 and 2:1 / 1:1 / 1:2. A click adds the model for the technique that stoichiometry was used with most: NMR while it never was used, then the technique you used last or the one you chose in **Customise...**. Every button names its technique ("1:1 · NMR"). The arrow next to a button lists the other techniques that fit the data: NMR, UV/Vis and Fluorescence for two or more independent columns, Calorimetry for one.
A stoichiometry stands for the fixed model of the technique (the four established ones) or for its flexible `*_any` model with the reactions preset. Scripted presets and model files can be buttons too.
The gear next to the buttons switches between **Show the most used models** and **Show a fixed list**, and has **Customise...**, **Fix the bar as it is** (turns what the bar shows now into the fixed list) and **Forget what was used** (resets the counts).
**Customise...** opens "Customise the quick bar": "The models I add most" with how many, or "A list I put together" (tick what the bar shows, order with Up and Down; the list holds all stoichiometries, models, scripted models and the model files you used), and the technique a model starts with that was never added.
Right-click on a button: **Remove from the bar** (fixed list), **Fix the bar as it is** (most used), **Customise...**.
Models added from the menu **Model** count for the bar as well. Right-click an entry there and choose **Add to the quick bar**; the bar then shows a fixed list.

## 4. Fitting (model tab)

- Below the fit controls a list on the left selects the page: Data series, Model Options, System Parameter, Model Definition (script models), Parameter Table (`*_any`), Statistics, Charts (the model's own charts, listed once it has one), Detailed, Simulated Data. Entries with a ">" (Statistics, Charts) replace the list by their own list with "← Back"; Charts has one entry per chart (right-click: Close chart).
  **Pin** in "Data series" keeps the series beside the pages; by default they are a page of the list. The last page and the pin are remembered per model type.
- Checkboxes **Global Parameter** / **Local Parameter** choose what is optimised; the box next to a single value fixes that parameter.
  Status label: Converged / Not converged / Model corrupt. "Read Only" locks the local parameters.
- **Fit** button runs the default "Tight". Its dropdown: Tight, Progress (shows optimisation history), Simplified MOC (fast F-test interval),
  **Global Search...** (the Scan: grid over start values, "No of calculations to be done"; opens the Scan window), **Local Fits** (one model per series),
  LevMar (classic) / VarPro (projection; only nmr_any/uvvis_any), submenu "Speciation solver" (LevMar / Newton default, BFGS legacy; `*_any` models).
  Picking a solver refits at once.
- The "..." button next to a global parameter ("Boundaries of <param> and root search"): Enable Lower/Upper Boundary (soft log-Fermi penalty, experimental;
  keep the start value inside), "Single Optimisation".
- Action buttons: **New Guess**, Restore, Save, Load Constants, Save Constants, **Fit Settings** (iterations, tolerances, Fit Solver).
  Statistics, results, charts and the detailed data (input data, model, parameters and statistics as text) are not buttons: they are the pages "Statistics", "Charts" and "Detailed" of the page list. "Detailed" shows the statistical analyses of the stored results only with its box "Include the statistical analyses" (off by default: they can take a while).
- No convergence: try New Guess, other start values, the Global Search, fix a weakly determined parameter, or exclude an outlier series/row.
- **Evaluation** button (right of the tabs): **Optimise All** (checked models), **Statistics**, **Analyse and Compare**, Remove All Models.

## 5. Statistics

The page "Statistics" of a model (one model; the page list is replaced by the analyses, "← Back" returns) or **Evaluation** > **Statistics** (all checked models, window) opens "Statistic Tools":
- **Cross Validation/Reduction**: Leave-One-Out / Leave-Two-Out / Leave-X-Out; buttons Cross Validation, Reduction Analysis.
- **Monte Carlo**: "Number of MC Steps:" (2000), σ taken as "σ from fit", "SEy" (default), user input, or bootstrapping; "Use Original Data"; **Simulate**.
  Percentile confidence interval. Assumes the model is right and noise random; a misspecified or sparse data set gives wide or skewed intervals.
- **Weakened Grid Search**: F-test confidence interval for one chosen parameter. **Model Comparison**: F-test with random steps (Max. Steps 20000).
- **Simplified MOC** (Fit dropdown) and the setting "Automatic Confidence Calculation using Simplified Model Comparison." (default on): quick F-test intervals.
- Results: a finished run shows the page "Statistics": its list has the analyses under "New analysis" (start one) and every finished run under "Results" ("Monte Carlo #1", "Cross Validation #1", "Grid Search #1", ...; each with the icon of its method, the tooltip has the full name; pick one to look at it, right-click: Open in window, Save data, Drop raw data, Remove). **Open in window** (also below the result) moves a result into a window of its own; closing it puts it back. Monte Carlo has Histogram / Scatter Plot / Series Chart,
  "Confidence Intervall", bins, Export Results. Global search: Result List / Scatter Plot, filters Only Valid / Only Converged Models, Export Models.
- Model quality box ("Fit results"): Parameters fitted, Points used, Degrees of freedom, SAE, R2, F value, Mean error, Standard error, χ2.
- **Analyse and Compare** (Evaluation menu, checked tabs): Compare Reduction Analysis (Cutoff), **Compare AIC**, Compare Cross Validation / Monte Carlo (LOO/L2O/LXO).
- BC50 (concentration at 50 % binding) and Shannon entropy come with the search / Monte Carlo output (setting "# integration points for BC50").

## 6. Saving, exporting, settings

- **Save Project** writes all open projects into the file in "Filename:" (`.suprafit` = compressed json, `.json` = plain; several projects give one multi-project file).
  Models and statistics are included. Agent chats are stored in the project if "Store agent chats in project files" is on - save to keep them.
- Export: project tree Export; Export Data (input tables as text); Export Projects (one `.suprafit` per project); model Save / Save Constants / Export Simulated;
  Monte Carlo Export Results; search Export Models; charts Export Diagram (PNG).
- **Settings** tabs: General Settings (Threads, "Report thermodynamics in kcal/mol (instead of kJ/mol)", "Automatic open Thermogram Dialog", "Show advanced options and simulation tools in GUI." shows
  further tools at once: more simulation chips, "Any Model…", "Drop Random", the LXO strategy and the thermogram "Advanced stuff"), Appearance ("Colour scheme:" System / Light / Dark, "Toolbar buttons:"), File Save Settings (agent chats, raw data), Standard Calculation
  (confidence 95 %, Maximal Steps, bins, BC50 points), Chart Settings, Optimizer Settings (defaults of Fit Settings).

## 7. Reading results

- Compare models by **AIC** (lower is better; AICc for few points). Evidence ratio = 1/exp(-0.5·ΔAIC). Only comparable for the same data and the same included rows.
  Report best model, ΔAIC and whether the fit converged. A model with SSE = 0 is not fitted yet.
- Residual = measured - modelled. A systematic trend, a non-zero mean or lag-1 autocorrelation near ±1 means the model misses an effect (try another stoichiometry).
- `lg K` is log10; K = 10^lgK in 1/(concentration unit). Stepwise (K21, K12) and cumulative (lg β in `*_any`) constants are not interchangeable.
- Wide intervals / many different minima in the Scan: parameter poorly determined by the data (more points in the transition region, simpler model, fix a parameter).

## 8. Your tools in this chat

Read (run freely): `get_help` (this document: no topic lists its sections, a number or title word returns one), `list_projects` (start here), `get_project_info`, `list_models`, `get_model_fit`, `get_model_parameters`, `get_model_statistics`,
`get_data_table`, `get_residuals`, `compare_models_aic`, `calculate_bc50`, `describe_model_catalog` (incomplete list).
Change/compute (need approval at autonomy "Ask for fits"): `add_model`, `add_reaction_model` (model_id 34 NMR, 35 UV/Vis, 36 fluorescence, 17 ITC), `define_script_model`,
`set_model_parameter`, `set_series_included`, `set_rows_included` (at least 4 rows must stay), `fit_model`, `fit_models`,
`run_postfit_analysis` (method 1 Monte Carlo, 2 weakened grid search, 3 model comparison, 4 cross validation, 5 reduction, 6 simplified model comparison, 7 global search;
needs a fitted model).
Typical order: inspect (`list_projects` -> `list_models` / `get_data_table`) -> add -> `fit_models` -> `compare_models_aic` -> `get_residuals` -> `run_postfit_analysis`.
Never invent project ids, model indices or numbers. The chat menu (☰): Endpoints..., New chat, History, Export as Markdown/JSON, Import JSON, Copy last answer, "Program help in the prompt" (off: the assistant fetches sections with `get_help`; on: this whole document is added to every request, about 4000 tokens - use it with models that do not look things up).

## 9. Command line and Python

- `suprafit_cli -i project.json -o project.suprafit` converts; `suprafit_cli -l file` lists structure and fit statistics; `-x --extract-model N file` prints fitted parameters;
  `-i task.json -o out` runs a task file (`Main`, `AddModels` with `ID`, `PostFitAnalysis`, optional data `Generator` with `Noise`); `-n N` threads.
  `-j` / `--join` merges several project files; `-p idx` extracts one project; `-s` splits.
- Thermogram: `suprafit_cli -i thermogram.itc -o thermogram.suprafit`.
- `suprafit_cli -i project.json --agent "question" -o out.json` runs this assistant headless (tools needing approval are refused); `--list-profiles`, `--profile`, `--model`, `--agent-reasoning`, `--agent-help` (whole program help in the prompt instead of `get_help`).
- Python (`import suprafit`, `pip install -e python/`): `Project.from_file/from_arrays`, `add_model("nmr_1_1")`, `fit()`, `monte_carlo()`, `cross_validation()`, `m.sse`, `m.aic`.
