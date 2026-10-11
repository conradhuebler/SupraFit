# SupraFit JSON Format Specification v2.0

## Overview

SupraFit uses a JSON-based file format to store project data, analytical models, and statistical results. This specification defines the complete data structure for both single and multi-project files.

## File Extensions

- **`.suprafit`**: Compressed JSON format (default, recommended)
- **`.json`**: Plain text JSON format (human-readable)

## Core Structure Types

### Single-Project Format

A single-project file contains one analytical experiment or analysis:

```json
{
  "data": {
    "DataType": 1,
    "SupraFit": 2024,
    "title": "Project Title",
    "uuid": "unique-project-identifier",
    "independent": {...},
    "dependent": {...},
    "system": {...},
    "raw": {},
    "begin_data": 0,
    "end_data": 25,
    "timestamp": 1706123456789
  },
  "model_0": {
    "model": 1,
    "name": "1:1 Model",
    "data": {
      "globalParameter": {...},
      "localParameter": {...},
      "methods": {...}
    }
  },
  "model_1": {...}
}
```

### Multi-Project Format

A multi-project file contains multiple independent analyses:

```json
{
  "project_0": {
    "data": {...},
    "model_0": {...},
    "model_1": {...}
  },
  "project_1": {
    "data": {...},
    "model_0": {...}
  }
}
```

## DataClass Schema (data object)

### Core Metadata
- **`DataType`** (integer): Data type identifier (1 = Table)
- **`SupraFit`** (integer): File format version number
- **`title`** (string): Human-readable project title
- **`uuid`** (string): Unique project identifier
- **`timestamp`** (integer): Creation timestamp in milliseconds
- **`git_commit`** (string): Git commit hash of SupraFit version
- **`content`** (string): Enhanced content description
- **`root_dir`** (string): Root directory path

### Data Tables
- **`independent`** (object): Independent variable data table
  - `header` (array): Column headers
  - `data` (object): Row-wise data storage
  - `checked` (object): Data point validation flags
- **`dependent`** (object): Dependent variable data table (same structure)
- **`independent_raw`** (object): Raw independent data (unprocessed)
- **`dependent_raw`** (object): Raw dependent data (unprocessed)

### Configuration
- **`system`** (object): System parameters (key-value pairs)
- **`raw`** (object): Additional raw data storage
- **`begin_data`** (integer): Start index for data analysis
- **`end_data`** (integer): End index for data analysis
- **`xaxis`** (integer): X-axis column selection
- **`simulate_dependent`** (integer): Number of dependent series to simulate

## Model Schema (model_X objects)

### Model Identification
- **`model`** (integer): Model type ID (see MODEL_ID_REFERENCE.md)
- **`name`** (string): Human-readable model name

### Model Data
- **`data`** (object): Core model information
  - **`globalParameter`** (object): Global fitting parameters (e.g., stability constants)
  - **`localParameter`** (object): Local fitting parameters (e.g., chemical shifts)
  - **`active_series`** (string): Active data series configuration
  - **`methods`** (object): Statistical analysis results

### Statistical Analysis (`methods` object)
Statistical post-processing results organized by analysis method:

```json
"methods": {
  "1": {  // Monte Carlo
    "controller": {...},
    "parameter_0": {"data": {...}, "boxplot": {...}, "confidence": {...}}
  },
  "2": {  // Cross Validation
    "controller": {...},
    "validation_results": {...}
  },
  "3": {  // Reduction Analysis
    "controller": {...},
    "reduction_results": {...}
  }
}
```

### UI Configuration (optional)
- **`colors`** (array): Chart color configuration
- **`keys`** (array): UI state information

## Legacy Compatibility

### Backward Compatibility
- **`LegacyImportData()`**: Supports older data formats
- **`LegacyImportModel()`**: Supports older model formats
- **Version checking**: Prevents loading of newer formats in older SupraFit versions

### Model Configuration Keys
- **`AddModels`**: Current standard for model specification
- **`MLModels`**: Legacy key, automatically converted to AddModels format
- **`ProcessMLPipeline`**: Enables ML pipeline workflow

## Data Generation Configuration

### Modern Modular Structure
```json
{
  "Main": {
    "OutFile": "output_name",
    "UseModularStructure": true,
    "ProcessMLPipeline": true
  },
  "Independent": {
    "Source": "generator",
    "Generator": {
      "Type": "equations",
      "DataPoints": 20,
      "Equations": "0.001|(X - 1) * 0.0001"
    }
  },
  "Dependent": {
    "Source": "generator",
    "Generator": {
      "Type": "model",
      "Model": {"ID": 1}
    },
    "Noise": {
      "Type": "gaussian",
      "Std": [1e-3, 1e-3]
    }
  }
}
```

### Legacy Format (deprecated)
```json
{
  "Main": {
    "OutFile": "output_name",
    "GenerateData": {
      "UseDataGenerator": true,
      "DataPoints": 15,
      "Equations": "0.001|(X - 1) * 0.0001",
      "Model": 1
    }
  }
}
```

## Validation Rules

### Required Fields
- **Single-project**: Must contain `data` object
- **Multi-project**: Must contain `project_X` objects
- **Data object**: Must contain `DataType`, `SupraFit`, `independent`, `dependent`
- **Model objects**: Must contain `model`, `name`, `data`

### Version Compatibility
- Files with `SupraFit` version > current version are rejected
- Warning displayed for significantly older versions
- Legacy import functions handle format differences

### Data Integrity
- Independent and dependent data tables must have consistent dimensions
- Model parameters must match model type requirements
- UUID must be unique within multi-project files

## Implementation Notes

### JSON Structure Access
- Use `JsonHandler::LoadFile()` for .suprafit files
- Use `QJsonDocument::fromJson()` for .json files
- Access data via `DataClass::ImportData()` and `AbstractModel::ImportModel()`
- Export via `DataClass::ExportData()` and `AbstractModel::ExportModel()`

### Performance Considerations
- Large model result datasets are stored in compressed .suprafit format
- Statistical analysis results can be memory-intensive
- Raw data preservation is optional and configurable

This specification defines the complete SupraFit JSON format as implemented in SupraFit v2024+.