# Input Configurations - Test Cases and Examples

## Overview
Collection of JSON configuration files for testing SupraFit functionality, including modular data generation, ML pipeline integration, and file loading workflows.

## ML Pipeline Configurations (Claude Generated) 🔥

### Simple_ML_Test.json
**Purpose**: Test ML Pipeline with 3-model comparison
```json
{
    "Main": {"OutFile": "simple_ml_test", "Repeat": 1, "FitModels": true},
    "MLModels": {"nmr_1_1": 1, "nmr_1_2": 2, "nmr_2_1": 3},
    "UseModularStructure": true,
    "ProcessMLPipeline": true,
    "Independent": {
        "Source": "generator",
        "Generator": {
            "Type": "equations",
            "DataPoints": 8,
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
            "GlobalRandomLimits": "[3 4]",
            "LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
        },
        "Noise": {"Type": "gaussian", "Std": [1e-3, 1e-3], "RandomSeed": 123}
    }
}
```

**Workflow**: 
1. Generate simulated NMR 1:1 data using DataGenerator
2. Test 3 models against the data: NMR 1:1, NMR 1:2, NMR 2:1  
3. Fit parameters using NonLinearFitThread
4. Generate statistical comparison (SSE, convergence, etc.)
5. Output: Clean filenames with .suprafit extension

**Usage**: `./bin/suprafit_cli -i ../../input/Simple_ML_Test.json`

**Generated Files**: 
- `simple_ml_test-0.suprafit` - Dataset with simulated NMR data
- `simple_ml_test-models-0.suprafit` - Fitted models with statistical evaluation

## Legacy Configurations

### test_ml_pipeline.json
Traditional ML pipeline configuration (pre-modular structure)

### test_datagenerator_pipeline.json  
DataGenerator-focused testing (equation-based generation)

### test_file_loading_modular.json
File range loading with modular Independent/Dependent structure

## Configuration Patterns

### ProcessMLPipeline Flag
- `"ProcessMLPipeline": true` - Activates enhanced ML workflow
- Automatically detected by CLI configuration type detection
- Routes to `ProcessMLPipeline()` method in SupraFitCli

### MLModels Section
Specifies which models to test:
```json
"MLModels": {
    "model_name": model_id,
    "nmr_1_1": 1,    // NMR 1:1
    "nmr_1_2": 2,    // NMR 1:2  
    "nmr_2_1": 3     // NMR 2:1
}
```

### Modular Structure
- `"UseModularStructure": true` - Enables Independent/Dependent separation
- `"Independent"` - X-axis data configuration
- `"Dependent"` - Y-axis data configuration with optional noise

## Generated Outputs

### Dataset Files
- `{OutFile}-0.suprafit` - Generated dataset in SupraFit format (was `{OutFile}__0.json`)
- Contains both independent and dependent data with metadata
- Default .suprafit extension, .json optional

### Model Files  
- `{OutFile}-models-0.suprafit` - Fitted models with statistical evaluation (was `{OutFile}_models_0.json`)
- Contains `model_0`, `model_1`, `model_2` sections with parameters and metrics
- Clean naming without underscores

## Testing Workflow

1. **Configuration**: Create JSON with ProcessMLPipeline and MLModels
2. **Execution**: `./bin/suprafit_cli -i config.json`
3. **Verification**: Check output shows "Using configured MLModels: N models"
4. **Analysis**: Review generated model files for fitted parameters and statistics

## Enhanced JSON Structure v2.0 (Session 2025-01-28) 🚀

### New Unified Configuration Structure
**Purpose**: Comprehensive ML Pipeline with integrated Post-Fit Analysis

