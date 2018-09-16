# SupraFit 2.0 pre-Alpha

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