# SupraFit CLI Usage Examples

## Overview

This document provides practical examples of using SupraFit CLI for data generation, analysis, and ML pipeline operations in supramolecular chemistry. Updated with the latest fixes and improvements (2025-07-18).

## Recent Improvements ✅

### Fixed Data Loading Issues
- **Problem**: Dependent data was showing as zeros due to incorrect QModelIndex access
- **Solution**: Changed to direct matrix access `m_data->DependentModel()->data(i, j)`
- **Result**: Both independent and dependent data now display correctly

### Enhanced File Analysis
- **New Feature**: Comprehensive file analysis tool
- **Usage**: `suprafit_cli -i filename.json` (without -o flag)
- **Output**: Detailed file information, data dimensions, sample values, configuration

### Mathematical Data Generation
- **New Feature**: Generate data using mathematical equations
- **Usage**: Configure with `"equations": "X|X*X"` in JSON
- **Result**: Creates systematic data series (1,1), (2,4), (3,9), etc.

## Basic Usage

### 1. File Analysis (New Feature)

```bash
# Analyze any SupraFit file - shows comprehensive information
suprafit_cli -i test_data.json

# Output includes:
# - File information (size, path, extension)
# - Data structure (dimensions, headers)
# - Sample data (first 5 rows of independent/dependent)
# - Configuration analysis
# - Output file settings
```

### 2. Mathematical Data Generation (New Feature)

```bash
# Generate data using equations
suprafit_cli -i config_with_equations.json

# Example configuration:
# {
#   "rows": 10,
#   "equations": "X|X*X",           // Creates X and X² columns
#   "dependent_columns": 2          // Auto-generates 2 random columns
# }
```

### 3. Simple Data Operations

```bash
# Load and save data (format conversion)
suprafit_cli -i input.dat -o output.json

# Load, analyze, but don't save
suprafit_cli -i data_file.suprafit
```

### 4. Single Pipeline Execution

```cpp
#include "ml_pipeline_manager.h"
#include "src/core/jsonhandler.h"

// Create pipeline manager
MLPipelineManager pipeline;

// Load configuration
QJsonObject config = JsonHandler::LoadFile("input/ml_pipeline_step1_generate.json");
pipeline.runSinglePipeline("input/ml_pipeline_step1_generate.json");
```

### 2. Batch Processing

```cpp
// Load batch configuration
QJsonObject batchConfig = JsonHandler::LoadFile("input/ml_pipeline_batch_config.json");
pipeline.setBatchConfig(batchConfig);

// Set pipeline steps
QStringList steps = {
    "input/ml_pipeline_step1_generate.json",
    "input/ml_pipeline_step2_analyze.json"
};
pipeline.setPipelineSteps(steps);

// Run batch pipeline
pipeline.runBatchPipeline();
```

## Command Line Usage

### File Analysis and Data Operations

```bash
# Comprehensive file analysis (new feature)
suprafit_cli -i experimental_data.json
# Shows: dimensions, sample data, configuration, output settings

# Generate data using mathematical equations (new feature)
suprafit_cli -i equation_config.json
# Creates data series like X, X², with automatic dependent variables

# Simple data conversion
suprafit_cli -i input_file.dat -o output_file.json

# Load and verify data without processing
suprafit_cli -i test_data.suprafit
```

### Generate Training Data

```bash
# Step 1: Generate simulated experimental data
suprafit_cli --config input/ml_pipeline_step1_generate.json

# Step 2: Analyze with multiple models
suprafit_cli --config input/ml_pipeline_step2_analyze.json

# Batch processing
suprafit_cli --batch-config input/ml_pipeline_batch_config.json
```

### Output Structure

The pipeline generates structured JSON files suitable for ML training:

```json
{
  "samples": [
    {
      "features": {
        "basic_statistics": {
          "SSE": 0.001234,
          "AIC": 123.45,
          "AICc": 125.67,
          "SEy": 0.045,
          "ChiSquared": 1.23,
          "RSquared": 0.987
        },
        "monte_carlo": {
          "shannon_entropy": 0.567,
          "std_deviation": 0.123,
          "model_rank": 2
        },
        "cross_validation": {
          "loo_entropy": 0.456,
          "lto_entropy": 0.478,
          "std_deviation": 0.098
        },
        "reduction_analysis": {
          "partial_std_dev": 0.234,
          "corrected_std_dev": 0.198,
          "stability_score": 0.876
        }
      },
      "labels": {
        "true_model_id": 1,
        "tested_model_id": 2,
        "is_correct_model": false,
        "model_quality_score": 0.345,
        "confidence_level": 0.234
      },
      "metadata": {
        "experiment_id": "exp_001",
        "noise_level": 0.001,
        "parameter_variation": "batch_01"
      }
    }
  ]
}
```

