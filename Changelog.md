# The (incomplete) SupraFit Changelog

### SupraFit 2.x
- add some simple kinetic models
- more flexible chart configs
- include FlowLayout from Qt Examples for flexible UI
- enable ChaiScript, including one example scripted model
- add option to show dilution thermogram in experimental thermogram
- basic variance/covariance based wavelength selection
- add explicit UV/VIS models (NMR options will be deprecated)
- add spectra import dialog
- add ChaiScript to make user defineable models
- Thermograms can be imported via suprafit_cli

## SupraFit 2.0

- resize data structure after editing thermograms 
- tracking of thermogram raw files within suprafit project adjusted ( more will follow )

### SupraFit 2.0 Beta 2 (1.8)

- slightly improved Model Comparison algorithm
- export thermogram as *.dh files

### SupraFit 2.0 Beta 1 (1.7)

- add comment section for every project

### SupraFit 2.0 pre-Alpha
- UI improvments for Results Dialog 
- Message Dock writes message and warnings
- option to save specific results from results dialog
- option for CXO map calculation
- add option for advanced ui tools
- add leave-many-out cross validation
- some cosmetic changes in thermogram widget
- addition of some thermogram manipulation guidelines
- thermogram zoom with mouse wheel
- automatic resize of integration range (change in sign or threshold (iterative - fix point problem) )
- variable integration range within peaks
- add variable peak time for integration in thermogram dialog
- export integrated peaks from thermogram dialog directly to txt file
- add splitter widgets into thermogram dialog, make chart and table resizable, stores last geometry
- add calibration peak for ITC
- input file structure for data generation, adding models, and running jobs
- add foreward reduction analysis to input keywords
- add command line option for suprafit_cli for execute jobfile
- Input variable randomisation for meta models enabled
- data points colors in meta models follow the series color definition
- biscetion like single parameter optimisation for initial guess of K for NMR and ITC parameters
- make series names and color changeable in listchart
- git commit hash is stored in project files
- add optional Google Noto Fonts as default application and chart font (cmake option!)
- fix additional charts for concentrations, fast confidence and enthalpies/heats
- add some model overview to compare dialog
- temperature, concentrations and volume will be read from \*.itc file
- add Scatter Plot for Weakend Grid Search results
- Global search results table can now have a colorfull background (green - ok, yellow - not optimised, red - invalid)
- Some adjustments for charts export as png
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
