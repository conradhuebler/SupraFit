# Claude-Generated Deviations Audit - Statistical JSON Functions

## Overview

This document identifies all Claude-generated statistical functions in `src/core/analyse.cpp` that deviate from Conrad's original JSON structure pattern. These functions must be standardized to use numeric keys ("0", "1", "2") and proper controller blocks.

**Critical Finding**: 9 Claude-generated functions use non-standard JSON structures that are incompatible with Conrad's original JobManager → AbstractSearchClass → AbstractModel → ResultsDialog system.

## Deviation Summary

### ❌ Non-Compliant Claude Functions

| Function | Location | Primary Deviation | Status |
|----------|----------|-------------------|---------|
| `CalculateAICMetrics` | analyse.cpp:830+ | Descriptive keys instead of numeric | ❌ NEEDS FIX |
| `CalculateCVMetrics` | analyse.cpp:860+ | Custom CV structure | ❌ NEEDS FIX |
| `CalculateMCMetrics` | analyse.cpp:1050+ | Reads original but outputs Claude format | ❌ NEEDS FIX |
| `CalculateReductionMetrics` | analyse.cpp:1150+ | Custom reduction structure | ❌ NEEDS FIX |
| `CalculateWGSMetrics` | analyse.cpp:1250+ | Custom grid search format | ❌ NEEDS FIX |
| `CalculateModelComparisonMetrics` | analyse.cpp:1350+ | Custom comparison format | ❌ NEEDS FIX |
| `CalculateFastConfidenceMetrics` | analyse.cpp:1450+ | Custom confidence format | ❌ NEEDS FIX |
| `CalculateGlobalSearchMetrics` | analyse.cpp:1550+ | Custom global search format | ❌ NEEDS FIX |
| `ExtractModelMLFeatures` | analyse.cpp:1650+ | ML-specific format | ⚠️ REVIEW NEEDED |

## Detailed Deviation Analysis

### 1. CalculateAICMetrics - AIC Model Comparison

**Current Claude Structure (❌ WRONG):**
```json
{
  "models": [
    {"index": 0, "name": "1:1 Model", "aic": 125.4, "parameters": 3}
  ],
  "aic_ranking": [
    {"rank": 1, "model": "0 - 1:1 Model", "aic": 125.4, "evidence_ratio": 1.0}
  ],
  "best_aic": 125.4
}
```

**Required Conrad Structure (✅ CORRECT):**
```json
{
  "methods": {
    "3": {  // ModelComparison method
      "0": {
        "models": [ /* model data */ ],
        "ranking": [ /* ranking data */ ]
      }
    }
  },
  "controller": {
    "Method": 3,  // SupraFit::Method::ModelComparison
    "MaxSteps": 10000,
    "confidence": 95,
    "ParameterIndex": 0,
    "title": "AIC Model Comparison",
    "timestamp": 1756542096007
  }
}
```

### 2. CalculateMCMetrics - Monte Carlo Analysis

**Current Claude Structure (❌ WRONG):**
```json
{
  "max_steps": 10000,
  "variance": 0.05,
  "models": [{
    "name": "1:1 Model",
    "parameters": [{"name": "K11", "mean": 4.25, "stddev": 0.12}]
  }],
  "parameter_averages": [{"parameter": "K11", "avg_stddev": 0.12}]
}
```

**Required Conrad Structure (✅ CORRECT):**
```json
{
  "methods": {
    "1": {  // MonteCarlo method
      "0": {
        "boxplot": {
          "count": 2000,
          "mean": -2.004490097500106,
          "stddev": 2.9023484882564414,
          "lower_quantile": -0.857433,
          "upper_quantile": -0.857433
        },
        "confidence": {
          "error": 95,
          "lower": -9.655895000000001,
          "upper": -0.857433
        }
      }
    }
  },
  "controller": {
    "Method": 1,  // SupraFit::Method::MonteCarlo
    "MaxSteps": 2000,
    "Variance": 0.04729520212443951,
    "VarianceSource": 2,
    "confidence": 95,
    "EntropyBins": 30,
    "StoreRaw": true,
    "title": "Monte Carlo Analysis",
    "timestamp": 1756542096007
  }
}
```

### 3. CalculateCVMetrics - Cross Validation

**Current Claude Structure (❌ WRONG):**
```json
{
  "cross_validation_type": "L0O",
  "models": [
    {"name": "Model1", "cv_error": 0.234, "prediction_variance": 0.045}
  ],
  "entropy_ranking": [
    {"model": "Model1", "entropy": 2.45, "rank": 1}
  ]
}
```

**Required Conrad Structure (✅ CORRECT):**
```json
{
  "methods": {
    "4": {  // CrossValidation method
      "0": {
        "prediction_errors": [ /* error data */ ],
        "validation_metrics": [ /* validation data */ ]
      }
    }
  },
  "controller": {
    "Method": 4,  // SupraFit::Method::CrossValidation
    "CXO": 1,     // L0O
    "MaxSteps": 10000,
    "Algorithm": 2,
    "EntropyBins": 30,
    "title": "Cross Validation Analysis",
    "timestamp": 1756542096007
  }
}
```

### 4. CalculateReductionMetrics - Parameter Reduction

**Current Claude Structure (❌ WRONG):**
```json
{
  "cutoff": 0.1,
  "models": [
    {"name": "Model1", "significance": 0.85, "reduced_parameters": 2}
  ],
  "parameter_significance": [
    {"parameter": "K11", "significance": 0.95, "keep": true}
  ]
}
```

