# SupraFit Development Guide

## Project Overview
SupraFit is a C++/Qt framework for supramolecular chemistry analysis with statistical post-processing and Machine Learning integration.

## Build System
- **Build**: Use `/build/debug` or `/build/release` for out-of-source builds
- **CMake**: `cmake .. && make -j4`
- **Tests**: `make run_tests`

## Development Standards

### Educational-First Design
- Clear, direct access to computational chemistry implementations
- Minimal abstraction layers that obscure scientific content
- Algorithm transparency with mathematical/physical implementations easily locatable
- Function/variable names reflecting scientific meaning

### Implementation Rules
- Mark new functions as "Claude Generated"
- Document functions (doxygen ready) with scientific context
- Use modern Qt6 patterns, avoid deprecated functions
- Comprehensive error handling and logging
- Debug output: `qDebug()` within `#ifdef DEBUG_ON #endif`
- Console output: Use `fmt`, avoid `std::cout`
- **Always check instructions blocks** in relevant CLAUDE.md files
- Update copyright year to current year: `Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>`

### Workflow States
- **ADD**: Features to be added
- **WIP**: Currently being worked on  
- **ADDED**: Basically implemented
- **TESTED**: Works (by operator feedback)
- **APPROVED**: Move to changelog, remove from CLAUDE.md

## Documentation

- [JSON Data Structure and I/O Functions](./JSON_datastruture.md)
- [Statistical Post-Processing Configuration](./src/capabilities/STATISTICAL_POSTPROCESSING.md)
- [Model ID Reference](./src/core/models/MODEL_ID_REFERENCE.md)
- [ML Integration Plan](./ml_integration_plan/README.md)
- [CLI Usage Examples](./src/client/usage_example.md)

## Key Components

### Core Libraries
- **libcore.a**: Core functionality (capabilities/, core/)
- **libmodels.a**: Data models and analysis models
- **suprafit_cli**: Command-line interface
- **suprafit**: GUI application

### Data Structure
- **DataClass**: Project container with metadata
- **DataTable**: Core data structure using Eigen::MatrixXd

### Key Functions
- **GenerateData()**: Creates training datasets
- **GenerateInputData()**: Equation-based data generation
- **AnalyzeFile()**: File analysis without task execution

## File Structure
```
src/
├── capabilities/    # DataGenerator, JobManager
├── client/         # CLI and ML pipeline
├── core/           # Core functionality
├── tests/          # Test suite
└── ui/             # GUI components
```

## Dependencies
- **Qt6**: Core, Test, Qml modules
- **Eigen**: Matrix operations
- **fmt**: Modern C++ formatting
- **ChaiScript**: Scripting support

## CLAUDE.md Organization
- Each `src/` subdirectory has detailed CLAUDE.md documentation
- **Keep entries concise and focused to save tokens**
- **Preserved Section**: Permanent documentation (don't edit)
- **Variable Section**: Short-term information (update regularly)
- **Instructions Block**: Operator-defined future tasks and visions