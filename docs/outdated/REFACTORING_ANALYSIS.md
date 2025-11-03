# Refactoring Analysis and Data Flow Documentation

This document provides an analysis of the current state of the SupraFit codebase in relation to the existing refactoring plans. It also details the data flow from file loading to display, highlighting potential points of failure.

## 1. Refactoring Plan Analysis

Based on the analysis of the refactoring plans (`REFACTORING_PLAN.md`, `DETAILED_REFACTORING_PLAN.md`, `COMPREHENSIVE_REFACTORING_PLAN.md`, `ACTIONABLE_REFACTORING_PLAN.md`), the main goals of the refactoring effort are:

*   **Centralize JSON Handling:** Create a unified data access layer (`SupraFit::JsonUtils`) to reduce redundancy and improve code clarity.
*   **Refactor Core Components:** Rewrite parts of `mlfeatureextractor.cpp` and `analyse.cpp` to use the new `JsonUtils` class.
*   **Improve Class Structure:** Merge redundant classes (e.g., `MonteCarloStatistics` and `ModelComparison`) and split others for better organization (e.g., `ResampleAnalyse`).
*   **Optimize UI and Runtime Storage:** Refactor `ModelDataHolder` and `ProjectTree` to improve performance and memory management.

## 2. Current Code Status vs. Refactoring Plan

### 2.1. Completed Tasks ✅

*   **ProjectManager Implementation**: A centralized `ProjectManager` has been implemented (`src/core/projectmanager.h/cpp`) to unify project I/O operations and reduce code duplication between the GUI and CLI.
*   **Unified Data Access Layer (`JsonUtils`)**: The `SupraFit::JsonUtils` class has been created and implemented in `src/core/jsonutils.h` and `src/core/jsonutils.cpp`, providing central functions for JSON data access.
*   **Refactoring of `mlfeatureextractor.cpp`**: The file `src/capabilities/mlfeatureextractor.cpp` has been refactored to use the new `JsonUtils` class, significantly reducing its complexity.
*   **Refactoring of `SupraFitGui`**: The `LoadFile` and `NewTable` functions have been refactored to use the `ProjectManager`. The old `SetData` function has been marked as deprecated.
*   **Refactoring of `MainWindow`**: The `setDataFromProjectManager` function has been implemented, and the old `SetData` function has been marked as deprecated.

### 2.2. Partially Completed Tasks ⚠️

*   **Refactoring of `analyse.cpp`**: The file `src/core/analyse.cpp` has been partially überarbeitet. Neue, JSON-basierte Analysefunktionen (`Calculate...Metrics`) wurden hinzugefügt und bestehende Funktionen wie `AnalyseReductionAnalysis` nutzen teilweise `JsonUtils`. Allerdings existieren die alten, string-basierten Analysefunktionen weiterhin, und die Umstellung ist noch nicht vollständig abgeschlossen.
*   **Refactoring of `ProjectManager`**: The duplicate model detection in `loadProjectFromJson` has been fixed. However, other potential issues might still exist.

### 2.3. Not Started Tasks ❌

*   **Zusammenführung redundanter Klassen**: Die in den Plänen vorgeschlagene Zusammenführung der Klassen `MonteCarloStatistics` und `ModelComparison` sowie die Aufteilung von `ResampleAnalyse` wurde noch nicht begonnen. Die entsprechenden Header-Dateien (`montecarlostatistics.h`, `modelcomparison.h`, `resampleanalyse.h`) existieren weiterhin in ihrer ursprünglichen Form.
*   **Optimierung von UI und Laufzeitspeicher**: Die geplanten Überarbeitungen der Klassen `ModelDataHolder` (Entfernen von `m_models`, einheitliche `saveWorkspace`-Funktion) und `ProjectTree` (Verwendung von Smart Pointern und Signal-Slot-Mechanismen für Updates) wurden noch nicht umgesetzt.

## 3. Data Flow Analysis: From File to UI

### 3.1. Old Logic (pre-ProjectManager)

