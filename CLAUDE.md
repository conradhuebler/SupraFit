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
- Each CLAUDE.md may contain an **instructions block** filled by the operator/programmer and from CLAUDE if approved with future tasks and visions that must be considered during code development
- Each CLAUDE.md file content should be important for ALL subdirectories
- If new knowledge is obtained from Claude Code Conversation preserve it in the CLAUDE.md files
- Always give improvments to existing code
## Key Components

### Core Libraries
- **libcore.a**: Core functionality (capabilities/, core/)
- **libmodels.a**: Data models and analysis models
- **suprafit_cli**: Command-line interface for batch processing
- **suprafit**: GUI application

### Data Processing Pipeline
- **DataClass**: Project container with metadata and system parameters
- **DataTable**: Core data structure using Eigen::MatrixXd

### Client Applications
- **src/client/suprafit_cli.cpp**: CLI with data generation and analysis
- **src/ui/**: GUI components


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

## Project Status

### ✅ Recent Achievements
- **Documentation**: Comprehensive CLAUDE.md files in all src/ subdirectories

### 🧪 Current Test Status

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
- Remove TODO Hashtags and text if done and approved
- Use modern Qt6 patterns and avoid deprecated functions
- Implement comprehensive error handling and logging 
- Debugging output with qDebug() within #ifdef DEBUG_ON #endif #TODO CLAUDE check if this is written correctly (CMakeLists.txt and include) 
- non-debugging console output is realised with fmt, port away from std::cout if appearing
- Maintain backward compatibility where possible
- **Always check and consider instructions blocks** in relevant CLAUDE.md files before implementing 
- reformulate and clarify task and vision entries if not alredy marked as CLAUDE formatted
- in case of compiler warning for deprecated suprafit functions, replace the old function call with the new one

### Add(ed)/Tested/Approved
- For Task/Features/Function use numeric identifieres (1,2,3,...) to organise the task/features/functions across the documents DURING development
### Workflow
- Features to be added have to be marked as ADD
- If the feature/function/task is being worked on, mark it as WIP
- If the feature/function/task is basically implemented, mark it as ADDED
- Summarize several functions to features/task
- If it works (by operator feedback), mark it as TESTED 
- Ask regulary if the TESTED feature is approved, if yes: it to the changelog (summarised) and remove from claude.md  

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