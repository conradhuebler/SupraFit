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
  **Open Project** (Ctrl+O; arrow: Open Thermogram, Open Spectra, Open NMR Spectra), **Save Project** (Ctrl+S; arrow: Save Project As, Ctrl+Shift+S).
- Three things act on the project on screen: **Edit Data** (button left of the tabs, arrow = Data menu), the menu **Model** in the toolbar, **Evaluation** (button right of the tabs). **Log** (F4) and **Agent** (F5, this chat) toggle docks.
  **SupraFit** menu: Settings, Info, License Info, About Qt, Quit.
- Left: the project tree (right-click: Duplicate, Add Up, Substract, Export, Delete; buttons Export Data, Export Projects, Close All) and the Log.
  Dropping files on the tree opens them. Dragging a model onto another project copies it.
- Centre: per project a **Workspace** dock (tab **Overview** = the data, one closable tab per model) and a **Charts** dock.
  The checkbox in a model tab shows or hides the curves (model and error chart) of this model only; unchecked it also takes the model out of
  **Optimise All**, **Analyse and Compare** and the project-level **Statistics**. It is partly checked when only some curves of the model are
  hidden (with "Show in Plot" of a series), which leaves the model in use. The data series and the other models are not touched.
- Dialogs that stay windows (a result opened in a window, the project-level Statistics) open centred on the main window the first time and
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
  While a file opens, the cursor is the wait cursor; a load that takes longer than about 0.3 s shows "Opening <file>: project n of m" and a bar "n of m models" in the status bar (gone when it is done).
  The format is taken from the content, not the name; files of older SupraFit versions (also several projects in one file) open too. If a file cannot be opened, or models of it are left out, a dialog says why ("Loading Error" / "Project loaded with problems", the details list every model: unknown number, does not fit the data, newer file, "fit of the file is not reproduced" = the model gives another sum of squares from the stored parameters than the file says: fit it again; "leave out the first n data points" = older fits did not use them, they are unchecked in the data table; a meta model of a multi-project file is rebuilt from the projects of the file and has the same page list as a model (Parameters, Statistics with the analyses and results, Summary, Detailed); one whose projects are missing is left out and said). The flexible kinetic model has the option "Formula" (`2022-08` = the formula of the first kinetic projects).
- **Thermogram (raw ITC heat curve)**: in the Import Table dialog use the button "Import as Thermogram" (plain x-y file: column 1 time, column 2 heat); the file of "Select file" opens as its experiment.
  The "Thermogram Dialog" has tabs Data Table, Experiment, Dilution. Set Cell Volume, Cell Concentration, Syringe Concentration, Temperature,
  Inject Volume; per thermogram "Start time [s]"/"End time [s]" (click "Click to Select", then double-click in the chart), "Inject Time [s]" with
  **Apply and Update** (finds peaks, fits baseline, integrates), "Peak Ranges" (Custom/Zero/Threshold), Calibration and "Scaling (raw heat unit)".
  Per-injection volumes are editable; the result is a table (volume, net heat).
  Setting "Automatic open Thermogram Dialog" opens it by itself.
- **Spectra**: Open Spectra / "Import as Spectra" opens "Spectra Analysis and Import"; "Open directory" (names containing the "File ending") and "Load file" (row 1 x, one spectrum per row) replace, "Add files" appends. "Spectra View": "Start"/"End" (or right-drag) set the x range of "VarCovar"; a double-click or a typed value (Enter, "Add value"; no number = refused) adds an x value; "Zoom to range". OK is off without spectra or x values.
  "Processing steps" (Add: Range, Smoothing (Savitzky-Golay), Baseline, Normalisation, Shift of the x axis, Blank region, Peak picking; Remove, Up, Down; untick = off) apply top to bottom to every spectrum before the table; the raw spectra stay unchanged, the steps are stored with the project. "Blank region" ("Ranges": two numbers per range, ';' between; "Blank the selected range") replaces a signal (standard, solvent) by a straight line between its neighbours; a Normalisation on that signal goes before it. "Show processed" draws the processed spectra (peaks as markers); an invalid parameter is refused with a message below "Parameters".
