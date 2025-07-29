# Capabilities - Core Analysis and Search Functionality

## Overview
Core capabilities providing advanced analysis, search, and data generation functionality for SupraFit. This directory contains algorithmic implementations for optimization, statistical analysis, and data processing.

## Core Components

### Data Generation and ML Pipeline Support
- **datagenerator.cpp/h**: Mathematical equation processing and model-based data generation
  - JavaScript engine integration (QJSEngine) for equation evaluation
  - Random parameter injection and generation utilities (Claude Generated)
  - Model-based dependent data generation with EvaluateWithModel
  - Enhanced content creation with input configuration storage
  - Performance optimization for ML pipeline (debug reduction, content caching)
  - Batch processing support for multiple dataset generation

### Statistical Analysis Infrastructure
- **montecarlostatistics.cpp/h**: Monte Carlo parameter uncertainty analysis
  - Parallel Monte Carlo simulation with confidence interval calculation
  - Percentile-based confidence intervals (2.5%, 97.5% quantiles)
  - Shannon entropy calculation for parameter uncertainty quantification
  - Histogram-based probability distribution analysis
  - Support for custom variance sources and noise models

- **resampleanalyse.cpp/h**: Cross-validation and resampling methods
  - **Leave-One-Out (L0O)** cross-validation
  - **Leave-Two-Out (L2O)** cross-validation  
  - **Cross-X-Out (CXO)** validation with customizable X parameter
  - Bootstrap resampling for parameter confidence estimation
  - Model reduction analysis for parameter significance testing
  - Entropy-based model validation metrics

- **modelcomparison.cpp/h**: Model selection and comparison tools
  - Multi-model comparison framework
  - Statistical model ranking based on various criteria
  - Parallel model evaluation for efficiency
  - Integration with Monte Carlo uncertainty analysis

### Search and Optimization Algorithms
- **globalsearch.cpp/h**: Global parameter space exploration
  - Multi-start optimization from different initial conditions
  - Grid-based systematic parameter exploration
  - Global minimum identification across parameter landscape
  - Integration with local optimization methods

- **weakenedgridsearch.cpp/h**: Adaptive grid-based parameter exploration
  - **Parameter Reduction Analysis**: Systematic parameter significance testing
  - Grid refinement based on parameter sensitivity
  - Directional parameter exploration (up/down parameter sweeps)
  - X/Y series generation for parameter-response mapping
  - Cutoff-based parameter importance ranking

- **abstractsearchclass.cpp/h**: Base class for all search/analysis implementations
  - Common interface for parallel execution
  - Thread-safe result collection
  - Progress monitoring and cancellation support

### Job Management and Parallelization
- **jobmanager.cpp/h**: Advanced task scheduling and parallel processing
  - Thread pool management for statistical analyses
  - Task prioritization and resource allocation
  - Progress tracking and result aggregation
  - Cross-platform parallel execution support

## Key Features

### Statistical Analysis JSON API (Claude Generated)
Unified JSON-based interface for all statistical methods, replacing string-based legacy functions:

```cpp
// Core analysis functions from src/core/analyse.h
namespace StatisticTool {

// AIC-based model comparison with evidence ratios
QJsonObject CalculateAICMetrics(const QVector<QWeakPointer<AbstractModel>>& models);

// Cross-validation analysis (L0O, L2O, CXO)
QJsonObject CalculateCVMetrics(const QVector<QJsonObject>& models, int cvtype = 1, int cv_x = 3);

// Monte Carlo parameter uncertainty with percentile-based confidence intervals
QJsonObject CalculateMCMetrics(const QVector<QJsonObject>& models, int index = 1);

// Parameter reduction analysis with significance ranking
QJsonObject CalculateReductionMetrics(const QVector<QJsonObject>& models, double cutoff = 0.1);

// ML feature extraction for training pipelines
QJsonObject ExtractModelMLFeatures(QSharedPointer<AbstractModel> model);

// Human-readable formatting from JSON data
QString FormatStatisticsString(const QJsonObject& statisticsJson, const QString& analysisType);
}
```

