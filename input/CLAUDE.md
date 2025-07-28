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

## Recent Additions (Session 2025-01-27/28)

### ✅ Completed
- **ML Pipeline Integration**: Full workflow from data generation to model evaluation
- **Configuration Control**: Precise model selection via MLModels section
- **Parameter Fitting**: NonLinearFitThread integration for optimization
- **Statistical Analysis**: SSE, convergence detection, model comparison metrics
- **Filename Improvements**: Clean naming without underscores (`-` instead of `__` and `_`)
- **Extension Support**: .suprafit default extension with .json optional

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