## Configuration Examples

### Mathematical Data Generation Configuration (New)

```json
{
  "rows": 10,
  "equations": "X|X*X",
  "dependent_columns": 2,
  "dependent_equations": "Y1|Y2"
}
```

**Result**: Creates systematic data:
- Independent: (1,1), (2,4), (3,9), (4,16), ...
- Dependent: Random values like (0.309, 0.369), (0.784, 0.622), ...

### Enhanced Data Generation Configuration

```json
{
  "Main": {
    "InFile": "tabelle.dat",
    "OutFile": "ml_training_data",
    "IndependentRows": 2,
    "Threads": 12,
    "Tasks": "GenerateDependent|GenerateNoisyDependent",
    "GenerateData": {
      "Series": 1,
      "Model": 1,
      "Repeat": 100,
      "Variance": 1e-3,
      "GlobalRandomLimits": "[1 15]",
      "LocalRandomLimits": "[0.1 2.0]"
    }
  },
  "Models": {
    "true_model": 1
  }
}
```

### DataOnly Configuration (Simple Load/Save)

```json
{
  "Main": {
    "InFile": "experimental_data.dat",
    "OutFile": "converted_data",
    "Tasks": "DataOnly"
  }
}
```

### Model Analysis Configuration

```json
{
  "Models": {
    "nmr_1_1": 1,
    "nmr_2_1": 2,
    "nmr_1_2": 3,
    "nmr_2_2": 4
  },
  "Jobs": {
    "monte_carlo": {
      "Method": 1,
      "MaxSteps": 1000,
      "VarianceSource": 2,
      "EntropyBins": 30
    },
    "cross_validation": {
      "Method": 4,
      "CXO": 1,
      "Algorithm": 2,
      "MaxSteps": 1000
    }
  }
}
```

## StatisticTool Integration

### Available Analysis Methods

1. **Monte Carlo Analysis**
   - Shannon entropy for parameter distributions
   - Standard deviation measures
   - Model-wise and parameter-wise rankings

2. **Cross-Validation Analysis**
   - Leave-One-Out (LOO)
   - Leave-Two-Out (LTO)
   - Leave-Many-Out (LMO)
   - Entropy-based parameter stability

3. **Reduction Analysis**
   - Partial standard deviation
   - Corrected standard deviation
   - Parameter stability under data reduction

4. **AIC Comparison**
   - Akaike Information Criterion
   - Corrected AIC (AICc)
   - Evidence ratios

### Extracting Features

```cpp
// Extract basic ML features
QJsonObject features = pipeline.extractMLFeatures(models);

// Extract StatisticTool features
QJsonObject statisticFeatures = pipeline.extractStatisticToolFeatures(models);

// Run specific analyses
QString mcAnalysis = pipeline.runMonteCarloAnalysis(models, true, 1);
QString cvAnalysis = pipeline.runCrossValidationAnalysis(models, 1, true, 3);
QString raAnalysis = pipeline.runReductionAnalysis(models, true, 0.1);
```

## Neural Network Training Data

### Feature Vector Structure

The pipeline generates comprehensive feature vectors for each model test:

```
Features (24 dimensions):
- Basic Statistics (6): SSE, AIC, AICc, SEy, ChiSquared, RSquared
- Monte Carlo (6): Entropy, StdDev, ModelRank, etc.
- Cross-Validation (6): LOO_Entropy, LTO_Entropy, LMO_Entropy, etc.
- Reduction Analysis (6): PartialStdDev, CorrectedStdDev, Stability, etc.

Labels (5 dimensions):
- true_model_id: Integer ID of the true model
- tested_model_id: Integer ID of the tested model
- is_correct_model: Boolean (true/false)
- model_quality_score: Float (0-1)
- confidence_level: Float (0-1)
```

### Training Pipeline

