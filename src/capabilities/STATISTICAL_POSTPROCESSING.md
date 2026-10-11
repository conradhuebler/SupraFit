# Statistical Post-Processing Configuration

This document describes the JSON configuration structure for controlling the statistical post-processing capabilities in SupraFit.

## 1. Overview

The `JobManager` class is the central point for handling statistical post-processing. It receives a list of jobs, each defined by a `QJsonObject`, and then calls the appropriate handler for each job. The configuration for each job is passed as a `QJsonObject` to the respective handler.

## 2. Job Configuration

A job is defined by a JSON object with a `Method` key that specifies the statistical method to be used. The other keys in the object are specific to the chosen method.

**Example:**

```json
{
  "Jobs": {
    "monte_carlo": {
      "Method": 1,
      "MaxSteps": 1000,
      "VarianceSource": 2,
      "EntropyBins": 30
    },
    "cross_validation": {
      "Method": 4,
      "CXO": 1,
      "Algorithm": 2,
      "MaxSteps": 1000
    }
  }
}
```

## 3. Statistical Methods

### 3.1. Monte Carlo Simulation

- **Method ID:** 1 (`SupraFit::Method::MonteCarlo`)
- **Configuration Block:** `MonteCarloConfigBlock`

| Parameter | Type | Description |
|---|---|---|
| `MaxSteps` | integer | The maximum number of Monte Carlo steps to perform. |
| `Variance` | double | The variance to be used for the simulation. |
| `VarianceSource` | integer | The source of the variance (1: custom, 2: SEy, 3: sigma, 4: bootstrap). |
| `OriginalData` | boolean | Whether to use the original data or the re-fitted data for the simulation. |
| `IndependentRowVariance` | string | A string of comma-separated values representing the variance to be applied to each row of the independent data. |
| `PlotBins` | integer | The number of bins to use for histogram plotting. |
| `EntropyBins` | integer | The number of bins to use for entropy calculation. |
| `StoreRaw` | boolean | Whether to store the raw data from the simulation. |
| `LightWeight` | boolean | Whether to store as little data as possible. |

### 3.2. Cross-Validation

- **Method ID:** 2 (`SupraFit::Method::CrossValidation`)
- **Configuration Block:** `ResampleConfigBlock`

| Parameter | Type | Description |
|---|---|---|
| `CXO` | integer | The type of cross-validation to perform (1: Leave-One-Out, 2: Leave-Two-Out, 3: Leave-Many-Out). |
| `X` | integer | The number of data points to leave out for Leave-Many-Out cross-validation. |
| `MaxSteps` | integer | The maximum number of steps to perform for LXO CV. |
| `Algorithm` | integer | The algorithm to use for selecting the data points to leave out (1: Precomputation, 2: Automatic, 3: Random). |
| `PlotBins` | integer | The number of bins to use for histogram plotting. |
| `EntropyBins` | integer | The number of bins to use for entropy calculation. |
| `StoreRaw` | boolean | Whether to store the raw data from the simulation. |
| `LightWeight` | boolean | Whether to store as little data as possible. |
| `LeftOutPoints` | boolean | Whether to calculate the left-out points. |

### 3.3. Model Comparison

- **Method ID:** 3 (`SupraFit::Method::ModelComparison`) or 5 (`SupraFit::Method::FastConfidence`)
- **Configuration Block:** `ModelComparisonConfigBlock`

| Parameter | Type | Description |
|---|---|---|
| `MaxSteps` | integer | The maximum number of steps to perform. |
| `MaxStepsFastConfidence` | integer | The maximum number of steps for Fast Confidence. |
| `FastConfidenceScaling` | integer | The scaling factor for the step size in Fast Confidence. |
| `MaxParameter` | double | The parameter threshold defined by F-Statistics. |
| `confidence` | double | The confidence level in %. |
| `ParameterIndex` | integer | The index of the statistical parameter to be analyzed. |
| `f_value` | double | The corresponding f-value. |
| `ErrorConvergency` | double | The threshold for convergence in SSE. |
| `BoxScalingFactor` | double | The scaling factor for the box. |
| `GlobalParameterList` | string | A string of comma-separated integers representing the global parameters to be tested. |
| `LocalParameterList` | string | A string of comma-separated integers representing the local parameters to be tested. |
| `GlobalParameterScalingList` | string | A string of comma-separated doubles representing the scaling factors for the global parameters. |
| `LocalParameterScalingList` | string | A string of comma-separated doubles representing the scaling factors for the local parameters. |
| `IncludeSeries` | boolean | Whether to include the series in Fast Confidence. |
| `StoreRaw` | boolean | Whether to store the raw data from the simulation. |
| `LightWeight` | boolean | Whether to store as little data as possible. |

