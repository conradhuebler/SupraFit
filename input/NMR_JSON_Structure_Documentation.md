# SupraFit JSON Configuration Structure Documentation

## Overview
This document explains the JSON structure for data generation in SupraFit, including both the new modular approach and legacy formats.

---

# 🚀 NEW MODULAR STRUCTURE (TODO - Implementation Plan)

## Design Goals
- **Clear Separation**: Independent and Dependent data generation are separate, explicit blocks
- **Flexible Input**: Support both file loading and generator-based creation
- **Unified Noise**: Optional noise/exportMC can be applied to either table
- **Model Integration**: Explicit choice between SupraFit models and equation generators
- **Parameter Control**: Clear random variable handling for equations

## New JSON Structure Plan

```json
{
    "Main": {
        "OutFile": "output_name",
        "Repeat": 1
    },
    "Independent": {
        "Source": "generator",  // "file" or "generator"
        "Generator": {
            "Type": "equations",    // "equations" or "model"
            "DataPoints": 20,
            "Variables": 1,
            "Equations": "X",
            "RandomParameters": {
                "X_start": {"min": 0, "max": 10},
                "X_step": {"min": 0.1, "max": 1.0}
            }
        },
        "File": {
            "Path": "input/data.dat",
            "StartRow": 0,
            "StartCol": 0,
            "Rows": 20,
            "Cols": 1
        },
        "Noise": {
            "Type": "exportMC",     // "gaussian", "exportMC", "none"
            "Std": [0.01],
            "RandomSeed": 12345
        }
    },
    "Dependent": {
        "Source": "generator",  // "file" or "generator"
        "Generator": {
            "Type": "equations",    // "equations", "model"
            "Series": 3,
            "Equations": "5|X*X|X*X + A",
            "RandomParameters": {
                "A": {"min": -0.5, "max": 0.5}
            },
            "Model": {
                "ID": 1,               // SupraFit model ID
                "GlobalLimits": {"K1": {"min": 2, "max": 5}},
                "LocalLimits": {
                    "delta1": {"min": 6.5, "max": 6.9},
                    "delta2": {"min": 6.0, "max": 6.4}
                }
            }
        },
        "File": {
            "Path": "input/dependent.dat",
            "StartRow": 0,
            "StartCol": 0,
            "Rows": 20,
            "Cols": 3
        },
        "Noise": {
            "Type": "exportMC",
            "Std": [0.0, 0.0, 0.1],  // Per-column noise
            "RandomSeed": 12345
        }
    }
}
```

## Implementation Plan

### Phase 1: JSON Parser Enhancement
1. **New Section Handlers**: Add `Independent` and `Dependent` section parsing
2. **Source Type Detection**: Handle `"Source": "file"` vs `"Source": "generator"`
3. **Generator Type Routing**: Route to equations vs model-based generation
4. **Parameter Format**: Support new JSON RandomParameters format

### Phase 2: Data Loading Logic
1. **File Loading**: Implement row/column range selection from files
2. **Generator Integration**: Use existing DataGenerator with new parameter format
3. **Model Integration**: Use existing model-based generation with cleaner interface

### Phase 3: Noise Application
1. **Per-Table Noise**: Apply noise independently to Independent/Dependent tables
2. **ExportMC Integration**: Support existing exportMC functionality
3. **Multi-Column Noise**: Different noise levels per column

### Phase 4: Legacy Compatibility
1. **Auto-Detection**: Detect old vs new format
2. **Migration Helper**: Convert old format to new structure internally
3. **Deprecation Warnings**: Inform users about old format

---

# 📚 LEGACY STRUCTURE (Current Implementation)

## Overview
This documents the current working JSON structure used for NMR titration data generation in SupraFit.

## Complete Working Example
```json
{
    "Main": {
        "OutFile": "nmr_1_1_titration_data",
        "IndependentRows": 2,
        "GenerateData": {
            "UseDataGenerator": true,
            "IndependentVariables": 2,
            "DataPoints": 15,
            "Equations": "0.001|(X - 1) * 0.0001",
            "Series": 2,
            "Model": 1,
            "Repeat": 3,
            "Variance": 1e-3,
            "GlobalRandomLimits": "[2 5]",
            "LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
        }
    }
}
```

