# Multiple PostFitAnalysis Runs Implementation Plan

**Status**: Planned (Not yet implemented)
**Priority**: Medium
**Complexity**: High (architectural changes required)
**Date**: 2025-11-02

---

## 1. Problem Statement

### Current Limitation
Currently, SupraFit supports **only ONE analysis per method type per model**:
- One Monte Carlo analysis
- One Cross-Validation analysis
- One Reduction analysis (etc.)

### Desired Feature
Allow **multiple analyses of the SAME type** with different parameters:
- Multiple MC runs with different MaxSteps (500, 1000, 2000)
- Multiple CV runs with different CVType (LOO, LTO, K-fold)
- Compare results across different parameter sets

### Use Cases
1. **Parameter Sensitivity Analysis**: Test how results change with MaxSteps=500 vs 1000 vs 2000
2. **Cross-Validation Comparison**: Compare LOO (CVType=1) vs LTO (CVType=2) results
3. **Robustness Testing**: Multiple independent MC runs to verify statistical stability
4. **Publication Requirements**: Show reproducibility with multiple analysis configurations

---

## 2. Current Architecture Analysis

### JSON Storage Structure (PROBLEMATIC)

```json
{
  "methods": {
    "0": {},  // Unused
    "1": {    // Method ID = Primary Key (only ONE allowed!)
      "0": { /* parameter 0 results */ },
      "1": { /* parameter 1 results */ },
      "controller": {
        "Method": 1,
        "MaxSteps": 1000
      }
    },
    "4": {    // Only ONE CV analysis
      "0": { /* parameter 0 results */ },
      "controller": {
        "Method": 4,
        "CVType": 1
      }
    }
  }
}
```

**Problem**: Method ID is the unique key → only one analysis per method type.

### Code Flow (CURRENT)

```
PostFitAnalysis config
    ↓
AnalysisManager::runPostFitAnalysis()
    ↓
for each method in array:
    - Convert to boolean flags (destroys multiple configs!)
    - Run JobManager with LAST value only
    ↓
Results stored with method ID as key
    ↓
Only LAST analysis is preserved
```

**Issue**: `runPostFitAnalysis()` converts methods array to boolean flags:
```cpp
// analysis_manager.cpp:903-944
if (methodId == 1) {
    convertedConfig["monteCarlo"] = true;  // ❌ Overwrites!
    convertedConfig["mcIterations"] = method["MaxSteps"];  // Uses LAST value
}
```

---

## 3. Proposed JSON Format

### New Structure (BACKWARD COMPATIBLE)

```json
{
  "methods": {
    "1": [  // Changed from object to ARRAY
      {
        "run_index": 0,
        "parameters": {
          "Method": 1,
          "MaxSteps": 500,
          "VarianceSource": 2
        },
        "results": {
          "0": { /* parameter 0 results */ },
          "1": { /* parameter 1 results */ },
          "controller": { "Method": 1, "MaxSteps": 500 }
        }
      },
      {
        "run_index": 1,
        "parameters": {
          "Method": 1,
          "MaxSteps": 1000,
          "VarianceSource": 2
        },
        "results": {
          "0": { /* parameter 0 results */ },
          "1": { /* parameter 1 results */ },
          "controller": { "Method": 1, "MaxSteps": 1000 }
        }
      },
      {
        "run_index": 2,
        "parameters": {
          "Method": 1,
          "MaxSteps": 2000,
          "VarianceSource": 2
        },
        "results": {
          "0": { /* parameter 0 results */ },
          "1": { /* parameter 1 results */ },
          "controller": { "Method": 1, "MaxSteps": 2000 }
        }
      }
    ],
    "4": [  // CV can also have multiple runs
      {
        "run_index": 0,
        "parameters": {
          "Method": 4,
          "CVType": 1,  // LOO
          "MaxSteps": 100
        },
        "results": { /* ... */ }
      },
      {
        "run_index": 1,
        "parameters": {
          "Method": 4,
          "CVType": 2,  // LTO
          "MaxSteps": 100
        },
        "results": { /* ... */ }
      }
    ]
  }
}
```

### JSON Configuration (USER INPUT)

```json
{
  "PostFitAnalysis": {
    "enabled": true,
    "methods": [
      {
        "Method": 1,
        "MaxSteps": 500,
        "VarianceSource": 2
      },
      {
        "Method": 1,
        "MaxSteps": 1000,
        "VarianceSource": 2
      },
      {
        "Method": 1,
        "MaxSteps": 2000,
        "VarianceSource": 2
      },
      {
        "Method": 4,
        "CVType": 1,
        "MaxSteps": 100
      },
      {
        "Method": 4,
        "CVType": 2,
        "MaxSteps": 100
      }
    ]
  }
}
```

### Backward Compatibility

For OLD config with single method per type:
```json
{
  "PostFitAnalysis": {
    "methods": [
      { "Method": 1, "MaxSteps": 1000 },
      { "Method": 4, "CVType": 1 }
    ]
  }
}
```