### 3.4. Weakened Grid Search

- **Method ID:** 4 (`SupraFit::Method::WeakenedGridSearch`)
- **Configuration Block:** `GridSearchConfigBlock`

| Parameter | Type | Description |
|---|---|---|
| `MaxSteps` | integer | The maximum number of steps to perform. |
| `MaxParameter` | double | The parameter threshold defined by F-Statistics. |
| `confidence` | double | The confidence level in %. |
| `f_value` | double | The corresponding f-value. |
| `ParameterIndex` | integer | The index of the statistical parameter to be analyzed. |
| `ErrorConvergency` | double | The threshold for convergence in SSE. |
| `OvershotCounter` | integer | The maximum number of steps allowed to be above the SSE threshold. |
| `ErrorDecreaseCounter` | integer | The maximum number of steps where the error is allowed to decrease. |
| `ErrorConvergencyCounter` | integer | The amount for all error changes below the threshold `error_conv`. |
| `ScalingFactor` | integer | The scaling factor for the step size. |
| `StoreRaw` | boolean | Whether to store the raw data from the simulation. |
| `LightWeight` | boolean | Whether to store as little data as possible. |
| `GlobalParameterList` | string | A string of comma-separated integers representing the global parameters to be tested. |
| `LocalParameterList` | string | A string of comma-separated integers representing the local parameters to be tested. |

### 3.5. Reduction Analysis

- **Method ID:** 6 (`SupraFit::Method::Reduction`)
- **Configuration Block:** `ResampleConfigBlock`

| Parameter | Type | Description |
|---|---|---|
| `ReductionRuntype` | integer | The type of reduction analysis to perform (1: backward, 2: forward, 3: both). |

### 3.6. Global Search

- **Method ID:** 7 (`SupraFit::Method::GlobalSearch`)
- **Configuration:** The configuration for the global search is passed directly to the `GlobalSearch` class as a `QJsonObject`. The parameters are defined by the user in the JSON file.

## 4. Analysis of Unused or Ineffective Settings

This section provides an analysis of the settings that are defined in the configuration blocks but are either unused or have no effect on the outcome of the analysis.

### 4.1. Monte Carlo Simulation

- **`StoreRaw`**: This parameter is defined in the `MonteCarloConfigBlock` but is not used in the `MonteCarloStatistics::Run()` function. The `LightWeight` parameter is used instead to control whether the raw data is stored. Therefore, `StoreRaw` is **ineffective**.

### 4.2. Cross-Validation

- **`LightWeight`**: This parameter is not used in the `ResampleAnalyse::CrossValidation()` function. Therefore, it is **ineffective**.
- **`StoreRaw`**: This parameter is not used in the `ResampleAnalyse::CrossValidation()` function. Therefore, it is **ineffective**.
- **`PlotBins`**: This parameter is not used in the `ResampleAnalyse::CrossValidation()` function. Therefore, it is **ineffective**.

### 4.3. Model Comparison

- **`LightWeight`**: This parameter is not used in the `ModelComparison::Confidence()` or `ModelComparison::FastConfidence()` functions. Therefore, it is **ineffective**.
- **`StoreRaw`**: This parameter is not used in the `ModelComparison::Confidence()` or `ModelComparison::FastConfidence()` functions. Therefore, it is **ineffective**.
- **`f_value`**: This parameter is defined in the `ModelComparisonConfigBlock` but is not used in the `ModelComparison` class. Therefore, it is **unused**.
- **`MaxParameter`**: This parameter is used in the `MCThread::run()` function, but it is set from the controller, not from the `ModelComparisonConfigBlock`. Therefore, the value in the config block is **unused**.

### 4.4. Weakened Grid Search

- **`LightWeight`**: This parameter is not used in the `WeakenedGridSearch::Run()` function. Therefore, it is **ineffective**.
- **`StoreRaw`**: This parameter is not used in the `WeakenedGridSearch::Run()` function. Therefore, it is **ineffective**.
- **`f_value`**: This parameter is defined in the `GridSearchConfigBlock` but is not used in the `WeakenedGridSearch` class. Therefore, it is **unused**.
- **`confidence`**: This parameter is defined in the `GridSearchConfigBlock` but is not used in the `WeakenedGridSearch` class. Therefore, it is **unused**.