## Structure Breakdown

### Main Section
- **OutFile**: Base name for generated output files
- **IndependentRows**: Number of dependent data columns (chemical shifts in NMR)
- **GenerateData**: Configuration object for data generation

### GenerateData Configuration

#### Core Parameters
- **UseDataGenerator**: `true` to enable DataGenerator-based generation
- **IndependentVariables**: Number of independent variable columns (usually 2 for NMR: concentration columns)
- **DataPoints**: Number of data points (rows) to generate
- **Series**: Number of dependent data series (should match IndependentRows)

#### Data Generation
- **Equations**: Pipe-separated equations for independent variables
  - Format: `"equation1|equation2|..."`
  - Example: `"0.001|(X - 1) * 0.0001"` generates two concentration columns
  - `X` represents the row index/data point number

#### Model-Based Generation (NMR Specific)
- **Model**: Model ID for dependent data generation (1 = NMR 1:1 binding model)
- **Repeat**: Number of datasets to generate with different random parameters

#### Randomization Parameters
- **GlobalRandomLimits**: Stability constants (log K values) as matrix string
  - Format: `"[min max]"` for single parameter
  - Example: `"[2 5]"` means log K between 2 and 5
- **LocalRandomLimits**: Chemical shift parameters as matrix string
  - Format: `"[min1 max1; min2 max2; ...]"` for multiple parameters
  - Example: `"[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"` for 4 chemical shift parameters
- **Variance**: Noise level for generated data (optional)

## Data Flow

### Step 1: Independent Data Generation
1. DataGenerator uses "Equations" to create independent variable columns
2. Example: `"0.001|(X - 1) * 0.0001"` creates:
   - Column 1: constant 0.001 for all rows
   - Column 2: (row_index - 1) * 0.0001

### Step 2: Dependent Data Generation
1. Uses the specified "Model" (NMR binding model)
2. Applies "GlobalRandomLimits" to stability constants
3. Applies "LocalRandomLimits" to chemical shift parameters
4. Generates "Series" number of dependent columns (chemical shifts)

### Step 3: Output
- Creates "Repeat" number of datasets
- Each dataset has different random parameter values
- Saves as .suprafit files with enhanced content including parameter values

## Key Differences from DataGenerator-Only Approach

### Traditional DataGenerator (equation-based)
```json
"GenerateData": {
    "UseDataGenerator": true,
    "Equations": "X",
    "DependentEquations": "5|X*X|X*X + A",
    "RandomParameterLimits": {
        "A": {"min": -0.5, "max": 0.5}
    }
}
```

### NMR Model-Based Generation
```json
"GenerateData": {
    "UseDataGenerator": true,
    "Equations": "0.001|(X - 1) * 0.0001",
    "Model": 1,
    "GlobalRandomLimits": "[2 5]",
    "LocalRandomLimits": "[6.5 6.9; 6.0 6.4; 2.3 2.6; 2.2 2.5]"
}
```

## Implementation Notes

1. **Matrix String Format**: GlobalRandomLimits and LocalRandomLimits use MATLAB-style matrix strings
2. **Model Integration**: When "Model" is specified, the system uses model-based dependent data generation
3. **Enhanced Content**: Generated files include detailed parameter information in project content
4. **Random Seed Management**: Global RNG ensures different parameters per iteration

## Supported Models
- **Model 1**: NMR 1:1 binding model with stability constants and chemical shift parameters

## Usage Examples

### Simple Linear/Quadratic Generation
For pure equation-based generation without models:
```json
{
    "Main": {
        "OutFile": "linear_quadratic_data",
        "IndependentRows": 3,
        "GenerateData": {
            "UseDataGenerator": true,
            "IndependentVariables": 1,
            "DataPoints": 20,
            "Equations": "X",
            "DependentEquations": "5|X*X|X*X + A",
            "Series": 3,
            "Repeat": 1,
            "RandomParameterLimits": {
                "A": {"min": -0.5, "max": 0.5}
            }
        }
    }
}
```

### NMR Titration Generation
For model-based NMR data generation:
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