**Required Conrad Structure (✅ CORRECT):**
```json
{
  "methods": {
    "5": {  // ReductionAnalyse method
      "0": {
        "parameter_analysis": [ /* parameter data */ ],
        "significance_scores": [ /* significance data */ ]
      }
    }
  },
  "controller": {
    "Method": 5,  // SupraFit::Method::ReductionAnalyse
    "ReductionRuntype": 1,  // backward
    "MaxSteps": 10000,
    "EntropyBins": 30,
    "title": "Parameter Reduction Analysis",
    "timestamp": 1756542096007
  }
}
```

## Anti-Pattern Analysis

### ❌ Common Claude Deviations

1. **Descriptive Keys**: Using "models", "aic_ranking", "best_aic" instead of "0", "1", "2"
2. **Flat Structure**: No nested "methods" hierarchy
3. **Missing Controller**: No JobManager config block with Method, MaxSteps, etc.
4. **Custom Formats**: Each function invents its own JSON structure
5. **Non-Standard Metadata**: Using "method_type", "execution_status" instead of controller
6. **Direct Arrays**: Using QJsonArray instead of numeric key objects

### ❌ Compatibility Issues

1. **AbstractModel Storage**: Cannot use UpdateStatistic() with non-standard structure
2. **ResultsDialog Display**: GUI expects getStatistic() to return standard format
3. **JobManager Integration**: No way to merge with standard config blocks
4. **Signal System**: Cannot emit ShowResult() with proper method/index
5. **vonHand_mc.json**: Incompatible with existing statistical data files

## Special Case: ExtractModelMLFeatures

**Current Structure:**
```json
{
  "model_name": "¹H 1:1-Model",
  "model_id": 1,
  "parameters": {
    "global_count": 1,
    "local_count": 4,
    "total_count": 5
  },
  "fit_quality": {
    "aic": -37.25,
    "r_squared": 0.9998,
    "sse": 0.0170
  }
}
```

**Status**: ⚠️ **REVIEW NEEDED** - This function is designed for ML training data export and may need a different approach than standard statistical analysis. However, it should still be compatible with the overall system architecture.

## Standardization Requirements

### For Each Claude Function

1. **Wrap in methods structure**: `{"methods": {"X": {"0": { /* data */ }}}}`
2. **Add controller block**: Use standard JobManager config blocks
3. **Use numeric keys**: Replace all descriptive keys with "0", "1", "2"
4. **Follow method ID mapping**: Use correct SupraFit::Method enum values
5. **Include timestamp**: Add controller metadata (title, timestamp)
6. **Maintain data content**: Preserve statistical calculations, just restructure format

### Method ID Mapping for Standardization

| Function | Method ID | SupraFit::Method | Controller Block |
|----------|-----------|------------------|------------------|
| CalculateAICMetrics | 3 | ModelComparison | ModelComparisonConfigBlock |
| CalculateMCMetrics | 1 | MonteCarlo | MonteCarloConfigBlock |
| CalculateCVMetrics | 4 | CrossValidation | ResampleConfigBlock |
| CalculateReductionMetrics | 5 | ReductionAnalyse | ResampleConfigBlock |
| CalculateWGSMetrics | 2 | WeakenedGridSearch | GridSearchConfigBlock |
| CalculateModelComparisonMetrics | 3 | ModelComparison | ModelComparisonConfigBlock |
| CalculateFastConfidenceMetrics | 6 | FastConfidence | ModelComparisonConfigBlock |
| CalculateGlobalSearchMetrics | 7 | GlobalSearch | Custom config needed |

## Legacy String Functions (✅ COMPATIBLE)

These functions follow original Conrad pattern and should be preserved:

- `AnalyseReductionAnalysis()` - String-based reduction analysis
- `CompareAIC()` - String-based AIC comparison
- `CompareMC()` - String-based Monte Carlo comparison
- `CompareCV()` - String-based cross-validation comparison

## Standardization Priority

### Phase 1: Critical Functions (High Priority)
1. **CalculateMCMetrics** - Already tries to read original structure
2. **CalculateAICMetrics** - Most commonly used for model selection
3. **CalculateCVMetrics** - Important for validation

### Phase 2: Extended Functions (Medium Priority)
4. **CalculateReductionMetrics** - Parameter significance testing
5. **CalculateWGSMetrics** - Grid search analysis
6. **CalculateModelComparisonMetrics** - Model comparison

### Phase 3: Specialized Functions (Low Priority)
7. **CalculateFastConfidenceMetrics** - Fast confidence intervals
8. **CalculateGlobalSearchMetrics** - Global optimization
9. **ExtractModelMLFeatures** - ML training data (review approach)

## Expected Benefits After Standardization

1. **Full Compatibility**: All functions work with AbstractModel storage system
2. **GUI Integration**: ResultsDialog can display all results properly
3. **JobManager Coordination**: Proper integration with job scheduling system
4. **File Format Consistency**: Compatible with vonHand_mc.json and other existing files
5. **Signal System**: Proper ShowResult() emission for GUI updates
6. **Unified Architecture**: Single consistent statistical analysis system

## Conclusion

The audit reveals **systematic deviation** from Conrad's original JSON structure across 9 statistical functions. All functions must be standardized to use numeric keys, methods hierarchy, and proper controller blocks to restore compatibility with the integrated SupraFit system.