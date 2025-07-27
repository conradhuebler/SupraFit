# SupraFit Development Guide

## Project Overview
SupraFit is a C++/Qt framework for supramolecular chemistry analysis. It provides a nice User Interface and statistical post-processing, like Monte Carlo simulation and Resampling Plans. It will now be extended wich Machine Learning functionals for improved Model Suggestion for complex supramolecular systems. 

## Build System
- **Main Build**: Use `/build/debug` or `/build/release` directories for out-of-source builds
- **CMake**: `cmake .. && make -j4`
- **Tests**: `make run_tests` or individual test executables

## General instructions

- Each source code dir has a CLAUDE.md with basic informations of the code, the corresponding knowledge and logic in the directory 
- If a file is not present or outdated, create or update it
- Task corresponding to code have to be placed in the correct CLAUDE.md file
- Each CLAUDE.md may contain a variable part, where short-term information, bugs etc things are stored. Obsolete information have to be removed
- Each CLAUDE.md has a preserved part, which should no be edited by CLAUDE, only created if not present
- Each CLAUDE.md may contain an **instructions block** filled by the operator/programmer with future tasks and visions that must be considered during code development
- Each CLAUDE.md file content should be important for ALL subdirectories

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
cd /home/conrad/src/SupraFit/build/debug
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
# Modular NMR 1:1 titration generation
./bin/suprafit_cli --config input/NMR_1_1_Modular.json

# DataGenerator-based generation
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

## Project Status

### ✅ Recent Achievements
- **Modular Structure**: Complete Independent/Dependent JSON configuration system
- **NMR Integration**: Fully functional 1:1 titration model with realistic parameter generation
- **Memory Safety**: JSON-based data transfer prevents crashes
- **Code Cleanup**: Removed obsolete legacy functions and approaches
- **Documentation**: Comprehensive CLAUDE.md files in all src/ subdirectories

### 🧪 Current Test Status
- **DataGenerator**: 9/9 tests passing ✅
- **Basic Functionality**: All core tests passing ✅  
- **Build System**: Compiles without errors ✅
- **Integration**: CLI and modular workflows functional ✅

## Development Guidelines

### Code Organization
- Each `src/` subdirectory contains detailed CLAUDE.md documentation
- Variable sections updated regularly with short-term information
- Preserved sections contain permanent knowledge and patterns
- Instructions blocks contain operator-defined future tasks and visions

### Implementation Standards
- Mark new functions as "Claude Generated" for traceability
- Document new functions briefly (doxygen ready)
- Document existing undocumented functions if appearing regulary (briefly and doxygen ready)
- Remove TODO Hashtags and text done and approve
- Use modern Qt6 patterns and avoid deprecated functions
- Implement comprehensive error handling and logging 
- Debugging output with qDebug() within #ifdef DEBUG_ON #endif #TODO CLAUDE check if this is written correctly (CMakeLists.txt and include) 
- non-debugging console output is realised with fmt, port away from std::cout if appearing
- Maintain backward compatibility where possible
- **Always check and consider instructions blocks** in relevant CLAUDE.md files before implementing 
- reformulate and clarify task and vision entries if not alredy marked as CLAUDE formatted

### CLAUDE.md Structure Template
```markdown
# Directory Name - Brief Description

## [Preserved Section - Permanent Documentation]

## [Variable Section - Short-term Information]

## [Instructions Block - Operator-Defined Tasks]
### Future Tasks
- Task 1: Description
- Task 2: Description

### Vision
- Long-term goals and architectural directions
```