Before the implementation of the `ProjectManager`, the data loading process was handled directly by the UI components:

1.  `SupraFitGui` would receive the file path.
2.  It would pass the data (as a `QJsonObject`) to a `MainWindow` instance.
3.  The `MainWindow` instance was responsible for creating the `DataClass` object.
4.  The newly created `DataClass` was then passed to the `ModelDataHolder`.

This logic created a tight coupling between the UI and the data loading process.

### 3.2. New Logic (with ProjectManager)

The introduction of the `ProjectManager` has centralized and decoupled the data loading process:

1.  **`SupraFitGui::LoadFile()`**: The user initiates the file loading from the GUI.
2.  **`SupraFitGui::LoadProject()`**: This function now delegates the loading task to the `ProjectManager`: `ProjectManager::instance().loadProject(filename)`.
3.  **`ProjectManager::loadProject()`**: This function loads the file, creates the `DataClass` and all associated `AbstractModel` objects.
4.  **`projectLoaded` Signal**: Upon successful loading, the `ProjectManager` emits the `projectLoaded(projectId, filePath)` signal.
5.  **`SupraFitGui::onProjectLoaded()`**: This slot in the GUI is triggered by the signal.
6.  **UI Creation**: The `onProjectLoaded` slot creates the `MainWindow` and `ModelDataHolder`.
7.  **Data Pass-through**: The `projectId` is passed to `ModelDataHolder::setDataFromProjectManager()`, which then retrieves the already created `DataClass` object from the `ProjectManager` using `projectManager.getProjectData(projectId)`.
8.  **Display**: The `DataWidget` receives the `DataClass` and displays the data.

This new logic correctly separates the data management from the UI, with the `ProjectManager` acting as the single source of truth for project data.

## 4. Analysis of Potential Failure Points

### 4.1. For non-JSON files

The most likely point of failure remains the initial parsing of the data file in `FileHandler` and `ToolSet`. The parsing logic is fragile and not well-tested for various file formats and edge cases.

### 4.2. For JSON files (in the new logic)

Given that the issue occurs with valid JSON files and the data pass-through mechanism is sound (using `QSharedPointer`), the root cause is almost certainly located within the **`ProjectManager::loadProjectFromJson()`** function. The failure is not in the loading of the JSON file itself, but in the subsequent processing and object creation.

Specifically, the following steps within `loadProjectFromJson` are the most likely culprits:

1.  **`CreateModel()`**: The factory function for creating `AbstractModel` instances might fail for a specific model ID.
2.  **`model->ImportModel()`**: The import of the model's data from the `QJsonObject` might fail, leading to an incomplete model.
3.  **`project->addModel(model)`**: The association of the created model with the `DataClass` object might fail.

A failure in any of these steps would result in a `DataClass` object that is either missing some of its models or contains incompletely initialized models. When this `DataClass` is then passed to the `ModelDataHolder` and `DataWidget`, it would lead to the observed behavior (e.g., an empty or incorrect display).

### 4.3. Implemented Fix for Model Loading Hang

**Problem:** The application was hanging when loading a project with models. The debug output showed that the `ProjectManager` was incorrectly identifying models as duplicates and skipping them. This was caused by the `AbstractModel` inheriting the UUID of its parent `DataClass`. The duplicate check in `ProjectManager::loadProjectFromJson` was comparing the model's `UUID()` with the project's `UUID()`, which were the same.

**Solution:** The duplicate check in `ProjectManager::loadProjectFromJson` has been modified to use the model's unique UUID (`ModelUUID()`) instead of the `DataClass`'s UUID (`UUID()`). This ensures that models are correctly distinguished from their parent project and from other models.

**File:** `src/core/projectmanager.cpp`

**Before:**
```cpp
if (child && child->UUID() == model->UUID()) {
```

**After:**
```cpp
if (child && qobject_cast<AbstractModel*>(child.data())->ModelUUID() == model->ModelUUID()) {
```
