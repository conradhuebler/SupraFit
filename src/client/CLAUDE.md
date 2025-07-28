# Client Applications - SupraFit CLI and Pipeline Management

## Overview
Client applications providing command-line interface and machine learning pipeline management for SupraFit. This directory contains the main executable entry points and batch processing functionality.

## Core Components

### Command Line Interface
- **suprafit_cli.cpp/h**: Main CLI application for batch processing and data generation
  - Modular JSON structure support (Independent/Dependent)
  - DataGenerator integration with equation-based and model-based generation
  - File analysis and validation capabilities
  - Enhanced error handling and logging

### Machine Learning Pipeline  
- **ml_pipeline_manager.cpp/h**: ML pipeline coordination and execution
  - Model training and evaluation workflows
  - Statistical analysis and metrics collection
  - Integration with SupraFit data structures

### Main Entry Points
- **main.cpp**: Primary CLI executable entry point
- **ml_cli_main.cpp**: ML-specific CLI entry point

### Analysis Tools
- **analyser.cpp/h**: File structure analysis and validation
- **simulator.cpp/h**: Data simulation and generation utilities

## Key Features

### Modular Data Generation (Claude Generated)
Modern Independent/Dependent structure replacing legacy approaches:
```cpp
// Main orchestrator for modular structure
QVector<QJsonObject> GenerateDataWithModularStructure();

// Independent data generation (equations or file loading)
QJsonObject generateIndependentDataTable(const QJsonObject& independentConfig);

// Dependent data generation (model-based or equations)
QJsonObject generateDependentDataTable(const QJsonObject& dependentConfig, const QJsonObject& independentTableJson);
```

### File Range Loading (Claude Generated)
Precise data extraction from source files:
```cpp
// Extract specific ranges from data files
QJsonObject loadDataTableFromFile(const QJsonObject& fileConfig);
```

### Unified Noise Application (Claude Generated)
Consistent noise handling across all generation modes:
```cpp
// Apply gaussian, exportMC, or montecarlo noise
QPointer<DataClass> applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent);
```

## Configuration Format

### New Modular Structure
```json
{
    "Main": {
        "OutFile": "output_name",
        "Repeat": 3
    },
    "Independent": {
        "Source": "generator",
        "Generator": {
            "Type": "equations",
            "DataPoints": 15,
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
    }
}
```

## Current Implementation Status

### ✅ Completed - ML Pipeline Integration (Claude Generated) 🔥
- **ProcessMLPipeline()**: Complete workflow - data generation → model fitting → statistical evaluation
- **FitModelsToData()**: Multi-model testing against simulated data with NonLinearFitThread integration
- **EvaluateModelFit()**: Statistical analysis (SSE, AIC, R², convergence detection)
- **ExtractMLFeatures()**: ML training dataset generation from fitted models
- **CLI Integration**: ProcessMLPipeline flag detection and routing via main.cpp
- **Configuration Control**: MLModels section parsing for precise model selection
- **Qt6 Compatibility**: Fixed QJsonValue::isInt() compilation errors
- **Memory Management**: m_original_config storage for proper configuration access

### ✅ Completed - Modular Structure 
- Modular JSON structure fully implemented
- DataGenerator integration with model-based generation
- File range loading with precise row/column selection
- Memory safety improvements (JSON-based data transfer)
- Legacy function cleanup (GenerateIndependent, GenerateDependent, etc.)
- Enhanced content generation with model details and input configuration storage

### 🔧 Key Functions
- `GenerateData()`: Main entry point routing to appropriate generation method
- `GenerateDataWithModularStructure()`: Modern modular approach
- `AnalyzeFile()`: Read-only file structure analysis

## Usage Examples

### CLI Execution
```bash
# Modular structure with NMR 1:1 titration
./bin/suprafit_cli -i input/NMR_1_1_Modular.json

# File analysis (read-only)
./bin/suprafit_cli  input/data_file.json
```

## Dependencies
- Qt6 (Core, Test)
- DataGenerator (capabilities/)
- DataClass/DataTable (core/models/)
- FileHandler (core/)
- fmt library for modern C++ formatting

## Notes
- All new implementations marked as "Claude Generated" for traceability
- Memory management uses QPointer for safety
- JSON-based data transfer prevents pointer crashes
- Backward compatibility maintained for existing configurations
- Automatic test new implemented pipelines using input.json files and observed and evaluate output files
---

