# Analysis of Post-Processing Methods

This document provides an analysis of each of the post-processing methods in the `capabilities` directory, detailing how their data is generated and stored.

## 1. Common Pattern

A common pattern is used by most of the post-processing methods:

1.  **In-Memory Storage of Full Models:** During the analysis, the methods generate a large number of full model `QJsonObject`s, which are stored in memory.
2.  **Parameter Extraction:** After the analysis is complete, the `ToolSet::Model2Parameter` function is used to process the list of model `QJsonObject`s and extract the distributions of the individual parameter values.
3.  **Final JSON Output:** The final JSON output does not contain the full model objects. Instead, it stores the aggregated results for each parameter, including the raw distribution of values as a space-separated string in the `data.raw` field.

## 2. Analysis of Each Method

### 2.1. `montecarlostatistics`

*   **Purpose:** Performs a Monte Carlo simulation to analyze parameter uncertainty.
*   **Data Generation:** Generates thousands of model `QJsonObject`s by repeatedly fitting the model to data with added random noise.
*   **Raw Data Storage:** Stores the distribution of each parameter's values as a space-separated string in the `data.raw` field.

### 2.2. `modelcomparison`

This capability provides two methods:

*   **`Confidence`:**
    *   **Purpose:** A Monte Carlo-based method for determining confidence intervals.
    *   **Data Generation:** Similar to `montecarlostatistics`, it generates a large number of model `QJsonObject`s.
    *   **Raw Data Storage:** Stores the distribution of each parameter's values as a space-separated string in the `data.raw` field.
*   **`FastConfidence`:**
    *   **Purpose:** A faster, iterative method for estimating confidence intervals.
    *   **Data Generation:** Does not generate full model objects. It iteratively adjusts each parameter to find the confidence interval bounds.
    *   **Raw Data Storage:** Stores the parameter values and their corresponding errors as a string of points in the `points` field.

### 2.3. `resampleanalyse`

This capability provides two methods:

*   **`CrossValidation`:**
    *   **Purpose:** Performs cross-validation on the data.
    *   **Data Generation:** Generates model `QJsonObject`s by fitting the model to data with certain rows left out.
    *   **Raw Data Storage:** Stores the distribution of each parameter's values as a space-separated string in the `data.raw` field.
*   **`PlainReduction`:**
    *   **Purpose:** Performs a "reduction analysis" to evaluate the importance of each data point.
    *   **Data Generation:** Generates model `QJsonObject`s by fitting the model to data with one data point left out at a time.
    *   **Raw Data Storage:** Stores the distribution of each parameter's values as a space-separated string in the `data.raw` field. The `controller` object also contains an `x` field with the values of the independent variable for each of the removed data points.

### 2.4. `weakenedgridsearch`

*   **Purpose:** An iterative method for exploring the parameter space and finding confidence intervals.
*   **Data Generation:** Generates model `QJsonObject`s at each step of the grid search.
*   **Raw Data Storage:** Stores the parameter values and their corresponding errors as two separate space-separated strings in the `x` and `y` fields of a `data` object.

### 2.5. `globalsearch`

*   **Purpose:** Performs a grid search over the parameter space.
*   **Data Generation:** Generates a full model `QJsonObject` for each point in the grid.
*   **Raw Data Storage:** The final result is a list of `QJsonObject`s, where each object contains the `initial` and `optimised` parameter values as strings, and the full `model` `QJsonObject`.

## 3. Conclusion

Most of the post-processing methods in SupraFit follow a similar pattern of generating a large number of full model `QJsonObject`s in memory and then processing this data to extract the final results. The final JSON output is optimized for size by only storing the aggregated parameter distributions and statistical summaries, rather than the full model objects.

The `FastConfidence` and `weakenedgridsearch` methods are exceptions to this pattern, as they use iterative methods that do not require storing a large number of full models.
