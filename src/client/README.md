# SupraFit CLI - Architecture and Implementation Guide

## Overview

SupraFit CLI is a command-line interface for simulation and analysis of supramolecular binding models. The system enables automated generation of simulated data, fitting to various models, and statistical analysis of results for machine learning applications.

## Architecture

### Core Components

#### 1. SupraFitCli (`suprafit_cli.h/cpp`)
**Main CLI interface class**
- JSON-based configuration via `setControlJson()`
- Data generation: `GenerateIndependent()`, `GenerateDependent()`, `GenerateNoisyDependent()`
- Model management: `AddModels()`, `AddModel()`
- Job execution: `PerformeJobs()`

**Key methods:**
```cpp
void setControlJson(const QJsonObject& control);
QVector<QJsonObject> GenerateData();
void Analyse(const QJsonObject& analyse, const QVector<QJsonObject>& models);
```

#### 2. JobManager (`src/capabilities/jobmanager.h/cpp`)
**Central job management system**
- Supported methods:
  - MonteCarlo statistics
  - GridSearch (WeakenedGridSearch)
  - ModelComparison/FastConfidence  
  - Cross-Validation/Reduction Analysis
  - GlobalSearch

**Configuration via JSON blocks:**
- `MonteCarloConfigBlock`: Variance, steps, confidence
- `ModelComparisonConfigBlock`: Parameter comparisons, F-statistics
- `GridSearchConfigBlock`: Parameter space exploration
- `ResampleConfigBlock`: Cross-validation settings

#### 3. AbstractModel (`src/core/models/AbstractModel.h`)
**Base class for all binding models**

**Available model types:**
- **NMR**: `nmr_ItoI` (1), `nmr_IItoI_ItoI` (2), `nmr_ItoI_ItoII` (3), `nmr_any` (4)
- **ITC**: `itc_ItoI` (43), `itc_IItoI` (44), `itc_ItoII` (45), `itc_n_ItoI` (46), `itc_any` (47)
- **Fluorescence**: `fl_ItoI` (48), `fl_ItoI_ItoII` (49), `fl_IItoI_ItoI` (50)
- **UV-Vis**: `uv_vis_ItoI` (51), `uv_vis_IItoI_ItoI` (52), `uvvis_any` (53)
- **Kinetics**: `MonoMolecularModel`, `BiMolecularModel`, `FlexMolecularModel`
- **Thermodynamics**: `Arrhenius`, `Eyring`, `BETModel`
- **Script**: `ScriptModel` (for custom models)

**Statistical functions:**
```cpp
qreal AIC() const;           // Akaike Information Criterion
qreal AICc() const;          // Corrected AIC
qreal SSE() const;           // Sum of Squared Errors
qreal SEy() const;           // Standard Error
qreal ChiSquared() const;    // Chi-Squared
```

#### 4. DataClass (`src/core/models/dataclass.h`)
**Data management system**
- Supports data types: `Table`, `Thermogram`, `Spectrum`, `Simulation`
- Independent/Dependent data tables with variable dimensions
- Raw/Processed data separation
- System parameter management

## Data Structure

### Variable Input/Output Format
The data structure is **flexible** depending on the experimental technique:

#### NMR Titration Data (e.g., `1_1_001.dat`)
```
# 2 independent + 7 dependent variables
[Host]  [Guest]  Signal1  Signal2  Signal3  Signal4  Signal5  Signal6  Signal7
0.001   0.000    6.31089  6.06753  2.33626  2.21989  4.27553  4.38816  3.18396
0.001   0.00013  6.31029  6.06437  2.35431  2.23115  4.27764  4.53730  3.19356
```

#### Enzyme Kinetics Data (e.g., `MM.dat`)
```
# 1 independent + 1 dependent variable
[Substrate]  Rate
0            -2
2.5          153
5            231
```

#### ITC Data
```
# 1 independent (injection volume) + multiple dependent (heat signals)
# Concentrations are calculated internally from injection volumes
```