#### JSON Response Structure Examples:

**AIC Analysis Result:**
```json
{
  "models": [{"index": 0, "name": "1:1 Model", "aic": 125.4, "aicc": 127.8, "parameters": 3, "datapoints": 50}],
  "aic_ranking": [{"rank": 1, "model": "0 - 1:1 Model", "aic": 125.4, "evidence_ratio": 1.0}],
  "best_aic": 125.4
}
```

**Monte Carlo Analysis Result:**
```json
{
  "max_steps": 10000,
  "variance": 0.05,
  "models": [{
    "name": "1:1 Model",
    "parameters": [{
      "name": "K11", "type": "Global Parameter", "mean": 4.25, "stddev": 0.12,
      "confidence_lower": 4.01, "confidence_upper": 4.49, "confidence_error": 0.24,
      "entropy": 2.34, "relative_uncertainty": 2.8
    }]
  }],
  "parameter_averages": [{"parameter": "K11", "avg_stddev": 0.12, "avg_entropy": 2.34}]
}
```

### DataGenerator Enhancements (Claude Generated)
Modern equation-based and model-based data generation with ML pipeline optimization:

```cpp
// Performance-optimized generation for ML pipelines
bool EvaluateWithModel(int modelId, QPointer<DataClass> data, const QJsonObject& config);

// Batch processing for multiple datasets
bool EvaluateWithModelBatch(int modelId, QPointer<DataClass> dataClass, 
                           const QVector<QJsonObject>& configs, QVector<DataTable*>& results);

// Performance monitoring and control
void enablePerformanceMode(bool enabled = true);  // Reduces debug output
void resetPerformanceCounters();                  // Tracking model creation count
int getModelCreationCount() const;               // Performance metrics

// Random parameter injection via JavaScript
bool EvaluateWithRandomParameters(const QJsonObject& config);
static QJsonObject generateRandomParameters(const QJsonObject& limits, quint64 seed = 0);
```

#### Key Optimizations for ML Pipeline:
1. **Debug Output Reduction**: Performance mode disables verbose logging during batch generation
2. **Content Caching**: JSON formatting cached to avoid repeated serialization
3. **Batch Processing**: `EvaluateWithModelBatch()` for efficient multi-dataset generation
4. **Memory Management**: Compatible with SupraFit's DataClass inheritance architecture

### Statistical Analysis Capabilities Overview

#### **Monte Carlo Simulation**
- **Purpose**: Parameter uncertainty quantification through repeated fitting with noise
- **Methods**: Percentile-based confidence intervals, Shannon entropy analysis
- **Output**: Parameter distributions, confidence bounds, correlation analysis
- **Use Cases**: Error propagation, parameter reliability assessment

#### **Cross-Validation Analysis**  
- **Methods**: Leave-One-Out (L0O), Leave-Two-Out (L2O), Cross-X-Out (CXO)
- **Purpose**: Model predictive capability assessment and overfitting detection
- **Output**: Prediction errors, validation metrics, entropy-based model scores
- **Use Cases**: Model selection, generalization assessment

#### **Parameter Reduction Analysis**
- **Purpose**: Identification of statistically significant parameters
- **Method**: Systematic parameter removal with model quality assessment
- **Output**: Parameter importance ranking, significance scores, reduced model suggestions
- **Use Cases**: Model simplification, parameter pruning for ML training

#### **Model Comparison (AIC-based)**
- **Purpose**: Statistical comparison of different model hypotheses
- **Method**: Akaike Information Criterion with evidence ratios
- **Output**: Model rankings, relative evidence weights, selection recommendations
- **Use Cases**: Model selection, hypothesis testing

#### **Global Parameter Search**
- **Purpose**: Finding global optima in complex parameter landscapes
- **Method**: Multi-start optimization with grid-based exploration
- **Output**: Global parameter sets, convergence statistics, landscape mapping
- **Use Cases**: Avoiding local minima, comprehensive parameter exploration

