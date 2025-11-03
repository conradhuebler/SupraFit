# Actionable Refactoring Plan for SupraFit

This document provides a detailed, actionable plan for refactoring the SupraFit codebase. It includes specific code snippets and line numbers to guide the implementation.

## 0. ProjectManager Implementation (Claude Generated - COMPLETED ✅)

**Status:** Done (January 2025)

**Action:** Create centralized project management system to eliminate CLI-GUI code duplication.

**File:** `src/core/projectmanager.h/cpp` (implemented)

**Summary:** 
- Thread-safe singleton ProjectManager with unified project I/O operations
- Eliminated separate project lists in SupraFitGui (m_data_list) and SupraFitCli (m_data_vector) 
- UUID-based project identification and caching system
- Signal-based notifications for GUI Model-View integration
- Phase 1 of CLI-GUI consolidation roadmap completed

**Next Phase:** TaskController for unified job execution (Task #2 in CLI_UI_CONSOLIDATION_PLAN.md)

## 1. Create a Unified Data Access Layer

**Status:** Done

**Action:** Create a new class, `SupraFit::JsonUtils`, to provide a single, unified interface for accessing data from the SupraFit JSON structure.

**File:** `src/core/jsonutils.h` (new file)

```cpp
#pragma once

#include "src/global.h"

#include <QtCore/QJsonObject>
#include <QtCore/QVector>

namespace SupraFit {

class JsonUtils {
public:
    static QJsonObject getPostFitAnalysis(const QJsonObject& object);
    static QJsonObject getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method);
    static QVector<qreal> getParameterDistribution(const QJsonObject& paramObject);
};

}
```

**File:** `src/core/jsonutils.cpp` (new file)

```cpp
#include "jsonutils.h"

#include "src/core/toolset.h"

namespace SupraFit {

QJsonObject JsonUtils::getPostFitAnalysis(const QJsonObject& object)
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

QJsonObject JsonUtils::getStatisticalMethod(const QJsonObject& modelObject, SupraFit::Method method)
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

QVector<qreal> JsonUtils::getParameterDistribution(const QJsonObject& paramObject)
{
    if (paramObject.contains("data") && paramObject["data"].toObject().contains("raw")) {
        QString rawDataString = paramObject["data"].toObject()["raw"].toString();
        return ToolSet::String2DoubleVec(rawDataString);
    }
    return QVector<qreal>();
}

}
```

## 2. Refactor `mlfeatureextractor.cpp`

**Status:** Done

**Action:** Rewrite the `parseMLPipelineData` function to use the new `JsonUtils` class.

**File:** `src/capabilities/mlfeatureextractor.cpp`

**Line numbers:** ~70-200

**Before:**

```cpp
// ... complex if-else if chain ...
```

**After:**

```cpp
QJsonObject MLFeatureExtractor::parseMLPipelineData(const QString& filename)
{
    // ... (file loading logic)

    QJsonObject mlData;

    if (root.contains("data") && root["data"].toObject().contains("raw")) { // Type C
        QJsonObject rawData = root["data"].toObject()["raw"].toObject();
        if (rawData.contains("ml_pipeline")) {
            mlData = rawData["ml_pipeline"].toObject();
            if (root.contains("model_0")) {
                QJsonArray fittedModels = mlData["fitted_models"].toArray();
                for (int i = 0; i < fittedModels.size(); ++i) {
                    QString modelKey = QString("model_%1").arg(i);
                    if (root.contains(modelKey)) {
                        QJsonObject fittedModel = fittedModels[i].toObject();
                        QJsonObject rootModel = root[modelKey].toObject();
                        fittedModel["post_fit_analysis"] = SupraFit::JsonUtils::getPostFitAnalysis(rootModel);
                        fittedModels[i] = fittedModel;
                    }
                }
                mlData["fitted_models"] = fittedModels;
            }
        }
    } else if (root.contains("model_0")) { // Type A
        QJsonObject syntheticML;
        QJsonArray fittedModels;
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it.key().startsWith("model_")) {
                QJsonObject model = it.value().toObject();
                model["post_fit_analysis"] = SupraFit::JsonUtils::getPostFitAnalysis(model);
                fittedModels.append(model);
            }
        }
        syntheticML["fitted_models"] = fittedModels;
        mlData = syntheticML;
    } else if (root.contains("methods")) { // Type B
        QJsonObject syntheticML;
        QJsonArray fittedModels;
        QJsonObject model;
        model["post_fit_analysis"] = SupraFit::JsonUtils::getPostFitAnalysis(root);
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

## 3. Refactor `analyse.cpp`

**Status:** Partially Done

**Action:** Rewrite the `Compare...` and `Calculate...Metrics` functions to use the new `JsonUtils` class.

**File:** `src/core/analyse.cpp`

**Line numbers:** ~200-800

**Completed:**

*   The `AnalyseReductionAnalysis`, `CompareCV`, and `CompareMC` functions have been refactored to use `SupraFit::JsonUtils::getPostFitAnalysis`.

**To Do:**

*   Implement the `getStatisticalMethod` and `getParameterDistribution` functions in `JsonUtils`.
*   Refactor the `Compare...` and `Calculate...Metrics` functions to use the new `getStatisticalMethod` and `getParameterDistribution` functions.

## 4. Alignment with the ML Integration Plan

**Status:** In Progress

**Action:** Align the refactoring plan with the ML integration plan.

**Completed:**

*   The creation of the `JsonUtils` class is a key step in supporting the `FeaturePreprocessor`.
*   The refactoring of `mlfeatureextractor.cpp` is a prerequisite for enhancing it for the ML pipeline.

**To Do:**

*   Complete the refactoring of the data access layer and the UI to provide a solid foundation for the ML integration.

## 5. Merge Redundant Classes

**Status:** Not Started

**Action:** Merge the `MonteCarloStatistics` and `ModelComparison` (the `Confidence` part) classes into a single `MonteCarloAnalysis` class.

**Files:**

*   `src/capabilities/montecarlostatistics.h`
*   `src/capabilities/montecarlostatistics.cpp`
*   `src/capabilities/modelcomparison.h`
*   `src/capabilities/modelcomparison.cpp`

**Action:** Split the `ResampleAnalyse` class into two separate classes: `CrossValidationAnalysis` and `ReductionAnalysis`.

**Files:**

*   `src/capabilities/resampleanalyse.h`
*   `src/capabilities/resampleanalyse.cpp`

## 6. UI and Runtime Storage Optimization

**Status:** Not Started

### 6.1. `ModelDataHolder` Refactoring

**Action:** Eliminate the `m_models` list from `ModelDataHolder`.

**File:** `src/ui/mainwindow/modeldataholder.h`

**Line number:** ~130

**Before:**

```cpp
QVector<QWeakPointer<AbstractModel>> m_models;
```

**After:**

```cpp
// This list is no longer needed.
```

**Action:** Create a unified `saveWorkspace` function.

**File:** `src/ui/mainwindow/modeldataholder.cpp`

**Line numbers:** ~250-280

**Before:**

```cpp
// ... separate SaveCurrentModels and SaveModel functions ...
```

**After:**

```cpp
QJsonObject ModelDataHolder::SaveWorkspace()
{
    QJsonObject toplevel, data;
    data = m_data.toStrongRef()->ExportData();

    if (m_datawidget) {
        QJsonArray modelsArray;
        for (int i = 0; i < m_model_widgets.size(); ++i) {
            ModelWidget* model = m_model_widgets[i];
            QJsonObject obj = model->Model()->ExportModel();
            obj["colors"] = model->Chart().signal_wrapper->ColorList();
            obj["keys"] = model->Keys();
            modelsArray.append(obj);
        }
        data["models"] = modelsArray;
        toplevel["data"] = data;
    } else {
        toplevel["data"] = m_metamodelwidget->Model()->ExportModel(true, false);
    }
    return toplevel;
}
```

### 6.2. `ProjectTree` Refactoring

**Action:** Use a `QSharedPointer` for `m_data_list`.

**File:** `src/ui/mainwindow/projecttree.h`

**Line number:** ~60

**Before:**

```cpp
QVector<QWeakPointer<DataClass>>* m_data_list;
```

**After:**

```cpp
QSharedPointer<QVector<QWeakPointer<DataClass>>> m_data_list;
```

**Action:** Use signals and slots for updates.

**File:** `src/ui/mainwindow/projecttree.cpp`

**Line number:** ~80

**Before:**

```cpp
void ProjectTree::UpdateStructure()
{
    // ... iterates through all projects and models ...
}
```

**After:**

*   Connect to `projectAdded` and `modelAdded` signals from the `ProjectHandler` (or a similar class).
*   Implement slots to handle these signals and update only the affected parts of the tree.
