# Detailed Refactoring Plan for SupraFit JSON Handling

This document provides a detailed analysis of the redundancies in the SupraFit JSON handling and proposes a concrete plan for refactoring.

## 1. Redundancy Analysis

### 1.1. Redundant Parsing Logic in `mlfeatureextractor.cpp`

The `parseMLPipelineData` function in `mlfeatureextractor.cpp` contains a series of `if-else if` statements to handle three different JSON structure types. This leads to code duplication, especially in how the `post_fit_analysis` and `methods` objects are located.

**Code Snippets with Redundancy:**

*   **Type C (ML-Pipeline):**

    ```cpp
    if (root.contains("data") && root["data"].toObject().contains("raw")) {
        // ...
        if (rawData.contains("ml_pipeline")) {
            // ...
            if (root.contains("model_0")) {
                // ...
                for (int i = 0; i < fittedModels.size(); ++i) {
                    // ...
                    if (rootModel.contains("data") && rootModel["data"].toObject().contains("methods")) {
                        // *** Redundant logic to get methods object ***
                    }
                }
            }
        }
    }
    ```

*   **Type A (Standard SupraFit):**

    ```cpp
    else if (root.contains("model_0")) {
        // ...
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it.key().startsWith("model_")) {
                // *** Redundant logic to get model object ***
            }
        }
    }
    ```

*   **Type B (Direct Analysis):**

    ```cpp
    else if (root.contains("methods")) {
        // ...
        if (root.contains("data")) {
            // ...
            postFitAnalysis["methods"] = root["methods"]; // *** Direct access to methods ***
        }
    }
    ```

### 1.2. Redundant JSON Structures

The `SUPRAFIT_JSON_FORMAT.md` documentation clearly shows that the same statistical data can be found at different paths depending on the file type.

*   **Path A (Standard .suprafit/.json):** `root.model_X.post_fit_analysis.methods.METHOD_ID.results.PARAM_INDEX`
*   **Path B (Direct Analysis):** `root.methods.METHOD_ID.PARAM_INDEX`
*   **Path C (ML-Pipeline):** `root.data.raw.ml_pipeline.fitted_models[X].post_fit_analysis.methods.METHOD_ID.results.PARAM_INDEX`

This makes it difficult to write generic functions that can operate on all file types.

### 1.3. Redundant Data Access in `analyse.cpp`

The functions in `analyse.cpp` have to handle the different JSON structures, leading to code that is more complex than necessary.

**Example from `CompareCV`:**

```cpp
for (const auto& model : models) {
    QJsonObject statistics = model["data"].toObject()["methods"].toObject();
    // ...
}
```

This code assumes a specific structure and would fail on a different one.

## 2. Refactoring Proposals

### 2.1. New Helper Function: `getPostFitAnalysis`

**`mlfeatureextractor.h`:**

```cpp
private:
    // ...
    QJsonObject getPostFitAnalysis(const QJsonObject& object);
```

**`mlfeatureextractor.cpp`:**

```cpp
QJsonObject MLFeatureExtractor::getPostFitAnalysis(const QJsonObject& object)
{
    if (object.contains("post_fit_analysis")) {
        return object["post_fit_analysis"].toObject();
    } else if (object.contains("data") && object["data"].toObject().contains("methods")) {
        QJsonObject postFitAnalysis;
        postFitAnalysis["analysis_completed"] = true;
        postFitAnalysis["methods"] = object["data"].toObject()["methods"].toObject();
        return postFitAnalysis;
    } else if (object.contains("methods")) {
        QJsonObject postFitAnalysis;
        postFitAnalysis["analysis_completed"] = true;
        postFitAnalysis["methods"] = object["methods"].toObject();
        return postFitAnalysis;
    }
    return QJsonObject();
}
```

### 2.2. Refactored `parseMLPipelineData`

**`mlfeatureextractor.cpp` (simplified):**

```cpp
QJsonObject MLFeatureExtractor::parseMLPipelineData(const QString& filename)
{
    // ... (file loading logic)

    QJsonObject mlData;

    if (root.contains("data") && root["data"].toObject().contains("raw")) { // Type C
        // ... (extract ml_pipeline object)
        if (root.contains("model_0")) {
            for (int i = 0; i < fittedModels.size(); ++i) {
                QString modelKey = QString("model_%1").arg(i);
                if (root.contains(modelKey)) {
                    QJsonObject fittedModel = fittedModels[i].toObject();
                    QJsonObject rootModel = root[modelKey].toObject();
                    fittedModel["post_fit_analysis"] = getPostFitAnalysis(rootModel);
                    fittedModels[i] = fittedModel;
                }
            }
            mlData["fitted_models"] = fittedModels;
        }
    } else if (root.contains("model_0")) { // Type A
        // ... (create syntheticML object)
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it.key().startsWith("model_")) {
                QJsonObject model = it.value().toObject();
                model["post_fit_analysis"] = getPostFitAnalysis(model);
                fittedModels.append(model);
            }
        }
        syntheticML["fitted_models"] = fittedModels;
        mlData = syntheticML;
    } else if (root.contains("methods")) { // Type B
        // ... (create syntheticML object)
        model["post_fit_analysis"] = getPostFitAnalysis(root);
        fittedModels.append(model);
        syntheticML["fitted_models"] = fittedModels;
        mlData = syntheticML;
    } else if (root.contains("ml_pipeline")) {
        mlData = root["ml_pipeline"].toObject();
    } else if (root.contains("raw") && root["raw"].toObject().contains("ml_pipeline")) {
        mlData = root["raw"].toObject()["ml_pipeline"].toObject();
    }

    return mlData;
}
```

### 2.3. New Unified Function: `getStatisticalMethod`

**`mlfeatureextractor.h`:**

```cpp
public:
    // ...
    static QJsonObject getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method);
```

**`mlfeatureextractor.cpp`:**

```cpp
QJsonObject MLFeatureExtractor::getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method)
{
    QJsonObject postFitAnalysis = getPostFitAnalysis(modelObject);
    if (postFitAnalysis.isEmpty()) {
        return QJsonObject();
    }

    QJsonObject methods = postFitAnalysis["methods"].toObject();
    QString methodId = QString::number(method);

    if (methods.contains(methodId)) {
        QJsonObject methodData = methods[methodId].toObject();
        if (methodData.contains("results")) {
            return methodData["results"].toObject();
        } else {
            return methodData;
        }
    }

    return QJsonObject();
}
```

### 2.4. Refactored `analyse.cpp`

**`analyse.cpp` (example with `CompareCV`):**

```cpp
QString CompareCV(const QVector<QJsonObject> models, int cvtype, bool local, int cv_x)
{
    // ...
    for (const auto& model : models) {
        QJsonObject cvData = MLFeatureExtractor::getStatisticalMethod(model, SupraFit::Method::CrossValidation);
        if (cvData.isEmpty()) {
            continue;
        }
        // ... (process cvData)
    }
    // ...
}
```

### 2.5. Redundant Functions to Remove

After refactoring, the following legacy string-based functions in `analyse.cpp` could be deprecated and eventually removed:

*   `AnalyseReductionAnalysis`
*   `CompareAIC`
*   `CompareCV`
*   `CompareMC`

### 2.6. Documentation Updates

The `SUPRAFIT_JSON_FORMAT.md` file should be updated to:

*   Remove the different access paths for statistical data.
*   Document the new `getStatisticalMethod` function as the single point of access.
*   Emphasize the unified `post_fit_analysis` object.
