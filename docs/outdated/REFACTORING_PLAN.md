# Refactoring Plan for SupraFit JSON Handling

This document outlines a plan to refactor the JSON handling in SupraFit, with the goal of reducing redundancy and improving code clarity.

## 1. Create a new helper function `getPostFitAnalysis` in `MLFeatureExtractor`

*   **Function:** `QJsonObject getPostFitAnalysis(const QJsonObject& object)`
*   **Location:** `src/capabilities/mlfeatureextractor.h` and `src/capabilities/mlfeatureextractor.cpp`
*   **Purpose:** This function will take a `QJsonObject` as input and will contain the logic to find and extract the `post_fit_analysis` object, regardless of whether it's in a Type A, B, or C structure. It will return the `post_fit_analysis` object, or an empty `QJsonObject` if not found.

## 2. Refactor `parseMLPipelineData` to use `getPostFitAnalysis`

*   **File:** `src/capabilities/mlfeatureextractor.cpp`
*   **Purpose:** The complex `if-else if` chain in `parseMLPipelineData` will be simplified. It will call `getPostFitAnalysis` to get the `post_fit_analysis` object. The rest of the function will then work with the extracted object, reducing code duplication.

## 3. Create a unified data access function for statistical data

*   **Function:** `QJsonObject getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method)`
*   **Location:** `src/capabilities/mlfeatureextractor.h` and `src/capabilities/mlfeatureextractor.cpp` (or a new utility class)
*   **Purpose:** This function will take a `QJsonObject` (representing a model) and a `SupraFit::Method` enum as input. It will use `getPostFitAnalysis` to find the "methods" object. It will then extract and return the requested statistical method's data, handling the different access paths (`results` sub-object vs. direct access).

## 4. Refactor `analyse.cpp` to use the new unified function

*   **File:** `src/core/analyse.cpp`
*   **Purpose:** The `Compare...` and `Calculate...Metrics` functions in `analyse.cpp` will be updated to use the new `getStatisticalMethod` function. This will simplify the code in `analyse.cpp` and make it more robust to changes in the JSON structure.

## 5. Remove redundant code

*   **Purpose:** Once the refactoring is complete, identify and remove any redundant code, such as the old string-based analysis functions in `analyse.cpp` if they are no longer needed.

## 6. Update documentation

*   **File:** `src/capabilities/SUPRAFIT_JSON_FORMAT.md`
*   **Purpose:** The documentation will be updated to reflect the new, simplified data access methods.
