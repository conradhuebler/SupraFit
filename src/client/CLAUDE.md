# Client Applications - CLI and ML Pipeline

## Core Components
- **suprafit_cli.cpp/h**: Main CLI with JSON structure support and file analysis
- **ml_pipeline_manager.cpp/h**: ML pipeline coordination and evaluation
- **main.cpp**: Primary executable entry point
- **analyser.cpp/h**: File analysis and validation
- **simulator.cpp/h**: Data simulation utilities

## Key Functions (Claude Generated)
```cpp
// Modern modular data generation
QVector<QJsonObject> GenerateDataWithModularStructure();
QJsonObject generateIndependentDataTable(const QJsonObject& config);
QJsonObject generateDependentDataTable(const QJsonObject& config, const QJsonObject& indepData);

// File operations
QJsonObject loadDataTableFromFile(const QJsonObject& fileConfig);
QPointer<DataClass> applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent);
```

## Configuration Structure
```json
{
    "Main": {"OutFile": "output_name", "Repeat": 3},
    "Independent": {
        "Source": "generator",
        "Generator": {"Type": "equations", "DataPoints": 15, "Variables": 2, "Equations": "X|X*X"}
    },
    "Dependent": {
        "Source": "generator", 
        "Generator": {"Type": "model", "Series": 2, "Model": {"ID": 1}},
        "Noise": {"Type": "gaussian", "Std": [1e-3, 1e-3]}
    }
}
```

## Implementation Status

### ✅ Completed Features
- **ML Pipeline**: Complete workflow (data generation → model fitting → statistical evaluation)
- **Modular JSON Structure**: Independent/Dependent configuration system
- **File Operations**: Range loading, analysis, validation
- **Memory Management**: JSON-based data transfer with QPointer safety

### Key Functions
- `GenerateData()`: Main entry point for data generation
- `GenerateDataWithModularStructure()`: Modern modular approach
- `AnalyzeFile()`: Read-only file structure analysis

## Thread Management
- **CLI Parameters**: `-n/--nproc <N>` sets parallel threads (default: 4)
- **JSON Config**: `"Threads": N` in Main section (default: all cores)
- **ML Pipeline**: `"WorkerThreads": N` for batch processing (default: ideal thread count)

### Performance Guidance
- **Single-threaded**: Use `"Threads": 1` or `-n 1` for debugging
- **Light workloads**: Use `"Threads": 2-4` to avoid system overload
- **Heavy ML**: Separate `"WorkerThreads": 2` from main `"Threads": 4`

## Usage
```bash
# Generate data with modular structure
./bin/suprafit_cli -i input/NMR_1_1_Modular.json

# Control thread usage
./bin/suprafit_cli -n 2 -i input/NMR_1_1_Modular.json     # 2 threads
./bin/suprafit_cli --nproc 1 -i input/test.json           # Single-threaded

# Model analysis - display fit statistics with post-processing summary
./bin/suprafit_cli -l test_addmodels_v2-models-0.suprafit # Show model quality table

# Analyze file with detailed post-processing statistics (New - Claude Generated)
./bin/suprafit_cli --show-post-processing release/vonHand_mc.json # JSON-based analysis
./bin/suprafit_cli input/data_file.json                           # Standard analysis

# File analysis (read-only)  
./bin/suprafit_cli input/data_file.json
```

## Dependencies
- Qt6 (Core, Test), DataGenerator, DataClass/DataTable, FileHandler, fmt
---

## Variable Section

### Current Status - 2025-08-30
- **Architecture**: Clean separation (DataGenerator/JobManager)
- **ML Pipeline**: Operational data→fitting→evaluation workflow
- **File Extensions**: .suprafit default, .json optional
- **Model Testing**: Processes configured models correctly
- **Build**: All compilation errors resolved

### Recent Fixes
- **Data Loading Fix**: Fixed QModelIndex access in AnalyzeFile() (line 813) - dependent data now displays correctly
- **Statistical Analysis**: Moved from DataGenerator to JobManager architecture
- **Test Suite**: Resolved linker problems - all tests now build and run
- **File Extensions**: .suprafit default, clean filename generation without underscores
- **Qt6 Compatibility**: All compilation errors resolved

### Post-Processing Analysis Features (New - Claude Generated)
- **Enhanced Model Statistics Table**: Shows all 7 statistical analysis methods in compact table format
- **JSON-based Statistical API**: Complete migration from QString to structured QJsonObject results
- **Method Coverage**: MonteCarlo, WeakenedGridSearch, ModelComparison, CrossValidation, Reduction, FastConfidence, GlobalSearch
- **CLI Integration**: `--show-post-processing` flag for detailed method results and parameter analysis
- **Automatic Method Detection**: Post-processing blocks counted and displayed per model
- **Structured Output**: Consistent JSON format for all statistical methods with console-friendly display

---

## Instructions Block

### ML Pipeline Workflow
1. **Data Generation**: Creates simulated experimental data using DataGenerator
2. **Model Testing**: Tests multiple models against data with NonLinearFitThread integration
3. **Statistical Analysis**: Monte Carlo, Cross-validation via JobManager
4. **Result Output**: Structured JSON with model statistics and comparison metrics

### Test Results
- ✅ Simple ML tests process exactly configured models
- ✅ Parameter fitting with convergence detection working
- ✅ Statistical evaluation generates comparison metrics
- ✅ Performance: Small datasets ~1-2 seconds, Medium ~10-20 seconds

### Future Tasks
- **Client Refactoring**: Improve ExtractMLFeatures() with core JSON statistical API  
- **Dual CLI Modes**: Support pipeline and stepwise modes

### Vision
- Transition current ML pipeline to implemented infrastructure for AI data generation