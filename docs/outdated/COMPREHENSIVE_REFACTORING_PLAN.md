# Comprehensive Refactoring Plan for SupraFit

This document provides a comprehensive analysis of the SupraFit data structure and a detailed plan for refactoring the codebase to reduce redundancy and improve clarity.

## 1. SupraFit Data Structure

### 1.1. Base Data Structure

The base data structure is defined by the `DataClass` and `AbstractModel` classes. It consists of a main JSON object with the following key components:

*   **`data`**: A `QJsonObject` containing the core data, including:
    *   `independent` and `dependent`: `DataTable` objects with the main data.
    *   `independent_raw` and `dependent_raw`: `DataTable` objects with the raw, unprocessed data.
    *   `system`: A `QJsonObject` with system parameters.
    *   `raw`: A `QJsonObject` for storing additional, model-specific raw data.
*   **`model_X`**: A `QJsonObject` for each model, containing:
    *   `data`: A `QJsonObject` with the model's parameters (`globalParameter`, `localParameter`) and the results of the statistical analysis (`methods`).
    *   `options`: A `QJsonObject` with model-specific options.
    *   Other metadata about the model (name, type, etc.).

### 1.2. Additions from CLI and Machine Learning Functions

The CLI and machine learning functions add a significant amount of data to the base structure, primarily for the purpose of logging and analysis.

*   **`ml_pipeline`**: The `mlfeatureextractor` capability adds a large `ml_pipeline` object to the `data.raw` field. This object contains:
    *   `generation_config`: Information about how the data was generated, including the ground truth model and noise parameters.
    *   `fitted_models`: A list of all the models that were fitted to the data, including their parameters and fit quality metrics.
*   **Statistical Methods Data:** The various post-processing methods in the `capabilities` directory add their results to the `methods` object within each model's `data` object. This includes:
    *   **Raw Parameter Distributions:** For methods like Monte Carlo and Cross-Validation, the raw distribution of values for each parameter is stored as a space-separated string in the `data.raw` field of the parameter's result object.
    *   **Full Model Objects (In-Memory):** During the execution of these methods, a large number of full model `QJsonObject`s are stored in memory. This data is **not** saved to the final JSON file, but it is used to generate the final parameter distributions.

## 2. Redundancies and Refactoring Opportunities

### 2.1. Redundant Data Structures

The biggest redundancy in the data structure is the different ways that the statistical analysis results are stored. As documented in `SUPRAFIT_JSON_FORMAT.md`, there are three different paths to the same data, depending on the file type.

### 2.2. Redundant Code

This redundant data structure leads to a significant amount of redundant code, especially in:

*   **`mlfeatureextractor.cpp`**: The `parseMLPipelineData` function has a complex and repetitive `if-else if` chain to handle the different JSON types.
*   **`analyse.cpp`**: The `Compare...` and `Calculate...Metrics` functions have to handle the different data access paths, making them more complex than necessary.

### 2.3. Refactoring Plan

To address these issues, I propose the following refactoring plan:

1.  **Create a Unified Data Access Layer:**
    *   Create a new class, `SupraFit::JsonUtils`, to provide a single, unified interface for accessing data from the SupraFit JSON structure.
    *   This class will contain the following static functions:
        *   `QJsonObject getPostFitAnalysis(const QJsonObject& object)`: This function will find and return the `post_fit_analysis` object, regardless of the JSON structure type.
        *   `QJsonObject getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method)`: This function will return the results of a specific statistical method for a given model.
        *   `QVector<qreal> getParameterDistribution(const QJsonObject& paramObject)`: This function will extract the raw parameter distribution from the `data.raw` field.

2.  **Refactor `mlfeatureextractor.cpp`:**
    *   Rewrite the `parseMLPipelineData` function to use the new `JsonUtils` class. This will significantly simplify the code and remove the `if-else if` chain.

3.  **Refactor `analyse.cpp`:**
    *   Rewrite the `Compare...` and `Calculate...Metrics` functions to use the new `JsonUtils` class. This will simplify the code and make it more robust.

4.  **Merge Redundant Classes:**
    *   The `MonteCarloStatistics` and `ModelComparison` (the `Confidence` part) classes are very similar. They both perform a Monte Carlo simulation and store the results in a similar way. These two classes could be merged into a single, more general-purpose `MonteCarloAnalysis` class.
    *   The `ResampleAnalyse` class could be split into two separate classes: `CrossValidationAnalysis` and `ReductionAnalysis`, to better reflect their distinct purposes.

5.  **Update Documentation:**
    *   Update the `SUPRAFIT_JSON_FORMAT.md` and `POSTPROCESSING_ANALYSIS.md` files to reflect the new, simplified data structure and access methods.
    *   Create a new `REFACTORING_SUMMARY.md` file to document the changes that were made.

### 2.4. UI and Runtime Storage Optimization

1.  **`ModelDataHolder` Refactoring:**
    *   **Eliminate `m_models` list:** Remove the `m_models` list from `ModelDataHolder` and access the models directly from the `m_model_widgets` list. This will reduce data duplication and simplify the code.
    *   **Create a unified `saveWorkspace` function:** Create a single function in `ModelDataHolder` that returns a `QJsonObject` representing the entire workspace. This will eliminate the need for the separate `SaveCurrentModels` and `SaveModel` functions.
    *   **Refactor `Compare...` functions:** Create a single function that takes a list of `AbstractModel` pointers and performs the comparison, rather than exporting each model to a `QJsonObject` first.

2.  **`ProjectTree` Refactoring:**
    *   **Use smart pointers:** Replace the raw pointer `m_data_list` with a `QSharedPointer` to improve memory management.
    *   **Use signals and slots for updates:** Instead of calling `UpdateStructure` to refresh the entire tree, use signals and slots to notify the `ProjectTree` of specific changes (e.g., project added, model added). This will make the UI more responsive, especially with a large number of projects.

This refactoring plan will significantly improve the quality of the SupraFit codebase, making it more maintainable, extensible, and easier to understand.
