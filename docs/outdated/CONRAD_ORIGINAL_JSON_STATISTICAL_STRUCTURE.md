# Conrad's Original JSON Statistical Structure - Reference Implementation

## Overview

This document defines the **authoritative reference** for SupraFit's JSON statistical data structure as originally implemented by Conrad Hübler. All statistical analysis functions MUST follow this pattern to ensure compatibility with the existing codebase and ResultsDialog integration.

**Key Principle**: Conrad's original implementation in JobManager → AbstractSearchClass → AbstractModel → ResultsDialog is the **reference standard**. All Claude-generated statistical functions must align with this structure.

## Core JSON Structure Pattern

### Root Project Structure
```json
{
  "data": {
    /* Standard SupraFit project data structure */
  },
  "model_1": {
    "data": {
      "methods": {
        "0": {},           // Method 0: Empty or contains results
        "1": {             // Method 1: Statistical analysis results
          "0": {           // Result index 0
            /* Statistical data */
          },
          "1": {           // Result index 1
            /* Statistical data */
          }
        },
        "2": {             // Method 2: Additional analysis
          /* Results with numeric keys */
        }
      }
    }
  }
}
```

**Critical Design Elements:**
1. **Numeric Keys Only**: All method identifiers and result indices use string numeric keys ("0", "1", "2")
2. **Nested Structure**: `methods` → method_id → result_index → statistical_data
3. **Method-Index Pattern**: Each statistical method can have multiple result indices

## Original Implementation Reference

### 1. JobManager Configuration Blocks (jobmanager.h)

**MonteCarloConfigBlock:**
```cpp
const QJsonObject MonteCarloConfigBlock{
    { "Method", SupraFit::Method::MonteCarlo },  // int = 1
    { "MaxSteps", 2e3 },                         // int
    { "Variance", 1e-3 },                        // double
    { "confidence", 95 },                        // double
    { "VarianceSource", 2 },                     // int: 1=custom, 2=SEy, 3=sigma, 4=bootstrap
    { "EntropyBins", 30 },                       // int
    { "StoreRaw", true },                        // bool
    { "OriginalData", false }                    // bool
};
```

**ModelComparisonConfigBlock:**
```cpp
const QJsonObject ModelComparisonConfigBlock{
    { "Method", SupraFit::Method::ModelComparison },  // int = 3
    { "MaxSteps", 1e4 },                              // int
    { "confidence", 95 },                             // double
    { "ParameterIndex", 0 },                          // int: 0=SSE, 1=SEy, 2=ChiSquared, 3=sigma
    { "StoreRaw", true },                             // bool
    { "GlobalParameterList", "" },                    // string
    { "LocalParameterList", "" }                      // string
};
```

**ResampleConfigBlock:**
```cpp
const QJsonObject ResampleConfigBlock{
    { "Method", SupraFit::Method::CrossValidation },  // int = 4
    { "CXO", 1 },                                     // int: 1=L0O, 2=L2O, 3=LXO
    { "X", 3 },                                       // int: for LXO
    { "MaxSteps", 1e4 },                              // int
    { "Algorithm", 2 },                               // int: 1=Precomputation, 2=Automatic, 3=Random
    { "EntropyBins", 30 },                            // int
    { "StoreRaw", true }                              // bool
};
```

**GridSearchConfigBlock:**
```cpp
const QJsonObject GridSearchConfigBlock{
    { "Method", SupraFit::Method::WeakenedGridSearch },  // int = 2
    { "MaxSteps", 1e3 },                                 // int
    { "confidence", 95 },                                // double
    { "ParameterIndex", 0 },                             // int
    { "ScalingFactor", -4 },                             // int
    { "StoreRaw", false },                               // bool
    { "GlobalParameterList", "" },                       // string
    { "LocalParameterList", "" }                         // string
};
```

### 2. AbstractSearchClass Result Generation

**Core Method (abstractsearchclass.cpp):**
```cpp
QJsonObject AbstractSearchClass::Result() const
{
    QJsonObject result;

    // CRITICAL: Creates numeric key structure
    for (int i = 0; i < m_results.size(); ++i)
        result[QString::number(i)] = m_results[i];

    // Add controller configuration
    QJsonObject controller = Controller();
    controller["title"] = m_model->Name();
    controller["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    return result;
}
```

**Result Structure Generated:**
```json
{
  "0": { /* Statistical result 0 */ },
  "1": { /* Statistical result 1 */ },
  "2": { /* Statistical result 2 */ },
  "controller": {
    "Method": 1,
    "MaxSteps": 2000,
    "Variance": 0.047295,
    "confidence": 95,
    "title": "Model Name",
    "timestamp": 1756542096007
  }
}
```

### 3. AbstractModel Storage/Retrieval System

**Storage Method:**
```cpp
int AbstractModel::UpdateStatistic(const QJsonObject& object)
{
    // Stores statistical results in method-specific vectors
    // Returns index where stored
}
```