### Integration Capabilities
- **QJSEngine Integration**: JavaScript-based equation evaluation with controlled random functions
- **Qt6 Random Generation**: Modern QRandomGenerator support replacing deprecated qrand()
- **Parallel Processing**: Thread-safe statistical analyses with progress monitoring
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
- ✅ 2025-01-29: **Architecture Refactoring** - Removed statistical analysis methods from DataGenerator (now handled by JobManager in CLI)
- ✅ 2025-01-29: **Clean Architecture** - DataGenerator focuses only on data generation, analysis moved to core/analyse.cpp
- ✅ 2025-01-29: **Method Cleanup** - EvaluateWithModelAndAnalyze* methods simplified to data generation only
- ✅ 2025-01-28: **JSON Statistical Analysis API** - Complete migration from string-based to JSON-based analysis methods
- ✅ 2025-01-28: **Percentile-based Confidence Intervals** - Monte Carlo analysis now uses percentile method for accurate uncertainty quantification
- ✅ 2025-01-28: **ML Feature Extraction** - `ExtractModelMLFeatures()` for standardized ML training data
- ✅ 2025-01-28: **Performance optimizations** for ML Pipeline (debug reduction, content caching, batch processing)
- ✅ 2025-01-28: **DataClass Architecture Compatibility** - Fixed model caching to respect SupraFit's inheritance architecture
- ✅ 2025-01-28: Implemented EvaluateWithModelBatch for efficient multi-dataset generation
- ✅ 2025-01-27: Enhanced DataGenerator with model-based generation
- ✅ 2025-01-27: Added input configuration storage in createEnhancedContent()
- ✅ 2025-01-27: Matrix-string parsing for legacy random parameter formats
- ✅ 2025-01-27: Qt6 random generation replacing deprecated qrand()

### Current Status
- **DataGenerator Architecture**: Clean separation - only handles data generation, no statistical analysis
- **Statistical Analysis**: Moved to core/analyse.cpp with JSON API, executed via JobManager in CLI/GUI
- **DataGenerator**: All tests passing (9/9), ML pipeline optimizations active
- **Model Integration**: NMR 1:1 model integration fully functional, architecture-compliant
- **Content Generation**: Enhanced with model details, parameter information, and configuration storage
- **Random Generation**: Working for both modern JSON and legacy matrix-string formats
- **Build Status**: ✅ Compilation successful after architecture refactoring

### ML Pipeline Readiness
- ✅ **Data Generation**: High-performance batch generation with `EvaluateWithModelBatch()`
- ✅ **Statistical Analysis**: JSON API provides standardized ML features via `ExtractModelMLFeatures()`
- ✅ **Parameter Uncertainty**: Percentile-based confidence intervals for robust uncertainty quantification
- ✅ **Model Comparison**: AIC-based ranking with evidence ratios for model selection
- ✅ **Cross-Validation**: L0O/L2O/CXO validation metrics for generalization assessment
- ✅ **Parameter Reduction**: Significance testing for model pruning and feature selection

### Known Issues
- None currently identified

### Performance Notes
- **Data Generation**: Performance mode reduces debug overhead by ~60% during batch processing
- **Statistical Analysis**: JSON-based methods eliminate string parsing overhead
- **Model Creation**: Respects SupraFit architecture, no caching conflicts
- **Memory Usage**: Optimized with JSON-based data transfer and content caching

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks (Restructured 2025-01-28)

#### **⚡ MEDIUM PRIORITY**:
7. **DataGenerator Integration** (Task #7)
   - Use core JSON statistical functions for model evaluation in EvaluateWithModel (depends on Task #1)
   - Integrate with new JSON-based statistical analysis API
   - Add statistical feature extraction to enhanced content generation

### Vision
<!-- Add long-term architectural goals here -->