```python
# Example Python code for neural network training
import json
import numpy as np
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split

# Load generated data
with open('ml_training_dataset/ml_dataset_combined.json', 'r') as f:
    dataset = json.load(f)

# Extract features and labels
X = []
y = []

for batch in dataset['batches']:
    for sample in batch['samples']:
        features = sample['features']
        labels = sample['labels']
        
        # Flatten features into vector
        feature_vector = [
            features['basic_statistics']['SSE'],
            features['basic_statistics']['AIC'],
            # ... add all features
        ]
        
        X.append(feature_vector)
        y.append(labels['is_correct_model'])

# Train neural network
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2)
model = MLPClassifier(hidden_layer_sizes=(100, 50), max_iter=1000)
model.fit(X_train, y_train)

# Evaluate
accuracy = model.score(X_test, y_test)
print(f"Model accuracy: {accuracy:.3f}")
```

## Performance Considerations

### Batch Processing

- Use multiple worker threads for parallel processing
- Adjust batch size based on available memory
- Monitor CPU and memory usage during large batches

### Memory Management

- Process batches sequentially to avoid memory overflow
- Clean up temporary files after processing
- Use structured output format for efficient storage

### Scaling

- For large datasets (>10,000 samples), consider:
  - Database storage instead of JSON files
  - Distributed processing across multiple machines
  - Progressive loading of training data

## Recent Fixes and Troubleshooting

### Fixed Issues ✅

1. **Data Loading Problem (FIXED)**
   - **Issue**: Dependent data showed as zeros
   - **Cause**: Incorrect QModelIndex access in DataTable
   - **Fix**: Changed to direct matrix access `data(i, j)`
   - **Result**: Now correctly displays values like 0.309, 0.622, etc.

2. **Debug Output Clutter (FIXED)**
   - **Issue**: Excessive debug messages cluttering output
   - **Fix**: Cleaned up debug statements, added clean fmt::print formatting
   - **Result**: Professional, informative output

3. **File Analysis Missing (ADDED)**
   - **New Feature**: Comprehensive file analysis when no output specified
   - **Usage**: `suprafit_cli -i filename.json`
   - **Shows**: File info, dimensions, sample data, configuration

### Common Issues

1. **Data Not Displaying Correctly**
   - ✅ **Fixed**: Matrix access issue resolved
   - **Verify**: Check that values show correctly in analysis output
   - **Example**: Should see actual random values, not zeros

2. **Memory Errors**
   - Reduce batch size
   - Increase system memory
   - Use disk-based storage for large datasets

3. **Slow Processing**
   - Increase worker threads
   - Optimize StatisticTool parameters
   - Use faster storage (SSD)

4. **Invalid Results**
   - Check model convergence
   - Verify input data quality
   - Validate configuration parameters

### Debug Mode

Enable debug output for detailed logging:

```cpp
// Enable debug logging
QLoggingCategory::setFilterRules("*.debug=true");

// Run with verbose output
pipeline.runBatchPipeline();
```

## Working Examples (Verified 2025-07-18)

### File Analysis Example
```bash
$ suprafit_cli -i test_random_dependent_0.json

# Output:
================================================================================
📊 SUPRAFIT FILE ANALYSIS
================================================================================

📁 FILE INFORMATION:
   Input file: test_random_dependent_0.json
   File size: 3790 bytes
   Extension: json

🔢 INDEPENDENT DATA:
   Dimensions: 10 rows × 2 columns
   Headers: X, X*X
   Sample data (first 5 rows):
      Row 0: 1.000 1.000 
      Row 1: 2.000 4.000 
      Row 2: 3.000 9.000 

📈 DEPENDENT DATA:
   Dimensions: 10 rows × 2 columns  
   Headers: Y1, Y2
   Sample data (first 5 rows):
      Row 0: 0.309 0.369   # ✅ Now shows actual values!
      Row 1: 0.784 0.622 
      Row 2: 0.746 0.730 

📤 OUTPUT SETTINGS:
   Output file: test_output
```

### Mathematical Data Generation Example
```json
{
  "rows": 5,
  "equations": "X|X*X|X*X*X",
  "dependent_columns": 1
}
```

Generates:
- Independent: (1,1,1), (2,4,8), (3,9,27), (4,16,64), (5,25,125)
- Dependent: Random values for Y1

## Next Steps

1. **Extend Feature Set**: Add more sophisticated features from domain knowledge
2. **Optimize Performance**: Profile and optimize bottlenecks  
3. **Add Validation**: Implement cross-validation for ML models
4. **Create Benchmarks**: Establish performance baselines
5. **Documentation**: Add API documentation and tutorials

## Status: Data Loading Issues Resolved ✅

The pipeline now correctly:
- Loads and displays both independent and dependent data
- Shows actual numerical values instead of zeros
- Provides comprehensive file analysis
- Supports mathematical data generation
- Uses clean, professional output formatting