- **NMR titration** (series of 1D spectra: ASCII/CSV ppm + intensity, a matrix ppm + one column per spectrum, TopSpin totxt, JEOL Delta FIDs `.jdf`): Open NMR Spectra (toolbar) or "Import as NMR Spectra" (Import Table dialog) opens "NMR Titration Import" ("Open directory" by "File ending", natural order, replaces; "Add files" appends).
  FIDs: panel "Processing (FID)" under the spectra: "LB" (Hz), "Zero fill", "ph0"/"ph1" for all spectra, "Auto phase" (phase of the reference spectrum for all), "Auto phase each spectrum" (default). Their ppm axis is the spectrometer's: use "Reference offset" (e.g. CHCl3 to 7.26).
  "Spectra": order (Up/Down), untick = left out, "Reference" = spectrum the signals are defined in. "Signals": "Reference offset" (ppm added to every spectrum), "Search window ±", "Pick from reference spectrum", or a position + "Add"; "Double-click adds the signal" (default on; off: it only sets the position); untick = left out. Pick, "Remove" and "Remove signal" ask before hand-set values or more than one signal are lost.
  "Chemical shift axis: high to low (left to right)" above the chart (default on, remembered): high ppm on the left whatever the order in the file; off: low ppm on the left.
  Table below the chart: δ per spectrum and signal; red = missing (no peak in the window; red in every spectrum: a much stronger line raises the threshold, the status line suggests a Range or Blank region step), orange = conflict (two signals, one peak), blue = set by hand (type a value; clear = tracked again). OK needs no red/orange cell and the concentrations on "Concentrations and table".
  "Shift from": "Peak picking" (maximum) or "Lineshape fit" (Gauss-Lorentz line fitted to the whole signal; for overlapping lines, can separate a conflict). Options behind "Lineshape fit options" "..more.." (opens for a lineshape fit): "Line width" ("Free per spectrum" / "Shared over the series"), "Baseline", "Fit range ±", "Max. iterations", "Peak position where the fit fails"; violet = fit failed. "Fit details" adds ±δ, FWHM, Area, η; the chart shows model (dashed) and residual of the selected spectrum.
