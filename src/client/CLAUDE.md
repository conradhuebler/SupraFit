# Client Applications - CLI and ML Pipeline

## Core Components
- **suprafit_cli.cpp/h**: Main CLI — JSON config parsing, data generation, model fitting, file analysis, parameter extraction (large god-class; decomposition tracked as D3 in root `TECHNICAL_DEBT.md`)
- **data_factory.cpp/h**: `DataFactory` — stateless static data-table builders extracted from SupraFitCli (D3): validate/generateIndependent/loadFromFile/applyNoise; the `GenerateData*` orchestrators delegate here
- **ml_pipeline_manager.cpp/h**: ML pipeline coordination and evaluation
- **main.cpp**: Primary executable entry point; parses args directly via `QCommandLineParser` and dispatches
- **ml_cli_main.cpp**: Separate `ml_cli_main` target (neural-network tutorials, built with `ML_NEURAL_NETWORKS`)

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

## Thread Management
- **CLI Parameters**: `-n/--nproc <N>` sets parallel threads (default: 4)
- **JSON Config**: `"Threads": N` in Main section (default: all cores)
- **ML Pipeline**: `"WorkerThreads": N` for batch processing (default: ideal thread count)

## Usage
```bash
# Generate data with modular structure
./bin/suprafit_cli -i input/NMR_1_1_Modular.json

# Control thread usage
./bin/suprafit_cli -n 2 -i input/NMR_1_1_Modular.json      # 2 threads
./bin/suprafit_cli --nproc 1 -i input/test.json            # single-threaded

# Model analysis - display fit statistics with post-processing summary
./bin/suprafit_cli -l project-models-0.suprafit

# Extract fitted model parameters
./bin/suprafit_cli -x project-models-0.suprafit            # all models
./bin/suprafit_cli -x --extract-model 2 models.suprafit    # specific model

# Analyze file with detailed post-processing statistics
./bin/suprafit_cli --show-post-processing release/vonHand_mc.json
```

## Dependencies
- Qt6 (Core, Test), DataGenerator, DataClass/DataTable, FileHandler, fmt

---

## Variable Section

### Status - 2026-07-02
- **Architecture**: Clean DataGenerator/JobManager separation; ProjectManager used for project I/O
- **Entry point**: `main.cpp` dispatches directly via `QCommandLineParser`
- **Command-pattern layer removed**: `cli_command_parser` / `cli_command_dispatcher` / `main_refactored.cpp` were compiled-but-never-wired dead code and have been deleted (history in `REFACTORING_CLI.md`)
- **File Extensions**: `.suprafit` default, `.json` optional
- **Parameter Extraction**: `-x/--extract-parameters` and `--extract-model N`

### Known debt (see root `TECHNICAL_DEBT.md`, CLI section)
- `SupraFitCli` is a ~3.7k-line god-class mixing ~8 responsibilities → D3 decomposition
- 12 remaining `MIGRATION POINT` markers still read legacy `m_toplevel`/`m_data` instead of ProjectManager (blocked on missing ProjectManager APIs → D2)
- ✅ `ModelStatistics` + `extractModelStatistics` de-duplicated (D4): core `AnalysisManager::extractModelStatistics` is the single authoritative extractor; `AnalysisReporter` renders only
- `PerformeJobs()` runs statistical jobs but discards their results

---

## Instructions Block

### ML Pipeline Workflow
1. **Data Generation**: Creates simulated experimental data using DataGenerator
2. **Model Testing**: Tests multiple models against data with NonLinearFitThread integration
3. **Statistical Analysis**: Monte Carlo, Cross-validation via JobManager
4. **Result Output**: Structured JSON with model statistics and comparison metrics

### Vision
- **Phase 3 (open)**: Decompose `SupraFitCli` into focused classes (e.g. DataFactory, TaskRunner, AnalysisReporter, MlPipeline, MlExport) — see `REFACTORING_CLI.md` Phases 3–5.
- Unify CLI `PerformeJobs()` with the GUI job-execution path (planned TaskController).