```json
{
    "Main": {
        "OutFile": "NMR_1_1_With_Analysis",
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
            "DataPoints": 25,
            "Variables": 2,
            "Equations": "0.001|(X - 1) * 0.0001"
        }
    },
    "Dependent": {
        "Source": "generator", 
        "Generator": {
            "Type": "model",
            "Series": 4,
            "Model": {"ID": 1},
            "GlobalRandomLimits": "[2 5]",
            "LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
        },
        "Noise": {
            "Type": "gaussian",
            "Std": [1e-3, 1e-3, 1e-3, 1e-3],
            "RandomSeed": 12345
        }
    },
    "AddModels": {
        "nmr_1_1": {
            "ID": 1,
            "Options": {
                "FastMode": true,
                "Convergency": 1e-7,
                "MaxIterations": 1000
            },
            "PostFitAnalysis": {
                "enabled": true,
                "methods": [
                    {
                        "Method": 1,
                        "MaxSteps": 3000,
                        "VarianceSource": 2
                    }
                ]
            }
        },
        "nmr_1_2": {
            "ID": 2,
            "Options": {
                "FastMode": true
            }
        }
    },
    "PostFitAnalysis": {
        "enabled": true,
        "methods": [
            {
                "Method": 1,
                "MaxSteps": 5000,
                "VarianceSource": 2,
                "EntropyBins": 50
            },
            {
                "Method": 4,
                "CVType": 1,
                "X": 3,
                "MaxSteps": 500,
                "EntropyBins": 30
            },
            {
                "Method": 5,
                "MaxSteps": 50,
                "cutoff": 0.15
            }
        ]
    }
}
```

### Key Structure Changes (v2.0)

#### **Main Section - Central Control**
- `"UseModularStructure": true` - Enables Independent/Dependent separation
- `"ProcessMLPipeline": true` - Activates enhanced ML workflow  
- `"FitModels": true` - Controls whether to perform parameter fitting
- `"PostFitAnalysis": true` - Controls whether to run statistical analysis after fitting

#### **AddModels Section (replaces MLModels)**
- **Enhanced Model Configuration**: Each model can have individual options and analysis settings
- **Model Options**: FastMode, Convergency, MaxIterations, etc.
- **Individual PostFitAnalysis**: Model-specific analysis that overrides global settings
- **Flexible Structure**: `"model_name": {"ID": model_id, "Options": {...}, "PostFitAnalysis": {...}}`

#### **PostFitAnalysis Section - Global Analysis Settings**
- **Statistical Methods**: Monte Carlo (1), Cross-Validation (4), Reduction (5), Model Comparison (3)
- **Method Configuration**: Each method has specific parameters
- **VarianceSource**: 2 = SEy (model error), 1 = manual variance (default for MC)
- **Inheritance**: Models without individual PostFitAnalysis use these global settings

### Monte Carlo Variance Configuration
```json
{
    "Method": 1,
    "MaxSteps": 10000,
    "VarianceSource": 2,     // 2 = Use model SEy, 1 = manual variance
    "Variance": 0.05,        // Only used if VarianceSource = 1
    "EntropyBins": 50
}
```

**VarianceSource Options:**
- `1` - Manual variance (uses `"Variance"` value)
- `2` - **SEy (Standard Error of y) - Default recommended**
- `3` - Other model-based variance sources

### Cross-Validation Configuration
```json
{
    "Method": 4,
    "CVType": 1,             // 1=L0O, 2=L2O, 3=CXO
    "X": 3,                  // For CXO only
    "MaxSteps": 1000,
    "EntropyBins": 50
}
```

### Parameter Reduction Configuration
```json
{
    "Method": 5,
    "MaxSteps": 100,
    "cutoff": 0.1            // Significance threshold
}
```

### Model Comparison Configuration
```json
{
    "Method": 3,
    "MaxSteps": 1000,
    "confidence": 95,
    "ParameterIndex": 0      // 0=SSE, 1=SEy, 2=ChiSquared, 3=sigma
}
```

## Legacy Configurations (v1.0)

### Simple_ML_Test.json
Traditional ML pipeline configuration with MLModels section (still supported for backwards compatibility)

## Recent Additions (Session 2025-01-27/28)