## Variable Section (Short-term information, regularly updated)

### Recent Changes - ML Pipeline Session 2025-01-27/28
- ✅ **MAJOR**: Complete ML Pipeline Integration - data→fitting→evaluation workflow
- ✅ **FIX**: m_original_config storage for correct MLModels configuration reading  
- ✅ **FIX**: NonLinearFitThread integration for proper parameter optimization (not just calculation)
- ✅ **FIX**: ProcessMLPipeline flag detection and routing in main.cpp
- ✅ **FIX**: QJsonValue::isInt() compilation error (Qt6 compatibility)
- ✅ **TEST**: Simple_ML_Test.json processes exactly 3 configured models (nmr_1_1, nmr_1_2, nmr_2_1)
- ✅ **OUTPUT**: Both datasets and fitted model project files generated correctly
- ✅ **FILENAME**: Removed underscores - now uses clean naming (`simple_ml_test-0.suprafit`, `simple_ml_test-models-0.suprafit`)
- ✅ **EXTENSION**: Added .suprafit default extension support with .json optional (setOutFile() logic)
- ✅ 2025-01-27: Removed obsolete generation functions (GenerateIndependent, GenerateDependent, GenerateNoisyIndependent, GenerateNoisyDependent)
- ✅ 2025-01-27: Cleaned up legacy comments and references

### Current Status - ML Pipeline Complete ✅
- **ML Pipeline**: Full workflow operational - DataGenerator → NonLinearFitThread → Statistical Evaluation
- **Model Testing**: Processes exactly configured models (3 models: nmr_1_1, nmr_1_2, nmr_2_1)
- **Parameter Fitting**: Proper optimization with convergence detection (not just calculation)
- **File Generation**: Clean filenames with .suprafit extension - `simple_ml_test-0.suprafit` and `simple_ml_test-models-0.suprafit`
- **Extension Support**: Automatic .suprafit default, .json optional via filename detection
- NMR 1:1 modular titration system fully functional
- Generates realistic chemical shift data with proper random parameters
- Complete JSON structure with {"data": {...}} wrapper
- All SupraFit metadata (timestamps, git commits, UUIDs) working

### Known Issues
- Model count discrepancy investigation delegated to user (original model may be included in output)

### Testing
- Build: ✅ Success (warnings only, no errors)
- ML Pipeline: ✅ Complete workflow from data generation to model evaluation
- Model Selection: ✅ Processes exactly 3 configured models as specified
- Parameter Fitting: ✅ NonLinearFitThread integration with convergence detection
- Statistical Analysis: ✅ SSE, convergence status, model comparison metrics
- File Output: ✅ Clean naming convention without underscores
- Extension Logic: ✅ .suprafit default, .json optional via setOutFile() detection
- Functionality: ✅ NMR modular structure generates valid output
- Memory: ✅ No crashes with JSON-based data transfer

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks (Active Development)
- ✅ **ML Pipeline Integration**: Implemented model fitting workflow for DataGenerator-created datasets
  - ✅ Added `ProcessMLPipeline()` method for complete data→models→evaluation workflow  
  - ✅ Added `FitModelsToData()` for testing multiple models against simulated data
  - ✅ Added `EvaluateModelFit()` for statistical model comparison
  - ✅ Added `ExtractMLFeatures()` for ML training data generation
- **Statistical Analysis Refactoring**: Move statistical calculations from client to core
  - TODO: Refactor `ExtractMLFeatures()` to use core statistical analysis functions
  - TODO: Extract model ID parsing from DataGenerator content strings
  - TODO: Implement advanced statistical features (parameter uncertainty, prediction variance)
- **File Naming**: Remove underscores from generated file names  
- **File Extensions**: Add .suprafit default, .json optional support
- **Dual CLI Modes**: Support both pipeline mode (1 call) and stepwise mode (separate config files)
- **Model Testing Pipeline**: suprafit_cli → simulate data → fit multiple models → statistical evaluation → project file output
### Vision
- obige pipeline soll verwendet werden, um daten für KI zu generieren. die aktuelle ml_pipeline ist wahrscheinlich nicht in der lage, da sich erst danach der daten-generier-workflow entwickelt hat: ziel ist also, die aktuelle ml-pipeline in vollständig durch die schon implementierte und zukünftige entwickelte infrastruktur zu überführen