**Auto-conversion**: System automatically wraps in array:
```json
{
  "1": [
    {
      "run_index": 0,
      "parameters": { "Method": 1, "MaxSteps": 1000 },
      "results": { /* ... */ }
    }
  ]
}
```

---

## 4. Required Code Changes

### 4.1 AnalysisManager::runPostFitAnalysis()
**File**: `src/core/analysis_manager.cpp:903-944`

**Change**: Stop converting to boolean flags, preserve ALL configurations

```cpp
// CURRENT (BROKEN):
QJsonArray methods = analysisConfig["methods"].toArray();
for (const QJsonValue& methodValue : methods) {
    int methodId = methodValue.toObject()["Method"].toInt();
    if (methodId == 1) {
        convertedConfig["monteCarlo"] = true;  // ❌ Overwrites!
        convertedConfig["mcIterations"] = methodValue.toObject()["MaxSteps"];
    }
}

// PROPOSED:
QJsonObject methodsResults;  // Store all runs
for (const QJsonValue& methodValue : methods) {
    QJsonObject methodConfig = methodValue.toObject();
    int methodId = methodConfig["Method"].toInt();

    // Run analysis and collect results
    QJsonObject results = runSingleMethodAnalysis(methodConfig, model);

    // Store in array by method ID
    if (!methodsResults.contains(QString::number(methodId))) {
        methodsResults[QString::number(methodId)] = QJsonArray();
    }
    QJsonArray& methodArray = methodsResults[QString::number(methodId)].toArray();

    QJsonObject runEntry;
    runEntry["run_index"] = methodArray.size();
    runEntry["parameters"] = methodConfig;
    runEntry["results"] = results;

    methodArray.append(runEntry);
}
```

### 4.2 JobManager Integration
**File**: `src/core/jobmanager.h/cpp`

**Changes**:
- Modify to accept multiple job configurations of same type
- Return indexed results instead of single result
- Support parallel execution if needed

```cpp
// For each method configuration:
JobManager jobMgr;
QJsonObject analysisResults = jobMgr.RunJob(
    JobType::MONTE_CARLO,
    methodConfig,
    model
);
// Results should include all 1000 MC samples, entropy calculations, etc.
```

### 4.3 Data Structure Changes
**File**: `src/core/analyse.cpp:CalculateMCMetrics()`

**Changes**:
- Ensure results contain both raw distribution data AND compact metrics
- Don't lose statistical data during storage

### 4.4 Post-Processing Comparison Functions
**Files**:
- `src/core/analyse.cpp:CompareMC()`
- `src/core/analyse.cpp:CompareCV()`

**Changes**:
- Update to handle arrays of results per method
- Implement cross-run comparison logic
- Generate comparison statistics across runs

```cpp
// EXAMPLE: Compare multiple MC runs
QJsonObject CompareMC(const QJsonArray& mcRuns) {
    QJsonObject comparison;

    // Extract all entropy values
    QVector<double> entropies;
    for (const auto& run : mcRuns) {
        // Extract from run["results"]
        entropies.append(extractEntropy(run));
    }

    // Calculate comparison metrics
    comparison["entropy_mean"] = calculateMean(entropies);
    comparison["entropy_std"] = calculateStd(entropies);
    comparison["entropy_range"] = {
        "min": *std::min_element(entropies.begin(), entropies.end()),
        "max": *std::max_element(entropies.begin(), entropies.end())
    };

    return comparison;
}
```

### 4.5 AnalysisManager::fitSingleModel()
**File**: `src/core/analysis_manager.cpp:540-620`

**Changes**:
- Pass methods array to runPostFitAnalysis()
- Don't filter to single method per type
- Store all results properly

---

## 5. Migration Strategy

### Phase 1: Backward Compatibility (Week 1)
- Detect old format (object-style methods)
- Auto-convert to new array format internally
- All existing code/configs continue to work

### Phase 2: Core Implementation (Weeks 2-3)
- Implement new AnalysisManager logic
- Update JobManager integration
- Ensure results store properly

### Phase 3: UI Updates (Week 4)
- GUI: Display multiple runs per method
- Comparison visualizations
- Export/import multiple analyses

### Phase 4: Testing & Validation (Week 5)
- Unit tests for multiple runs
- Integration tests with ml_pipeline
- Documentation updates

---

## 6. Testing Strategy

### Unit Tests

```cpp
// test_multirun_analysis.cpp
TEST(MultiRunPostFitAnalysis, MultipleMCRuns) {
    // Create config with 3 MC runs
    QJsonObject config = {
        "methods": [
            { "Method": 1, "MaxSteps": 500 },
            { "Method": 1, "MaxSteps": 1000 },
            { "Method": 1, "MaxSteps": 2000 }
        ]
    };

    // Run analysis
    AnalysisManager mgr;
    QJsonObject results = mgr.runPostFitAnalysis(config, model);

    // Verify structure
    ASSERT_TRUE(results.contains("1"));
    ASSERT_EQ(results["1"].toArray().size(), 3);

    // Verify parameters preserved
    auto run0 = results["1"].toArray()[0].toObject();
    ASSERT_EQ(run0["parameters"]["MaxSteps"].toInt(), 500);
}

TEST(MultiRunPostFitAnalysis, MixedMethodRuns) {
    // Test MC + CV with multiple runs each
}

TEST(MultiRunPostFitAnalysis, BackwardCompatibility) {
    // Old single-run config still works
}
```