### Configuration Parameters
- `"IndependentRows"`: Number of independent variable columns
- `"independent"`: Number of independent variables in simulation
- `"dependent"`: Number of dependent variables (response signals)

## JSON Configuration Examples

### Basic Configuration Structure
```json
{
  "Main": {
    "InFile": "1_1_001.dat",
    "OutFile": "output_prefix",
    "IndependentRows": 2,        // 2 concentration columns
    "Threads": 4,
    "Guess": true,
    "Fit": true
  },
  "Models": {
    "1": 1,    // nmr_ItoI
    "2": 2,    // nmr_IItoI_ItoI
    "3": 3     // nmr_ItoI_ItoII
  },
  "Jobs": {
    "1": {
      "Method": 1,              // MonteCarlo
      "MaxSteps": 1000,
      "VarianceSource": "SEy",
      "Bootstrap": false
    }
  }
}
```

### Data Generation Configuration
```json
{
  "Main": {
    "InFile": "tabelle.dat",
    "OutFile": "Simulated_1-1_Model",
    "IndependentRows": 2,
    "Threads": 12,
    "GenerateData": {
      "Series": 8,                    // Number of dependent signals
      "Model": 1,                     // Model type
      "Repeat": 20,                   // Number of simulations
      "Variance": 3e-3,               // Noise level
      "GlobalRandomLimits": "[1 5]",  // Parameter bounds
      "LocalRandomLimits": "[4 8]"
    }
  }
}
```

### NMR Simulation Configuration
```json
{
  "datapoints": 20,
  "equations": "0.001|2*(X-1)/10000",   // Concentration equations
  "independent": 2,                      // Host + Guest concentrations
  "dependent": 5,                        // NMR signals
  "models": {
    "0": 1,    // Test different binding models
    "1": 2,
    "2": 3,
    "3": 4
  }
}
```

### Method Types
- **Method 1**: MonteCarlo statistics
- **Method 4**: Cross-Validation (CXO: 1=LOO, 2=LTO, 3=LMO)
- **Method 5**: Reduction Analysis
- **Method 8**: Model Comparison/FastConfidence

### Data Generation Tasks
Available via `Tasks` string (pipe-separated):
- `"GenerateIndependent"`: Creates independent variables (concentrations, volumes)
- `"GenerateDependent"`: Calculates dependent variables (signals) with model
- `"GenerateNoisyIndependent"`: Adds noise to independent variables
- `"GenerateNoisyDependent"`: Adds noise to dependent variables

## Technique-Specific Data Formats

### NMR Titrations
- **Independent**: Host concentration, Guest concentration
- **Dependent**: Multiple chemical shifts (δ values)
- **Model calculates**: Bound/free species concentrations → chemical shift changes

### ITC (Isothermal Titration Calorimetry)
- **Independent**: Injection volume
- **Dependent**: Heat per injection
- **Model calculates**: Concentrations from cumulative injections → binding heat

### Fluorescence/UV-Vis
- **Independent**: Concentration(s)
- **Dependent**: Intensity/Absorbance at different wavelengths
- **Model calculates**: Bound species → spectral response

### Enzyme Kinetics
- **Independent**: Substrate concentration
- **Dependent**: Reaction rate
- **Model calculates**: Michaelis-Menten kinetics

## Workflow for ML Training Pipeline

### 1. Data Simulation
```cpp
// Load JSON configuration
SupraFitCli cli;
cli.setControlJson(config);

// Generate simulated data with variable dimensions
QVector<QJsonObject> data = cli.GenerateData();
```

### 2. Model Fitting
```cpp
// JobManager for statistical analysis
JobManager manager;
manager.setModel(model);
manager.AddSingleJob(monte_carlo_job);
manager.RunJobs();
```

### 3. Result Extraction
Fit quality is evaluated through multiple metrics:
- **SSE**: Sum of Squared Errors
- **AIC/AICc**: Information Criteria
- **SEy**: Standard Error
- **R²**: Coefficient of determination
- **χ²**: Chi-Squared

