# Reference Implementation Flow - Conrad's Original Statistical System

## Overview

This document maps the complete data flow through Conrad's original statistical analysis system, showing how JSON statistical data moves from JobManager through AbstractSearchClass to AbstractModel and finally to ResultsDialog display.

**Critical Principle**: This flow is the **reference implementation** that all statistical functions must follow for compatibility with SupraFit's integrated system.

## Complete System Flow

### Phase 1: Job Initiation (JobManager)

**Location**: `src/capabilities/jobmanager.cpp`

```cpp
// 1. JobManager receives statistical analysis job
QJsonObject JobManager::RunMonteCarlo(const QJsonObject& job)
{
    // 2. Merge job parameters with standard config block
    QJsonObject block = QJsonObject(MonteCarloConfigBlock);
    for (const QString& key : job.keys())
        block[key] = job[key];

    // 3. Configure AbstractSearchClass handler
    m_montecarlo_handler->setController(block);
    m_montecarlo_handler->setModel(m_model);

    // 4. Execute statistical analysis
    m_montecarlo_handler->Run();

    // 5. Get result with numeric key structure
    QJsonObject result = m_montecarlo_handler->Result();

    return result;
}
```

**Key JobManager Methods:**
- `RunMonteCarlo(job)` → MonteCarloStatistics
- `RunGridSearch(job)` → WeakenedGridSearch
- `RunResample(job)` → ResampleAnalyse
- `RunModelComparison(job)` → ModelComparison
- `RunGlobalSearch(job)` → GlobalSearch

### Phase 2: Statistical Analysis (AbstractSearchClass)

**Location**: `src/capabilities/abstractsearchclass.cpp`

```cpp
// 1. AbstractSearchClass executes analysis (implemented in subclasses)
void AbstractSearchClass::Run() {
    // Perform statistical calculations
    // Store results in m_results vector
}

// 2. Generate result with Conrad's numeric key structure
QJsonObject AbstractSearchClass::Result() const
{
    QJsonObject result;

    // CRITICAL: Creates numeric key structure "0", "1", "2"
    for (int i = 0; i < m_results.size(); ++i)
        result[QString::number(i)] = m_results[i];

    // Add controller configuration from JobManager
    QJsonObject controller = Controller();
    controller["title"] = m_model->Name();
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    result["controller"] = controller;
    return result;
}
```

**AbstractSearchClass Implementations:**
- **MonteCarloStatistics**: Parameter uncertainty analysis
- **WeakenedGridSearch**: Grid-based parameter exploration
- **ResampleAnalyse**: Cross-validation and reduction analysis
- **ModelComparison**: Multi-model statistical comparison
- **GlobalSearch**: Global parameter optimization

### Phase 3: Result Storage (AbstractModel)

**Location**: `src/core/models/AbstractModel.cpp`

```cpp
// 1. JobManager stores result in model
int index = m_model->UpdateStatistic(result);

// 2. AbstractModel::UpdateStatistic stores in method-specific vectors
int AbstractModel::UpdateStatistic(const QJsonObject& object)
{
    // Parse method type from controller
    QJsonObject controller = object["controller"].toObject();
    SupraFit::Method method = (SupraFit::Method)controller["Method"].toInt();

    // Store in appropriate vector based on method
    switch (method) {
    case SupraFit::Method::MonteCarlo:
        m_mc_statistics.append(object);
        return m_mc_statistics.size() - 1;
    case SupraFit::Method::WeakenedGridSearch:
        m_wg_statistics.append(object);
        return m_wg_statistics.size() - 1;
    // ... other methods
    }
}

// 3. Retrieval method for GUI access
QJsonObject AbstractModel::getStatistic(SupraFit::Method type, int index) const
{
    switch (type) {
    case SupraFit::Method::MonteCarlo:
        if (index < m_mc_statistics.size())
            return m_mc_statistics[index];
        break;
    // ... other methods
    }
    return QJsonObject();
}
```

**AbstractModel Storage Vectors:**
- `m_mc_statistics` → Monte Carlo results
- `m_wg_statistics` → Weakened Grid Search results
- `m_cv_statistics` → Cross-Validation results
- `m_moco_statistics` → Model Comparison results
- `m_reduction_statistics` → Reduction Analysis results
- `m_globalsearch_statistics` → Global Search results

### Phase 4: Result Notification (JobManager Signal)

**Location**: `src/capabilities/jobmanager.cpp`

```cpp
// 1. After storing result, JobManager emits signal
void JobManager::RunJobs()
{
    // ... execute job ...
    QJsonObject result = RunMonteCarlo(job);  // or other method

    // 2. Store in model and get index
    int index = m_model->UpdateStatistic(result);

    // 3. Notify GUI with method type and storage index
    SupraFit::Method method = (SupraFit::Method)job["Method"].toInt();
    emit ShowResult(method, index);
}
```

### Phase 5: GUI Display (ResultsDialog)

**Location**: `src/ui/dialogs/resultsdialog.cpp`