- **Both spectra dialogs**: dropped files or directories (by the ending) are appended; "n file(s) read, m not read" (tooltip: why). "Concentrations": Ctrl+V (Ctrl+B appends, Ctrl+Y prepends columns) or "Load from file"; "n rows for m spectra", warning if they differ. Kept for the next dialog: size, place, dividers, ending, VarCovar options, NMR "Search window ±", "Shift from" with options, LB, zero fill; a reopened project keeps its own.
  Enter never presses OK; ESC first ends a chart drag; Cancel, ESC or closing ask "Discard the changes?" (Discard / Keep editing) only after a change. The bottom line says what OK hands over or why not. The line under the chart lists the mouse actions (setting "Show hints in the import dialogs", also the Thermogram Dialog's guide): left-drag zooms as set in "Tools" → "Zoom Strategy" ("Rectangular" default, "Horizontal", "Vertical"), wheel = intensity of the spectra (×1.25 per notch, view only, axis unchanged, label "Intensity ×…"), Ctrl+wheel = y zoom at the pointer, middle click = whole spectrum and intensity 1, right-drag = range, double-click. "Fit y to the visible range" sets the intensity to 1 and scales y to the shown curves. Zoom and intensity stay when steps, stack or axis direction change (other spectra: whole range, intensity 1).
  View only (remembered per dialog, not in the project): "Stack offset" (% of the tallest spectrum in Start..End) stacks the spectra, the first at the bottom ("Reverse order": on top); "x offset" (% of Start..End) shifts each right; y labels hidden while stacked; left-out NMR spectra not drawn.
- **Simulated data**: in New Table tick "Simulate Data with # Data Points" (columns by equation, default `X`; Store/Load Recipe), or in a fitted model's
  tab "Simulated Data" drag an entry (Ideal model, MC with model σfit, MC with model SEy, Bootstrap, MC with user σ) onto the project list.
- **Excluding data**: in the data tab right-click a row of the signal table to toggle it; right-click in the independent table: Begin Data / End Data.
  Per series ("Data series" of a model): "Include" = the series is fitted (and drawn) by this model, "Show in Plot" shows/hides its curve, "Single Plot" shows only this series in the data and every model chart (off: back as before). Data tab buttons: Regression (linear), Hide Datapoints, Plot X Values, Show Raw Data.
- **Edit Data** button (left of the tabs; arrow = Data menu): **Edit Data** (tables: editable; thermogram, spectra, NMR: their dialog), **Undo Edit Data** (one step), **Split Data Set** (a project per signal column).
  OK replaces the data (title, system parameters, notes stay). Same independent columns, series, type: models stay, recalculated (no refit), marked outdated; left-out points stay out. Else a question with reasons: "Create a new project" (default; spectra via Import Table), "Remove the models and replace the data", Cancel. During analyses: "Stop the running analyses and replace the data?"; closing: "Save the changed data?".

## 3. Adding a model (the models in the middle of the toolbar and the menu **Model**)

The toolbar has three zones: files on the left (Edit Data and Evaluation sit by the tabs of the project), the menu Model with the models you add most in the middle, Log, Agent and the SupraFit menu on the right. The models in the middle only show for a project on screen.
Each model is a button for a stoichiometry; a fresh installation shows 1:1, 2:1 / 1:1, 1:1 / 1:2 and 2:1 / 1:1 / 1:2. A click adds the model for the technique that stoichiometry was used with most: NMR while it never was used, then the technique you used last or the one you chose in **Customise...**. Every button names its technique ("1:1 · NMR"). The arrow next to a button lists the other techniques that fit the data: NMR, UV/Vis and Fluorescence for two or more independent columns, Calorimetry for one.
A stoichiometry stands for the fixed model of the technique (the four established ones) or for its flexible `*_any` model with the reactions preset. Scripted presets and model files can be buttons too.
The gear next to the buttons switches between **Show the most used models** and **Show a fixed list**, and has **Customise...**, **Fix the bar as it is** (turns what the bar shows now into the fixed list) and **Forget what was used** (resets the counts).
**Customise...** opens "Customise the quick bar": "The models I add most" with how many, or "A list I put together" (tick what the bar shows, order with Up and Down; the list holds all stoichiometries, models, scripted models and the model files you used), and the technique a model starts with that was never added.
Right-click on a button: **Remove from the bar** (fixed list), **Fix the bar as it is** (most used), **Customise...**.
Models added from the menu **Model** count for the bar as well. Right-click an entry there and choose **Add to the quick bar**; the bar then shows a fixed list.

## 4. Fitting (model tab)

- Below the fit controls a list on the left selects the page: Data series, Model Options, System Parameter, Model Definition (script models), Parameter Table (`*_any`), Statistics, Charts (the model's own charts, listed once it has one), Detailed, Simulated Data. Entries with a ">" (Statistics, Charts) replace the list by their own list with "← Back"; Charts has one entry per chart (right-click: Close chart).
  **Pin** in "Data series" keeps the series beside the pages (default: a page of the list); last page and pin are remembered per model type.
- Fluorescence models (the four stoichiometries and the flexible one are the same model with different reactions): signal = sum of concentration x fluorescence coefficient (a local parameter per component and species and series); stability constants are cumulative lg beta; the last global parameter "lg K(SV)" is the Stern-Volmer constant. Model Options: "Inner filter correction (AI-implemented)" (off/on; needs System Parameters "Path length" and the absorptivities of the components at excitation and emission) and "Dynamic quenching by free (AI-implemented)" (none or a component; fits lg K(SV)). Both were written by an AI and are not yet checked against the literature (docs/fluorescence_models.md).
- Checkboxes **Global Parameter** / **Local Parameter** choose what is optimised; the box next to a single value fixes that parameter.
  Status label: Converged / Not converged / Model corrupt / "Data changed – refit" (Edit Data; banner "This model was fitted to earlier data.", **Fit again**, **Mark as current**; results get a note, project list "(outdated)"; saved). "Read Only" locks the local parameters.
- **Fit** button runs the default "Tight". Its dropdown: Tight, Progress (shows optimisation history), Simplified MOC (fast F-test interval),
  **Global Search...** (the Scan: opens the analysis "Scan" on the Statistics page), **Local Fits** (one model per series),
  LevMar (classic) / VarPro (projection; only nmr_any/uvvis_any), submenu "Speciation solver" (LevMar / Newton default, BFGS legacy; `*_any` models).
  Picking a solver refits at once.
- The "..." button next to a global parameter ("Boundaries of <param> and root search"): Enable Lower/Upper Boundary (soft log-Fermi penalty, experimental;
  keep the start value inside), "Single Optimisation".
- Action buttons: **New Guess**, Restore, Save, Load Constants, Save Constants, **Fit Settings** (iterations, tolerances, Fit Solver).
  "Statistics", "Charts" and "Detailed" (data, model, parameters, statistics as text) are pages of the list, not buttons; "Detailed" adds the analyses of the stored results only with "Include the statistical analyses" (off: slow).
- No convergence: New Guess, other start values, the Global Search, fix a weakly determined parameter, or exclude an outlier series/row.
- **Evaluation** button (right of the tabs): **Optimise All** (checked models), **Statistics**, **Analyse and Compare**, Remove All Models.

## 5. Statistics

The page "Statistics" of a model (one model; the page list is replaced by the analyses, "← Back" returns) or **Evaluation** > **Statistics** (all checked models, window) opens "Statistic Tools":
- **Cross Validation/Reduction**: Leave-One-Out / Leave-Two-Out / Leave-X-Out; buttons Cross Validation, Reduction Analysis. The calculation strategy of Leave-X-Out is behind **..more..**.
- **Monte Carlo**: "Number of MC Steps:" (2000), one choice for the noise: "SEy" (default), "σ of fit", user input (σ editable), or bootstrapping; "Use Original Data"; scatter of the independent variables behind **..more..**; **Simulate**.
  Percentile confidence interval. Assumes the model is right and noise random; a misspecified or sparse data set gives wide or skewed intervals.
- **Scan** (menu **Global Search...**): a grid over start values, every point is fitted. Per parameter: tick = varied, "Start:", "End:", "Step:", "Fit" (optimised in the fit); a logarithmic constant is proposed as value ± 3. Local parameters of the series are behind "..more..". "No of calculations to be done" counts the grid; **Scan** starts it, the result is "Scan #1".
- **Weakened Grid Search** and **Model Comparison**: F-test confidence limits of single parameters. In sight: "Parameters to test:" (the constants ticked, the local parameters not), "Confidence:" (95 %) with a line that says the F value and how far the sum of squares may rise, for the Model Comparison "Steps:" (20000), **Calculate**. Behind "..more..": Grid Search the size of a step ("Scaling factor of a step:", "Maximal steps (each direction):"), the stopping rules ("Change of the SSE counted as zero:", "Stop after steps without change:", "Stop after steps above the limit:", "Stop after steps below the fit:") and "Store intermediate models" (needed for derived values such as the entropy, makes large files); Model Comparison "Factor of the box of each parameter:". The statistic dialog of the whole project (Evaluation > Statistics) has no model to pick parameters from: there "Parameters:" chooses "All parameters (global and local)" or "Global parameters only", and every model of the project is tested with its own limit.
- **Landscape Sampling** (one model only; "Sample" is disabled for models without variable projection, i.e. it needs a reaction model such as the NMR/UV-Vis/fluorescence "any" models): samples the sum of squares over the global parameters and reads the F-test confidence interval of every constant from it, also for correlated constants, and refits other minima it finds. In sight: "Task:" ("Confidence intervals" or "Find other minima"), "Confidence:" (95 %; confidence intervals only), "Steps per walker:" (50000), "Sampler:" (Metropolis, Langevin), "2D surfaces of the pairs" (on for two to four parameters), **Sample**. Behind "..more..": "Walkers:", "Step size (integration):" (the start value of the step as a fraction of the range, the walker adapts it), for Langevin only "Gradient step (finite differences):", "Central differences" and "Metropolis correction" (on; accepts or rejects each Langevin step so that the surface is sampled exactly), "Range (log10):", "Minima to refit:" (5), and under "Stopping and barriers" "Wall height:" (3, in units of how far the limit of the confidence region lies above the fit; confidence intervals only) or, for "Find other minima", "Wall (x SSE of the fit):" (10: another minimum behind a barrier higher than the wall is not found), "Minimum steps:", "Patience:", "Bias convergence:", "Bias strength:", "Hill width:". If the walkers find a deeper minimum than the fit, the intervals belong to that one. "Find other minima" lets the walkers climb out of the basin of the fit up to the wall, refits what they find (default 10 minima, a point of the same flat valley is not listed twice) and lists the minima by SSE with "SSE / SSE of the fit"; the line "Minima:" below the chart chooses one and **Load into model** sets the model's parameters to it, **Add as model** adds it as a new model; the minima are marked in the surfaces. The box is fixed to 6 log10 units around the fit ("Range (log10):" changes it): a minimum further away (a constant running to zero lies at -9 and more) is only found with a larger range. The result shows a table Parameter / Fit / Lower / Upper (a note where an end reaches the border of the sampled range: the data do not limit the constant there) and one chart. With more than one constant the line "Show:" above the chart chooses what it shows: "Profiles" (all constants; the interval is where a curve lies below the dashed line), "Profile: <constant>", and with "2D surfaces of the pairs" "Surface: <A> × <B>" for every pair (up to six): the sum of squares relative to the fit as coloured height lines, the limit of the confidence region thick, a circle at the fit; the shape of the region shows the correlation of the two. With exactly two constants the cells in and around the explored region are evaluated exactly (at most 3600 evaluations, extended until the region below the wall is enclosed), so the surface has no gaps and the intervals are exact in the sampled range; with three or more constants the holes enclosed by visited cells are closed by a search over the other constants (up to 3000 cells, takes several times the time of the sampling). While it runs, the page shows what happens now ("Setting up ...", "Sampling the landscape: n walkers, up to m steps each", "Evaluating the explored region ..." for two constants, "Refitting the minima ...") above a bar that starts again with every stage; the refits and the stages write nothing to the log, the log gets one line when the analysis is done. Takes seconds to a minute for one to three constants (synthetic NMR titrations, 4 threads).
- **Simplified MOC** (Fit dropdown) and the setting "Automatic Confidence Calculation using Simplified Model Comparison." (default on): quick F-test intervals.
- Results: a finished run shows the page "Statistics": its list has the analyses under "New analysis" (start one) and every finished run under "Results" ("Monte Carlo #1", "Cross Validation #1", "Grid Search #1", "Landscape Sampling #1", "Scan #1", ...; each with the icon of its method, the tooltip has the full name; pick one to look at it, right-click: Open in window, Save data, Drop raw data, Remove). **Open in window** (also below the result) moves a result into a window of its own; closing it puts it back. Monte Carlo has Histogram / Scatter Plot / Series Chart,
  "Confidence Intervall", bins, Export Results. Global search: Result List / Scatter Plot, filters Only Valid / Only Converged Models, Export Models.
- Model quality box ("Fit results"): Parameters fitted, Points used, Degrees of freedom, SAE, R2, F value, Mean error, Standard error, χ2.
- **Analyse and Compare** (Evaluation menu, checked tabs): Compare Reduction Analysis (Cutoff), **Compare AIC**, Compare Cross Validation / Monte Carlo (LOO/L2O/LXO).
- BC50 (concentration at 50 % binding) and Shannon entropy come with the search / Monte Carlo output (setting "# integration points for BC50").

## 6. Saving, exporting, settings

- **Save Project** writes all open projects into the file in "Filename:" (`.suprafit` = compressed json, `.json` = plain; several projects give one multi-project file).
  Models and statistics are included. Agent chats are stored in the project if "Store agent chats in project files" is on - save to keep them.
- Export: project tree Export; Export Data (input tables as text); Export Projects (one `.suprafit` per project); model Save / Save Constants / Export Simulated;
  Monte Carlo Export Results; search Export Models; charts Export Diagram (PNG).
- **Settings** tabs: General Settings (Threads, "Report thermodynamics in kcal/mol (instead of kJ/mol)", "Automatic open Thermogram Dialog", "Show hints in the import dialogs", "Show advanced options and simulation tools in GUI." shows
  further tools at once: more simulation chips, "Any Model…", "Drop Random", the LXO strategy and the thermogram "Advanced options"), Appearance ("Colour scheme:" System / Light / Dark, "Toolbar buttons:"), File Save Settings (agent chats, raw data), Standard Calculation
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