## Implementation Status

### Already Implemented ✅
- JSON-based configuration
- Variable input/output dimensions
- Model library (>20 binding models)
- JobManager with statistical methods
- Project export/import
- Noise addition to data
- Parameter randomization
- Multi-technique support

### Recent Extensions ✅
- **Mathematical Data Generation**: Use equations like "X|X*X" to generate data
- **Automatic Dependent Variables**: Auto-generate random dependent data when not specified
- **File Analysis Tool**: Comprehensive analysis of SupraFit files without processing
- **Enhanced Data Loading**: Fixed data access issues and improved reliability
- **Clean Output**: Removed debug clutter, added informative progress messages
- **Output File Management**: Track and display output file configurations

### Extensions for ML Pipeline 🚧
- Batch processing of many simulations
- Structured output for ML training
- Automated parameter variation
- Performance optimization for large datasets

## Recent Improvements (2025-07-18)

### Fixed Issues ✅
1. **Data Loading Pipeline**: Fixed QModelIndex access issue preventing proper data display
2. **Matrix Access**: Changed from `m_data->DependentModel()->data(index).toDouble()` to `m_data->DependentModel()->data(i, j)`
3. **Debug Output**: Cleaned up excessive debug statements, added clean progress reporting
4. **File Analysis**: Added comprehensive file analysis tool showing dimensions, data samples, and configuration
5. **Output Management**: Added output file information to analysis reports

### Working Examples ✅
```bash
# Generate data using mathematical equations
suprafit_cli -i test_equations.json

# Analyze existing data files
suprafit_cli -i experimental_data.json

# Load and save data without processing
suprafit_cli -i input.dat -o output.json
```

## Next Steps

1. **Batch Simulation Framework**: Automated generation of many simulated experiments
2. **ML Data Export**: Structured output of features and labels
3. **Parameter Sampling**: Intelligent variation of model parameters
4. **Performance Tests**: Benchmarking for large datasets
5. **Integration Tests**: End-to-end pipeline tests

## New Features (2025-07-18)

### Data Generation Pipeline
- **GenerateInputData()**: Creates data using mathematical equations via DataGenerator
- **GenerateDataOnly()**: Simple load/save verification without model processing  
- **AnalyzeFile()**: Comprehensive file analysis with detailed output information
- **Automatic Dependent Data Generation**: Creates random dependent variables when not specified
- **Enhanced Data Access**: Fixed data loading issues with direct matrix access

### Mathematical Data Generation
```cpp
// Configuration for equation-based data generation
{
  "rows": 10,
  "equations": "X|X*X",           // Independent variables: X, X²
  "dependent_columns": 2,          // Auto-generate 2 random dependent columns
  "dependent_equations": "Y1|Y2"   // Optional dependent equations
}
```

### File Analysis Tool
The CLI now provides detailed file analysis when no output is specified:
```bash
# Analyze any SupraFit file
suprafit_cli -i data_file.json

# Output includes:
# - File information (size, path, extension)
# - Data structure (dimensions, headers, sample data)
# - Configuration analysis
# - Model and job information
# - System parameters
# - Output file settings
```

### Data Loading Improvements
- **Fixed Matrix Access**: Corrected data access from QModelIndex to direct matrix access
- **Enhanced Debug Output**: Clean, informative output with proper data display
- **Better Error Handling**: Improved validation and error reporting
- **Output File Tracking**: Shows configured output filenames in analysis

## Dependencies

- **Qt6**: Core, JSON handling
- **Eigen3**: Numerical computations
- **fmt**: Modern C++ formatting library
- **OpenMP**: Parallelization (optional)
- **ChaiScript**: Script models (optional)

## Notes

The codebase supports complex binding models used in supramolecular chemistry:
- 1:1, 2:1, 1:2, 2:2 binding stoichiometries
- Multiple spectroscopic techniques with different data formats
- Statistical validation methods
- Parameter confidence intervals
- Model comparison tools
- Flexible data dimensionality (1-N independent, 1-M dependent variables)