```cpp
// 1. ResultsDialog receives ShowResult signal
void ResultsDialog::ShowResult(SupraFit::Method type, int index)
{
    // 2. Retrieve statistical data from model
    QJsonObject jsonobject = m_model.toStrongRef().data()->getStatistic(type, index);

    // 3. Create display widget
    ResultsWidget* results = new ResultsWidget(jsonobject, m_model, m_wrapper);
    QString name = tr("%1 # %2").arg(SupraFit::Method2Name(type)).arg(index + 1);

    // 4. Add to tabbed interface
    int tab = m_tabs->addTab(results, name);
}

// GUI also populates results list using same pattern
void ResultsDialog::updateResultView()
{
    // Iterate through all stored statistical results
    for (int i = 0; i < m_model.toStrongRef().data()->getMCStatisticResult(); ++i) {
        QJsonObject stats = m_model.toStrongRef().data()->getStatistic(SupraFit::Method::MonteCarlo, i);
        addItem(item, stats, i);
    }
    // ... similar for other methods
}
```

## Data Structure Flow

### Input: Job Configuration
```json
{
  "Method": 1,  // SupraFit::Method::MonteCarlo
  "MaxSteps": 5000,
  "Variance": 0.05,
  "confidence": 95
}
```

### Processing: Merged with Standard Config
```json
{
  "Method": 1,
  "MaxSteps": 5000,    // User override
  "Variance": 0.05,    // User override
  "confidence": 95,    // User override
  "VarianceSource": 2, // Default from MonteCarloConfigBlock
  "EntropyBins": 30,   // Default from MonteCarloConfigBlock
  "StoreRaw": true     // Default from MonteCarloConfigBlock
}
```

### Output: Statistical Result with Numeric Keys
```json
{
  "0": {  // First statistical result
    "boxplot": { /* statistical data */ },
    "confidence": { /* confidence intervals */ }
  },
  "1": {  // Second statistical result
    "boxplot": { /* statistical data */ },
    "confidence": { /* confidence intervals */ }
  },
  "controller": {
    "Method": 1,
    "MaxSteps": 5000,
    "Variance": 0.05,
    "confidence": 95,
    "title": "Model Name",
    "timestamp": 1756542096007
  }
}
```

### Storage: In AbstractModel Vectors
```cpp
// Stored as complete JSON objects in method-specific vectors
m_mc_statistics[0] = { "0": {...}, "1": {...}, "controller": {...} }
m_mc_statistics[1] = { "0": {...}, "1": {...}, "controller": {...} }
```

### Retrieval: By Method and Index
```cpp
// GUI requests specific result
QJsonObject result = model->getStatistic(SupraFit::Method::MonteCarlo, 0);
// Returns: { "0": {...}, "1": {...}, "controller": {...} }
```

## Critical Integration Points

### 1. Numeric Key Generation
**AbstractSearchClass::Result()** is responsible for creating the `"0"`, `"1"`, `"2"` structure:
```cpp
for (int i = 0; i < m_results.size(); ++i)
    result[QString::number(i)] = m_results[i];
```

### 2. Standard Config Blocks
**JobManager** merges user parameters with standard config blocks from `jobmanager.h`:
- `MonteCarloConfigBlock`
- `ModelComparisonConfigBlock`
- `ResampleConfigBlock`
- `GridSearchConfigBlock`

### 3. Method Type Mapping
**SupraFit::Method** enum values in controller determine storage location:
```cpp
case SupraFit::Method::MonteCarlo:        // 1 → m_mc_statistics
case SupraFit::Method::WeakenedGridSearch: // 2 → m_wg_statistics
case SupraFit::Method::ModelComparison:   // 3 → m_moco_statistics
case SupraFit::Method::CrossValidation:   // 4 → m_cv_statistics
```

### 4. Signal-Based Communication
**JobManager** coordinates between analysis and GUI:
```cpp
emit ShowResult(method, index);  // Triggers GUI update
```

## Compliance Requirements

### For All Statistical Functions

1. **Follow JobManager Pattern**: Use config blocks, call AbstractSearchClass pattern
2. **Generate Numeric Keys**: Use AbstractSearchClass::Result() pattern for `"0"`, `"1"`, `"2"`
3. **Include Controller**: Standard config block with Method, MaxSteps, etc.
4. **Store via UpdateStatistic**: Use AbstractModel::UpdateStatistic() for persistence
5. **Signal Integration**: Emit ShowResult() for GUI notifications
6. **Method Type Consistency**: Use correct SupraFit::Method enum values

### Anti-Patterns (Claude Deviations)

❌ **Direct JSON creation without numeric keys**
❌ **Custom config formats instead of JobManager blocks**
❌ **Bypassing AbstractModel storage system**
❌ **Non-standard method type identifiers**
❌ **Direct GUI calls without signal system**

## Validation Checklist

For any statistical implementation:

- [ ] Uses JobManager config block merging pattern
- [ ] Generates numeric keys via AbstractSearchClass::Result()
- [ ] Stores results via AbstractModel::UpdateStatistic()
- [ ] Emits ShowResult() signal for GUI integration
- [ ] Compatible with ResultsDialog::ShowResult() display
- [ ] Uses standard SupraFit::Method enum values
- [ ] Includes proper controller block structure
- [ ] Maintains compatibility with vonHand_mc.json format

## Conclusion

This reference implementation flow is the **authoritative pattern** for all SupraFit statistical analysis. Any deviation breaks integration with the existing JobManager → AbstractSearchClass → AbstractModel → ResultsDialog system that Conrad originally designed.