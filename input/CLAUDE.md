# Input Configurations - Test Cases and Examples

## Key Test Files

### Test_AddModels_v2.json ✅ Current
Modern ML pipeline with AddModels structure and PostFitAnalysis:
```json
{
    "Main": {"ProcessMLPipeline": true, "FitModels": true, "PostFitAnalysis": true},
    "AddModels": {
        "nmr_1_1": {"ID": 1, "Options": {"FastMode": true, "Convergency": 1e-7}},
        "nmr_1_2": {"ID": 2, "Options": {"FastMode": true}}
    },
    "PostFitAnalysis": {
        "methods": [
            {"Method": 1, "MaxSteps": 2000, "VarianceSource": 2},
            {"Method": 4, "CVType": 1, "MaxSteps": 100}
        ]
    }
}
```

### NMR_1_1_Modular.json ✅ Current  
Simple modular data generation:
```json
{
    "Independent": {
        "Generator": {"Type": "equations", "DataPoints": 15, "Equations": "0.001|(X - 1) * 0.0001"}
    },
    "Dependent": {
        "Generator": {"Type": "model", "Model": {"ID": 1}},
        "Noise": {"Type": "gaussian", "Std": [1e-3, 1e-3]}
    }
}
```

### Simple_ML_Test.json (Legacy)
Traditional ML pipeline with MLModels section (still supported)

## Configuration Parameters

### Main Section Controls
- `UseModularStructure: true` - Independent/Dependent separation
- `ProcessMLPipeline: true` - ML workflow activation
- `FitModels: true` - Parameter fitting control
- `PostFitAnalysis: true` - Statistical analysis after fitting

### Thread Configuration
```json
// Main section - General threading (default: all cores)
"Main": {
    "Threads": 4,          // Use 4 threads
    "ProcessMLPipeline": true
}

// Batch processing - ML pipeline workers
"BatchProcessing": {
    "WorkerThreads": 2     // ML pipeline worker threads
}
```

### AddModels vs MLModels
```json
// Modern (v2.0)
"AddModels": {
    "nmr_1_1": {"ID": 1, "Options": {"FastMode": true, "Convergency": 1e-7}}
}

// Legacy (v1.0) - still supported
"MLModels": {"nmr_1_1": 1, "nmr_1_2": 2}
```

### PostFitAnalysis Methods
- **Method 1**: Monte Carlo (VarianceSource: 2=SEy recommended)
- **Method 4**: Cross-Validation (CVType: 1=L0O, 2=L2O, 3=CXO)  
- **Method 5**: Parameter Reduction (cutoff threshold)
- **Method 3**: Model Comparison

## Usage
```bash
# Modern ML pipeline
./bin/suprafit_cli -i input/Test_AddModels_v2.json

# Simple data generation  
./bin/suprafit_cli -i input/data_generation/NMR_1_1_Modular.json

# File analysis with model statistics
./bin/suprafit_cli -l test_addmodels_v2-models-0.suprafit  # Show fit quality

# Thread control
./bin/suprafit_cli -n 2 -i input/Test_AddModels_v2.json  # Use 2 threads
./bin/suprafit_cli --nproc 1 -i input/test.json         # Single-threaded
```

## Output Files
- `{OutFile}-0.suprafit` - Generated dataset
- `{OutFile}-models-0.suprafit` - Fitted models with statistics
- Clean naming without underscores, .suprafit default extension

## Status - 2025-08-30
- ✅ ML Pipeline fully functional
- ✅ Modern AddModels structure implemented
- ✅ PostFitAnalysis integration completed
- ✅ Clean architecture with JobManager statistical analysis