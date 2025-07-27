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

### ✅ Completed
- Modular JSON structure fully implemented
- DataGenerator integration with model-based generation
- File range loading with precise row/column selection
- Memory safety improvements (JSON-based data transfer)
- Legacy function cleanup (GenerateIndependent, GenerateDependent, etc.)
- Enhanced content generation with model details and input configuration storage

### 🔧 Key Functions
- `GenerateData()`: Main entry point routing to appropriate generation method
- `GenerateDataWithModularStructure()`: Modern modular approach
- `GenerateDataWithDataGenerator()`: Legacy DataGenerator support
- `AnalyzeFile()`: Read-only file structure analysis

## Usage Examples

### CLI Execution
```bash
# Modular structure with NMR 1:1 titration
./bin/suprafit_cli --config input/NMR_1_1_Modular.json

# File analysis (read-only)
./bin/suprafit_cli --analyze input/data_file.dat
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

---

## Variable Section (Short-term information, regularly updated)

### Recent Changes
- ✅ 2025-01-27: Removed obsolete generation functions (GenerateIndependent, GenerateDependent, GenerateNoisyIndependent, GenerateNoisyDependent)
- ✅ 2025-01-27: Cleaned up legacy comments and references
- ✅ 2025-01-27: Build system validates - no compilation errors

### Current Status
- NMR 1:1 modular titration system fully functional
- Generates realistic chemical shift data with proper random parameters
- Complete JSON structure with {"data": {...}} wrapper
- All SupraFit metadata (timestamps, git commits, UUIDs) working

### Known Issues
- None currently identified

### Testing
- Build: ✅ Success (warnings only, no errors)
- Functionality: ✅ NMR modular structure generates valid output
- Memory: ✅ No crashes with JSON-based data transfer

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks
- remove underscore from genereted file names
- add file extension (default *.suprafit, make *.json optional)
- suprafit_cli kann nun experimentelle Daten simulieren. darauf aufbauen sollen auch modelle an diese daten angefittet werden können und anschließend statistisch bewertet werden. zum einen kann ein solcher datensatz in einer pipeline mit einem programmaufruf verarbeitet werden 1) independent, dependend(modell/gleichung); 2) modell an datensatz testen oder ein solcher datensatz kann als suprafit_cli als argument mit einer weiteren steuerdatei übergeben werden. suprafit_cli muss also beides können. am ende erhalten wir eine projektdatei in der für einen datensatz (simuliert oder echt) verschiedene modelle angepasst werden können, und erst am ende per UI "fertig" ausgewertet werden
### Vision
- obige pipeline soll verwendet werden, um daten für KI zu generieren. die aktuelle ml_pipeline ist wahrscheinlich nicht in der lage, da sich erst danach der daten-generier-workflow entwickelt hat: ziel ist also, die aktuelle ml-pipeline in vollständig durch die schon implementierte und zukünftige entwickelte infrastruktur zu überführen