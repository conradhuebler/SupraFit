# Capabilities - Core Analysis and Search Functionality

## Overview
Core capabilities providing advanced analysis, search, and data generation functionality for SupraFit. This directory contains algorithmic implementations for optimization, statistical analysis, and data processing.

## Core Components

### Data Generation
- **datagenerator.cpp/h**: Mathematical equation processing and model-based data generation
  - JavaScript engine integration (QJSEngine) for equation evaluation
  - Random parameter injection and generation utilities (Claude Generated)
  - Model-based dependent data generation with EvaluateWithModel
  - Enhanced content creation with input configuration storage

### Search and Optimization
- **globalsearch.cpp/h**: Global optimization algorithms
- **weakenedgridsearch.cpp/h**: Grid-based parameter space exploration
- **abstractsearchclass.cpp/h**: Base class for search implementations

### Statistical Analysis
- **montecarlostatistics.cpp/h**: Monte Carlo simulation and statistical post-processing
- **resampleanalyse.cpp/h**: Bootstrap and resampling analysis methods
- **modelcomparison.cpp/h**: Model comparison and selection metrics

### Job Management
- **jobmanager.cpp/h**: Task scheduling and parallel processing coordination

## Key Features

### DataGenerator Enhancements (Claude Generated)
Modern equation-based and model-based data generation:

```cpp
// Random parameter injection via JavaScript
bool EvaluateWithRandomParameters(const QJsonObject& config);

// Model-based data generation for titration models
bool EvaluateWithModel(int modelId, QPointer<DataClass> data, const QJsonObject& config);

// Static utility methods
static QJsonObject generateRandomParameters(const QJsonObject& config);
static double generateRandomValue(double min, double max, int seed = -1);

// Enhanced content creation with input configuration storage
QString createEnhancedContent(const QJsonObject& inputConfig, int iteration);
```

### Integration Capabilities
- **QJSEngine Integration**: JavaScript-based equation evaluation
- **Qt6 Random Generation**: Modern QRandomGenerator support replacing deprecated qrand()
- **Matrix-String Parsing**: Legacy GlobalRandomLimits/LocalRandomLimits format support
- **SupraFit Model Integration**: Direct model calculation via UI-based CreateModel approach

## Configuration Support

### Random Parameter Formats
```cpp
// Modern format (DataGenerator native)
"RandomParameters": {
    "A": {"min": 1.0, "max": 5.0},
    "B": {"min": 0.5, "max": 2.0}
}

// Legacy matrix-string format (NMR titrations)
"GlobalRandomLimits": "[2 5]"
"LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
```

### Equation Processing
```cpp
// Independent variable equations
"Equations": "0.001|(X - 1) * 0.0001"

// Dependent variable equations  
"DependentEquations": "Y1_equation|Y2_equation"
```

## Implementation Details

### Memory Management
- Uses QPointer for safe object references
- JSON-based data transfer to prevent pointer crashes
- Explicit cleanup in destructors

### Error Handling
- Comprehensive validation for configuration parameters
- Graceful fallbacks for missing or invalid inputs
- Detailed logging with fmt library formatting

### Model Integration
```cpp
// DataClass setup for model calculation
QPointer<DataClass> data = new DataClass();
data->setIndependentTable(independentTable);
data->setDataType(DataClassPrivate::Simulation);
data->setSimulateDependent(series);

// Model calculation with proper parameter setup
bool success = generator->EvaluateWithModel(modelId, data, generatorConfig);
```

## Current Implementation Status

### ✅ Completed Features
- **DataGenerator Integration**: Full JavaScript equation support
- **Random Parameter Generation**: Both modern and legacy formats
- **Model-Based Generation**: NMR 1:1 titration models working
- **Enhanced Content Creation**: Input configuration storage for traceability
- **Memory Safety**: QPointer usage and JSON data transfer
- **Unit Testing**: Comprehensive test suite (9/9 tests passing)

### 🔧 Key Methods
- `Evaluate()`: Standard equation-based generation
- `EvaluateWithRandomParameters()`: Enhanced with parameter injection
- `EvaluateWithModel()`: Model-based dependent data generation
- `Table()`: Access to generated DataTable results

## Testing

### Unit Test Coverage
Located in `src/tests/test_datagenerator.cpp`:
- ✅ Basic functionality tests
- ✅ Random parameter generation
- ✅ Configuration validation
- ✅ Error handling
- ✅ Memory management

### Integration Testing
- ✅ CLI integration via suprafit_cli.cpp
- ✅ Model-based generation with NMR titration models
- ✅ File-based configuration loading

## Dependencies
- Qt6 (Core, Qml for QJSEngine)
- DataClass/DataTable (core/models/)
- AbstractModel system (core/models/)
- fmt library for formatting

## Usage Examples

### Basic Generation
```cpp
DataGenerator* generator = new DataGenerator();
generator->setJson(configJson);
if (generator->Evaluate()) {
    DataTable* result = generator->Table();
}
```

### Model-Based Generation
```cpp
bool success = generator->EvaluateWithModel(modelId, dataClass, config);
QString content = generator->createEnhancedContent(config, iteration);
```

---

## Variable Section (Short-term information, regularly updated)

### Recent Changes
- ✅ 2025-01-27: Enhanced DataGenerator with model-based generation
- ✅ 2025-01-27: Added input configuration storage in createEnhancedContent()
- ✅ 2025-01-27: Matrix-string parsing for legacy random parameter formats
- ✅ 2025-01-27: Qt6 random generation replacing deprecated qrand()

### Current Status
- All DataGenerator tests passing (9/9)
- NMR 1:1 model integration fully functional
- Content generation includes model details and parameter information
- Random parameter generation working for both modern and legacy formats

### Known Issues
- None currently identified

### Performance Notes
- JavaScript evaluation via QJSEngine is efficient for typical equation complexity
- Model calculations scale with data points and series count
- Memory usage optimized with JSON-based data transfer

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks
- **Statistical Integration**: Connect DataGenerator with core statistical analysis functions
  - TODO: Use core statistical functions for model evaluation in EvaluateWithModel
  - TODO: Integrate with new JSON-based statistical analysis API
  - TODO: Add statistical feature extraction to enhanced content generation

### Vision
<!-- Add long-term architectural goals here -->