**Retrieval Method:**
```cpp
QJsonObject AbstractModel::getStatistic(SupraFit::Method type, int index) const
{
    switch (type) {
    case SupraFit::Method::WeakenedGridSearch:
        if (index < m_wg_statistics.size())
            return m_wg_statistics[index];
    case SupraFit::Method::MonteCarlo:
        if (index < m_mc_statistics.size())
            return m_mc_statistics[index];
    // ... other methods
    }
}
```

### 4. ResultsDialog Integration

**Display Method (resultsdialog.cpp):**
```cpp
void ShowResult(SupraFit::Method type, int index)
{
    QJsonObject result = m_model.toStrongRef().data()->getStatistic(type, index);
    // Display the statistical results in GUI
}
```

## Real-World Example: vonHand_mc.json

**Complete Structure from Actual File:**
```json
{
  "data": {
    "DataType": 1,
    "SupraFit": 2004,
    /* ... project data ... */
  },
  "model_1": {
    "data": {
      "methods": {
        "0": {},
        "1": {
          "0": {
            "boxplot": {
              "count": 2000,
              "extreme_outliers": "...",
              "lower_quantile": -0.857433,
              "lower_whisker": -0.857433,
              "mean": -2.004490097500106,
              "median": -0.857433,
              "stddev": 2.9023484882564414,
              "upper_quantile": -0.857433,
              "upper_whisker": -0.857433
            },
            "confidence": {
              "error": 95,
              "lower": -9.655895000000001,
              "upper": -0.857433
            }
          }
        }
      }
    },
    "controller": {
      "EntropyBins": 30,
      "MaxSteps": 2000,
      "Method": 1,
      "StoreRaw": true,
      "Variance": 0.04729520212443951,
      "VarianceSource": 2,
      "confidence": 95,
      "raw": {
        "0": { /* Raw Monte Carlo data */ },
        "1": { /* Raw Monte Carlo data */ }
      }
    }
  }
}
```

## Method ID Mapping

| Method | ID | Description |
|--------|----|-----------|
| WeakenedGridSearch | 2 | Parameter grid exploration |
| MonteCarlo | 1 | Parameter uncertainty analysis |
| ModelComparison | 3 | Multi-model comparison |
| CrossValidation | 4 | L0O/L2O/CXO validation |
| ReductionAnalyse | 5 | Parameter significance testing |
| FastConfidence | 6 | Fast confidence intervals |
| GlobalSearch | 7 | Global parameter optimization |

## Implementation Requirements

### For All Statistical Functions

1. **Use Numeric Keys Only**: `"0"`, `"1"`, `"2"` - never descriptive names
2. **Follow Nested Structure**: `methods[method_id][result_index][data]`
3. **Include Controller Block**: Use original JobManager config blocks
4. **Maintain Compatibility**: Must work with AbstractModel::getStatistic()
5. **Support ResultsDialog**: GUI must display results correctly

### JSON Response Template

```json
{
  "methods": {
    "1": {  // Method ID (MonteCarlo = 1)
      "0": {  // Result index 0
        "boxplot": { /* statistical data */ },
        "confidence": { /* confidence intervals */ },
        "histogram": { /* distribution data */ }
      }
    }
  },
  "controller": {
    "Method": 1,
    "MaxSteps": 2000,
    "Variance": 0.05,
    "confidence": 95,
    "VarianceSource": 2,
    "EntropyBins": 30,
    "StoreRaw": true,
    "title": "Model Name",
    "timestamp": 1756542096007
  }
}
```

## Common Anti-Patterns (Claude-Generated Deviations)

### ❌ Incorrect Structures
```json
// WRONG: Descriptive keys instead of numeric
{
  "monte_carlo_results": {
    "parameter_uncertainty": { /* data */ }
  }
}

// WRONG: Flat structure without methods hierarchy
{
  "aic_results": { /* data */ },
  "mc_results": { /* data */ }
}

// WRONG: Non-standard controller format
{
  "configuration": {
    "monte_carlo_settings": { /* non-standard config */ }
  }
}
```

### ✅ Correct Structure
```json
// CORRECT: Numeric keys, nested methods, standard controller
{
  "methods": {
    "1": {
      "0": { /* statistical data */ }
    }
  },
  "controller": {
    "Method": 1,
    "MaxSteps": 2000,
    /* ... standard JobManager config ... */
  }
}
```

## Validation Checklist

For any statistical JSON function:

- [ ] Uses numeric keys ("0", "1", "2") for all identifiers
- [ ] Follows `methods[method_id][result_index]` structure
- [ ] Includes standard controller block from JobManager
- [ ] Compatible with AbstractModel::UpdateStatistic()/getStatistic()
- [ ] Works with ResultsDialog::ShowResult()
- [ ] Matches pattern in vonHand_mc.json
- [ ] No descriptive key names (use numeric only)
- [ ] Proper SupraFit::Method enum values in controller

## Conclusion

This structure is the **definitive reference** for all SupraFit statistical JSON. Any deviation from this pattern breaks compatibility with Conrad's original implementation and the integrated GUI system. All statistical analysis functions must be standardized to follow this exact structure.