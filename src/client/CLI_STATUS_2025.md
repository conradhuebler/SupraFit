# SupraFit CLI - Current Status & Documentation

## Overview
SupraFit CLI provides data generation, file analysis, and ML pipeline functionality with modern JSON configurations.

## Core Components
- **suprafit_cli.cpp/h**: Main CLI with modular JSON structure support
- **ml_pipeline_manager.cpp/h**: ML pipeline coordination and model evaluation
- **analyser.cpp/h**: File analysis and validation tools

## Current JSON Structure (2025)

### Modern Modular Configuration
```json
{
    "Main": {
        "OutFile": "test_output",
        "Repeat": 1,
        "UseModularStructure": true,
        "ProcessMLPipeline": true,
        "FitModels": true,
        "PostFitAnalysis": true
    },
    "Independent": {
        "Source": "generator",
        "Generator": {
            "Type": "equations",
            "DataPoints": 20,
            "Variables": 2,
            "Equations": "0.001|(X - 1) * 0.0001"
        }
    },
    "Dependent": {
        "Source": "generator",
        "Generator": {
            "Type": "model",
            "Series": 2,
            "Model": {"ID": 1},
            "GlobalRandomLimits": "[2 5]",
            "LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
        },
        "Noise": {
            "Type": "gaussian",
            "Std": [1e-3, 1e-3],
            "RandomSeed": 12345
        }
    },
    "AddModels": {
        "nmr_1_1": {
            "ID": 1,
            "Options": {"FastMode": true, "Convergency": 1e-7}
        },
        "nmr_1_2": {
            "ID": 2,
            "Options": {"FastMode": true}
        }
    },
    "PostFitAnalysis": {
        "enabled": true,
        "methods": [
            {"Method": 1, "MaxSteps": 2000, "VarianceSource": 2},
            {"Method": 4, "CVType": 1, "MaxSteps": 100}
        ]
    }
}
```

## Key Features

### Data Generation Methods
- **Equation-based**: `"Type": "equations"` with formula strings
- **Model-based**: `"Type": "model"` with SupraFit model IDs
- **File loading**: `"Source": "file"` with range selection
- **Noise application**: Gaussian, exportMC, or montecarlo

### ML Pipeline Capabilities
- Multi-model testing and comparison
- Statistical analysis (Monte Carlo, Cross-validation, Reduction)
- Parameter optimization with convergence detection
- Clean filename generation (`.suprafit` default)

### File Operations
- Comprehensive file analysis (`AnalyzeFile()`)
- Data structure validation
- Range-based data extraction
- Format conversion (JSON/DAT/SUPRAFIT)

## Usage Examples

### Basic Data Generation
```bash
# Generate modular NMR data
./bin/suprafit_cli -i input/Test_AddModels_v2.json

# Analyze file structure
./bin/suprafit_cli -i input/data_file.json

# Convert formats
./bin/suprafit_cli -i input.dat -o output.json
```

### Working Test Files
- `Test_AddModels_v2.json` - Complete ML pipeline with post-fit analysis
- `NMR_1_1_Modular.json` - Simple modular NMR data generation
- `Simple_ML_Test.json` - 3-model comparison workflow

## Implementation Status

### ✅ Completed (2025-08-30)
- **Modular JSON Structure**: Independent/Dependent separation
- **ML Pipeline Integration**: Complete data→fitting→evaluation workflow
- **Statistical Analysis**: Monte Carlo, Cross-validation via JobManager
- **File Operations**: Analysis, validation, range loading
- **Memory Management**: JSON-based transfer with QPointer safety
- **Clean Architecture**: DataGenerator/JobManager separation
- **File Extensions**: .suprafit default, .json optional
- **Qt6 Compatibility**: All compilation errors resolved

### Key Functions
```cpp
// Main data generation orchestrator
QVector<QJsonObject> GenerateDataWithModularStructure();

// Individual table generation
QJsonObject generateIndependentDataTable(const QJsonObject& config);
QJsonObject generateDependentDataTable(const QJsonObject& config, const QJsonObject& indepData);

// File operations
QJsonObject loadDataTableFromFile(const QJsonObject& fileConfig);
QPointer<DataClass> applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent);

// Analysis and validation
void AnalyzeFile();
```

### Configuration Controls
- `ProcessMLPipeline`: Enables enhanced ML workflow
- `UseModularStructure`: Activates Independent/Dependent structure
- `FitModels`: Controls parameter fitting execution
- `PostFitAnalysis`: Enables statistical analysis after fitting
- `AddModels`: Individual model configuration with options

### Output Files
- **Dataset**: `{OutFile}-0.suprafit` - Generated data with metadata
- **Models**: `{OutFile}-models-0.suprafit` - Fitted models with statistics
- **Analysis**: Comprehensive statistical evaluation included

## Dependencies
- Qt6 (Core, Test), DataGenerator, DataClass/DataTable, FileHandler, fmt

## Current Issues
None identified. System fully functional with clean architecture.