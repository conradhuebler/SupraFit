# SupraFit 2.0 pre-Alpha

- Implement Concentrational Descriptor in SupraFit
- Models with defined Reduction Analysis Cutoff (NMR Titration so far) automatically evaluate the Reduction Results
- add some benchmark stuff for 2:1/1:1/1:2 concentration solver
- add the legacy concentration solver for 2:1/1:1/1:2 systems for benchmark/comparison
- Charts representing Fast Confidence results added
- more tweaks for Grid Search method
- grid search integral below error curve
- one more latex snippet (regression analysis) + some adaptions
- variable chart size (x and y) on export, stronger axis, optional grid
- introduce binding concentration descriptor for mixed complexes
- compare single parameters in reduction analysis
- latex table snippets for monte carlo analysis
- add some more comparison to reduction analysis
- exported charts have now a transparent background and are croped to the opaque are (remove transparent bounding box)
- some more colors and titles in statistics output
- add optional in chart annotation as legend
- add custom charts for model (nmr titration models already got concentration chart)
- add recent files list and close all open projects to gui
- some changes in histograms
- introduce entropy for histograms and variable bin counts
- 'Drag and Drop' adding the same error matrix to all open models (inclusive saving and reusing the vector)
- Export all loaded projects indiviually as suprafit file or plain dat file
- 'Click and Copy' Copy of SSE, SEy and standard deviation to ClipBoard
- 'Drag and Drop' Model Simulation from calculated data
- improve monte carlos' standard deviation input (std, SEy, user defined)
- projects can be loaded by droping files in the tree view
- add fancy blur effect while loading projects
- Re-Enable local fits, results are stored in tree view
- Formatted Monte Carlo Confidence output for thermodynamics and BC50 values
- Scaling of x axis can be set in system parameters for several models
- Removing models from meta models removes them from meta models, not only from UI
- Chart support for meta models
- Add global search for local parameter in series, partly for meta models as well
- Some rework of the global search results table
- Import of thermogram from plain txt files possible, peak time and peak start have to be given for peak picking
- Improved thermodynamic output -> calculation of free enthalpy and entropy (if heat available) including confidence
- ToolSet::Confidence functions gives identical results compared to the octave quantile function
- More consistent parameter names 
- MetaModels are uniquely created and SupraFit doesn't crash due to wrongly
override system parameters on drag and drop in the project list

Changelog begin on 5.09.2018
