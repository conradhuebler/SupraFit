# SupraFit Development Guide

## Project Overview
SupraFit is a C++/Qt framework for supramolecular chemistry analysis. It provides a nice User Interface and statistical post-processing, like Monte Carlo simulation and Resampling Plans. It will now be extended wich Machine Learning functionals for improved Model Suggestion for complex supramolecular systems. 

## Build System
- **Main Build**: Use `/blob` or `/release` directories for out-of-source builds
- **CMake**: `cmake .. && make -j4`
- **Tests**: `make run_tests` or individual test executables

## Key Components

### Core Libraries
- **libcore.a**: Core functionality (capabilities/, core/)
- **libmodels.a**: Data models and analysis models
- **suprafit_cli**: Command-line interface for batch processing
- **suprafit**: GUI application

### Data Processing Pipeline
- **DataClass**: Project container with metadata and system parameters
- **DataTable**: Core data structure using Eigen::MatrixXd
- **DataGenerator**: Mathematical equation processing via QJSEngine
- **MLPipelineManager**: Machine learning pipeline coordination

### Client Applications
- **src/client/suprafit_cli.cpp**: CLI with data generation and analysis
- **src/client/ml_pipeline_manager.cpp**: ML pipeline management
- **src/ui/**: GUI components

## Testing

### Test Suite Structure
- **test_simple**: Basic functionality (4/4 tests pass)
- **test_datatable_simple**: DataTable basics (4/4 tests pass) 
- **test_datatable**: Comprehensive DataTable tests (18/25 tests pass)
- **test_dataclass**: DataClass functionality (crashes in one test)
- **test_pipeline**: ML pipeline tests (30/35 tests pass)

## Development Workflow

### Data Loading Pipeline
Critical fix implemented for data access:
```cpp
// CORRECT: Direct matrix access
double value = m_data->DependentModel()->data(i, j);

// INCORRECT: QModelIndex access (was causing zero values)
double value = m_data->DependentModel()->data(index).toDouble();
```

### JSON Configuration
Pipeline uses JSON configurations for:
- Data generation parameters
- Model selection and configuration
- Batch processing settings
- ML training parameters

### Key Functions
- **GenerateData()**: Creates training datasets using mathematical models
- **GenerateInputData()**: Equation-based data generation with DataGenerator
- **AnalyzeFile()**: Comprehensive file analysis without task execution
- **GenerateDataOnly()**: Simple load/save verification

## Common Commands

### Building
```bash
cd /home/conrad/src/SupraFit/release
cmake .. && make -j4
```

### Testing
```bash
# Run all tests
make run_tests

# Individual tests
./src/tests/test_simple
./src/tests/test_datatable
./src/tests/test_pipeline
```

### CLI Usage
```bash
# Traditional model-based generation
./bin/suprafit_cli --config input/test_ml_pipeline.json

# New DataGenerator-based generation (Claude Generated)
./bin/suprafit_cli --config input/test_datagenerator_pipeline.json

# Analyze file structure
./bin/suprafit_cli --analyze input/test_data.dat
```

### DataGenerator Configuration (Claude Generated)
```json
{
    "GenerateData": {
        "UseDataGenerator": true,
        "IndependentVariables": 2,
        "DataPoints": 25,
        "Equations": "X * A|X * B + C",
        "Repeat": 5,
        "RandomParameterLimits": {
            "A": {"min": 1.0, "max": 5.0},
            "B": {"min": 0.5, "max": 2.0},
            "C": {"min": -1.0, "max": 1.0}
        }
    }
}
```

## External Dependencies
- **Qt6**: Core, Test, Qml modules
- **Eigen**: Matrix operations (via libpeakpick)
- **fmt**: Modern C++ formatting
- **ChaiScript**: Scripting support
- **CxxThreadPool**: Parallel processing

## File Structure
```
src/
├── capabilities/    # Core capabilities (DataGenerator, JobManager, etc.)
├── client/         # CLI and ML pipeline management
├── core/           # Core functionality and file handlers
├── tests/          # Test suite
└── ui/             # GUI components

external/           # Third-party libraries
input/             # Sample configurations and test data
```

## Notes
- ML pipeline generates structured JSON output with SupraFit project format
- DataGenerator fully integrated into GenerateData() method for seamless operation

## DataGenerator Integration - COMPLETED ✅
**TASK 2 fully implemented by Claude Code AI Assistant:**

1. **Enhanced DataGenerator** (`src/capabilities/datagenerator.h/.cpp`):
   - `EvaluateWithRandomParameters()` - Random parameter injection via JavaScript
   - `generateRandomParameters()` - Static utility for configuration
   - `generateRandomValue()` - Seeded random generation
   - Comprehensive unit tests (9/9 pass ✅)

2. **SupraFitCli Integration** (`src/client/suprafit_cli.cpp`):
   - Automatic DataGenerator detection in `GenerateData()`
   - `GenerateDataWithDataGenerator()` - Enhanced pipeline
   - `validateDataGeneratorConfig()` - Configuration validation
   - Backward compatibility with traditional model approach

3. **Clean Code**:
   - Removed redundant/commented code from old implementation
   - Modern parameter generation replaces legacy approach
   - Professional error handling and logging

4. **Usage**: Set `"UseDataGenerator": true` in configuration to enable
   - Falls back gracefully to traditional approach if disabled
   - Supports both equation-based and model-based data generation

## Commands to Claude
- Document all you generated Source Code ✅ 
- Mark new implemented functions as Claude Generated ✅
- Add in every source code file you edit a note on what this file/methods/class do in the preambel sections ✅
- update copyright to include current year ✅
- Check suprafit_cli and datagenerator for TODOs and fix them ✅

## Current Status - Session 2025-01-24

### ✅ Completed Features

#### Modular JSON Structure (Independent/Dependent)
- **File**: `src/client/suprafit_cli.cpp`
- **Functions**: 
  - `GenerateDataWithModularStructure()` - Main orchestrator
  - `generateIndependentDataTable()` - Independent data generation 
  - `generateDependentDataTable()` - Dependent data generation
  - `loadDataTableFromFile()` - File loading with range selection
  - `applyNoise()` - Unified noise application (gaussian, exportMC, montecarlo)
- **Memory Management**: Fixed crashes using JSON DataTable transfer instead of direct DataClass copying
- **Testing**: ✅ Working with `input/test_file_loading_modular.json`

#### File Range Loading 
- **File**: `src/core/filehandler.h/.cpp` 
- **Functions**:
  - `getDataRange(startRow, endRow, startCol, endCol)` - Extract specific data ranges
  - Range parameters: `m_start_row`, `m_end_row`, `m_start_col`, `m_end_col`
- **Testing**: ✅ Loads Independent (10×2) and Dependent (8×4) data with correct offsets

#### DataGenerator Input Configuration Storage
- **File**: `src/capabilities/datagenerator.cpp`
- **Function**: `createEnhancedContent()` - Now saves original JSON input configuration in output
- **Implementation**: Appends full input JSON to generated content for traceability

#### Code Cleanup
- **Removed TODOs**: 
  - ✅ Line 1809 `suprafit_cli.cpp` - Obsolete file loading TODO (already implemented)
  - ✅ Line 511 `datagenerator.cpp` - Input JSON configuration now stored
- **Git Hash**: ✅ Already using proper `git_commit_hash` variable from version.h

### 🧪 Test Results
- **Compilation**: ✅ Success (warnings only, no errors)
- **DataGenerator Tests**: ✅ 9/9 passing
- **Modular Structure**: ✅ Successfully generates data from file ranges
- **File Loading**: ✅ Correctly extracts ranges: Independent (10×2), Dependent (8×4)  
- **Noise Application**: ✅ Gaussian noise applied (σ=0.02, seed=555)
- **JSON Output**: ✅ Valid SupraFit project format with git commit hash

### 📁 Sample Outputs
- `test_file_loading_data__0.json` - Modular structure with file loading
- Independent data: 10 rows × 2 cols from `/input/1_1_1_2_001.dat` (StartRow=0, StartCol=0)
- Dependent data: 8 rows × 4 cols from same file (StartRow=5, StartCol=2) with gaussian noise

### 🔧 Key Improvements
1. **Memory Safety**: JSON-based data transfer prevents pointer crashes
2. **Modularity**: Clean separation of Independent/Dependent data generation
3. **Traceability**: Input configurations stored in output for reproducibility
4. **Range Loading**: Flexible file loading with precise row/column selection
5. **Unified Noise**: All noise types (gaussian, exportMC, montecarlo) use same PrepareMC implementation