### Integration Tests

```cpp
// test_ml_pipeline_multirun.cpp
TEST(MLPipelineMultiRun, GeneratesCorrectOutput) {
    // Run ml_pipeline with 3 MC runs
    // Verify -ml-features.json contains all 3 results
    // Verify statistics are calculated for each run
    // Verify comparison metrics are present
}
```

### Example Test Data

```json
{
  "input": "examples/ml_pipeline_multirun_example.json",
  "expected_output": {
    "methods": {
      "1": [
        { "run_index": 0, "parameters": { "MaxSteps": 500 }, "results": { /* ... */ } },
        { "run_index": 1, "parameters": { "MaxSteps": 1000 }, "results": { /* ... */ } },
        { "run_index": 2, "parameters": { "MaxSteps": 2000 }, "results": { /* ... */ } }
      ]
    }
  }
}
```

---

## 7. Example Use Cases

### Use Case 1: MC Parameter Sensitivity

**Input**:
```json
{
  "PostFitAnalysis": {
    "methods": [
      { "Method": 1, "MaxSteps": 500 },
      { "Method": 1, "MaxSteps": 1000 },
      { "Method": 1, "MaxSteps": 5000 }
    ]
  }
}
```

**Output**: Compare how entropy and confidence intervals change with different MaxSteps.

### Use Case 2: Cross-Validation Comparison

**Input**:
```json
{
  "PostFitAnalysis": {
    "methods": [
      { "Method": 4, "CVType": 1 },  // LOO
      { "Method": 4, "CVType": 2 },  // LTO
      { "Method": 4, "CVType": 3 }   // K-fold
    ]
  }
}
```

**Output**: Compare LOO vs LTO vs K-fold results.

### Use Case 3: Statistical Robustness

**Input**:
```json
{
  "PostFitAnalysis": {
    "methods": [
      { "Method": 1, "MaxSteps": 1000, "RandomSeed": 12345 },
      { "Method": 1, "MaxSteps": 1000, "RandomSeed": 54321 },
      { "Method": 1, "MaxSteps": 1000, "RandomSeed": 99999 }
    ]
  }
}
```

**Output**: Verify results are consistent across different random seeds.

---

## 8. Implementation Checklist

- [ ] Design JSON structure (proposed above)
- [ ] Implement AnalysisManager changes
- [ ] Update JobManager integration
- [ ] Implement comparison functions
- [ ] Add backward compatibility layer
- [ ] Write unit tests (>80% coverage)
- [ ] Write integration tests
- [ ] Update GUI to display multiple runs
- [ ] Create example configuration files
- [ ] Update user documentation
- [ ] Update API documentation
- [ ] Performance testing (ensure no slowdown)
- [ ] Release notes and changelog

---

## 9. Risks & Mitigation

| Risk | Severity | Mitigation |
|------|----------|-----------|
| JSON format change breaks existing files | HIGH | Implement auto-conversion from old→new format |
| Code complexity increases significantly | MEDIUM | Careful refactoring, thorough testing |
| Performance impact with many runs | MEDIUM | Profile and optimize JobManager |
| UI becomes cluttered with many results | LOW | Implement tabs/collapsible sections |
| GUI doesn't support display | MEDIUM | Start with CLI-only, add GUI later |

---

## 10. Timeline Estimate

- **Phase 1 (Backward Compatibility)**: 1 week
- **Phase 2 (Core Implementation)**: 2 weeks
- **Phase 3 (UI Updates)**: 1 week
- **Phase 4 (Testing)**: 1 week
- **Total**: 5 weeks

---

## 11. Success Criteria

- ✅ Multiple analyses per method type work correctly
- ✅ All existing single-run configs still work (backward compatible)
- ✅ Results are properly indexed and accessible
- ✅ Comparison metrics are calculated across runs
- ✅ >80% test coverage for new code
- ✅ No performance regression (<5% slower)
- ✅ Documentation is complete and clear
- ✅ Example files demonstrate all use cases

---

## 12. Future Enhancements

1. **Parallel Execution**: Run multiple analyses in parallel for faster results
2. **Batch Comparison**: Automated comparison reports across all methods
3. **Machine Learning**: Use ML to predict optimal parameter combinations
4. **Visualization**: Interactive plots comparing run results
5. **Statistical Tests**: Automated significance testing between runs

---

**Document Version**: 1.0
**Last Updated**: 2025-11-02
**Status**: Ready for Implementation Planning
