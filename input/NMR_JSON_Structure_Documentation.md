# SupraFit JSON Configuration Structure

## Current Modular Structure (✅ Implemented)
```json
{
    "Main": {
        "OutFile": "output_name",
        "Repeat": 1,
        "UseModularStructure": true,
        "ProcessMLPipeline": true
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
        "nmr_1_1": {"ID": 1, "Options": {"FastMode": true}},
        "nmr_1_2": {"ID": 2, "Options": {"FastMode": true}}
    }
}
```

## Legacy Structure (Still Supported)
```json
{
    "Main": {
        "OutFile": "nmr_titration_data",
        "IndependentRows": 2,
        "GenerateData": {
            "UseDataGenerator": true,
            "IndependentVariables": 2,
            "DataPoints": 15,
            "Equations": "0.001|(X - 1) * 0.0001",
            "Series": 2,
            "Model": 1,
            "Repeat": 3,
            "GlobalRandomLimits": "[2 5]",
            "LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
        }
    }
}
```

## Key Parameters

### Modern Modular
- `UseModularStructure: true` - Enable Independent/Dependent separation
- `ProcessMLPipeline: true` - Activate ML workflow
- `AddModels` - Individual model configuration with options

### Legacy
- `GenerateData` - Single configuration block
- `IndependentRows` - Number of dependent columns
- `GlobalRandomLimits` / `LocalRandomLimits` - Matrix string format "[min max]"

## Data Generation Types
- **Equations**: Formula-based (e.g., "X", "0.001|(X-1)*0.0001")  
- **Model-based**: SupraFit model ID with parameter limits
- **File loading**: Range selection from existing data files

## Output
- `.suprafit` files (default) with metadata
- Clean filenames without underscores
- Enhanced content with parameter information