### ✅ Completed  
- **ML Pipeline Integration**: Full workflow from data generation to model evaluation
- **Configuration Control**: Precise model selection via MLModels section (v1.0)
- **Parameter Fitting**: NonLinearFitThread integration for optimization
- **Statistical Analysis**: SSE, convergence detection, model comparison metrics
- **Filename Improvements**: Clean naming without underscores (`-` instead of `__` and `_`)
- **Extension Support**: .suprafit default extension with .json optional

### ✅ New in v2.0 (Session 2025-01-28)
- **Unified Configuration Structure**: Main section controls all pipeline aspects
- **Enhanced Model Configuration**: AddModels with individual options and analysis settings  
- **Post-Fit Analysis Integration**: Comprehensive statistical analysis after model fitting
- **JSON-based Analysis API**: Modern statistical methods with percentile-based confidence intervals
- **JobManager Integration**: Uses SupraFit's method IDs for statistical analysis execution
- **SEy Default Variance**: Recommended VarianceSource=2 for Monte Carlo analysis

### 🔧 Key Fixes
- **Qt6 Compatibility**: Removed `QJsonValue::isInt()` calls
- **Configuration Reading**: Fixed m_original_config storage for proper model selection
- **CLI Routing**: ProcessMLPipeline flag detection and MLPipelineConfig routing
- **File Naming**: Replaced underscores with hyphens for cleaner output files
- **Extension Logic**: Automatic .suprafit default with .json detection in setOutFile()

### 📊 Test Results
- ✅ Simple_ML_Test.json processes exactly 3 configured models
- ✅ Model fitting with convergence detection working
- ✅ Statistical evaluation and comparison metrics generated
- ✅ Clean filename output: `simple_ml_test-0.suprafit`, `simple_ml_test-models-0.suprafit`
- ✅ Default .suprafit extension working correctly

---

## Variable Section (Updated 2025-01-27)

### Current Focus
ML Pipeline development and integration completed. System can now:
- Generate simulated datasets using DataGenerator
- Test multiple models against data for statistical comparison  
- Perform parameter optimization using NonLinearFitThread
- Generate ML training datasets with model evaluation metrics
- Output clean filenames with .suprafit extension by default

### Known Issues
None currently identified. ML Pipeline workflow fully functional with clean file naming.

### Testing Status
All ML Pipeline configurations working correctly with proper model selection and clean file output.

---

## Vision & Future Development

### **Re-Analysis and Monte Carlo Data Merging** 🔄
**Comprehensive statistical analysis updates for existing fitted models**

**Core Capabilities:**
1. **Re-Analysis of Fitted Models**: Update statistical analysis for previously fitted models without re-fitting
   - Load existing .suprafit files with fitted model parameters
   - Apply new statistical methods (CV, Reduction, enhanced MC) to existing results
   - Update entropy calculations with different bin sizes or new algorithms
   - Recalculate confidence intervals using improved percentile methods
   - Preserve original fit results while adding enhanced statistical analysis

2. **Monte Carlo Data Merging**: Combine MC simulations with identical parameters for enhanced statistical power
   - Merge raw MC data from multiple simulation runs with same model parameters and variance settings
   - Incrementally extend MC simulations (add more steps to existing runs)
   - Combine distributed MC calculations performed on different machines
   - Recalculate percentiles, entropy, and confidence intervals with larger datasets
   - Maintain traceability of merged simulation components

**Implementation Strategy:**
```json
{
    "Main": {
        "Mode": "ReAnalysis",
        "InputFiles": ["existing_model_1.suprafit", "mc_extension_data.suprafit"],
        "PostFitAnalysis": true
    },
    "PostFitAnalysis": {
        "UpdateExisting": true,
        "MergeCompatibleMC": true,
        "methods": [
            {
                "Method": 1,
                "AdditionalSteps": 5000,
                "MergeExistingData": true,
                "UpdateEntropyBins": 100,
                "RecalculateConfidenceIntervals": true
            }
        ]
    }
}
```

**Benefits:**
- Leverage existing computational work without re-fitting
- Improve statistical reliability through data aggregation
- Enable research continuity and iterative analysis refinement
- Support cost-effective high-precision statistical analysis
- Facilitate collaborative and distributed computational workflows