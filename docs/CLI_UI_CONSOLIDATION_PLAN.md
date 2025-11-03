# Refactoring Plan: Consolidating CLI and UI Core Logic

**Objective:** To refactor the SupraFit application to create a single, unified "Core" layer that contains all shared business logic, eliminating code duplication between the CLI and GUI frontends. This will make the application more robust, maintainable, and easier to extend.

**Guiding Principles:**
*   The `src/core` and `src/capabilities` directories together form the application's core logic.
*   The `src/client` (CLI) and `src/ui` (GUI) directories should be treated as thin clients that consume the core logic.
*   Functionality that is currently duplicated in both the CLI and GUI should be moved into a shared component within the core.

---

## Phase 1: Create a `ProjectManager` for Centralized Project Handling ✅ **COMPLETED**

- **Status:** ✅ **Done (January 2025)** - Claude Generated Implementation
- **Problem:** Both `SupraFitGui` and `SupraFitCli` have their own logic for loading, saving, and managing projects (collections of data and models). This is redundant.
- **Solution:** Create a new singleton or application-level class responsible for all project-related operations.
- **Tasks:**
    1.  ✅ **Create `ProjectManager` class** in `src/core/` - **IMPLEMENTED**
    2.  ✅ **Move Project I/O Logic** - **COMPLETED**
        -   Migrated the logic from `SupraFitGui::LoadProject()`, `SupraFitGui::SaveProjectAction()`, and `SupraFitCli::LoadFile()`, `SupraFitCli::SaveFile()` into the `ProjectManager`.
        -   The `ProjectManager` has methods like `loadProject()`, `saveProject()`, `getCurrentProject()`, `addModelToProject()`, `getProjectData()`, `getLoadedProjectIds()`, etc.
    3.  ✅ **Centralize Project State** - **COMPLETED**
        -   The `ProjectManager` owns the list of open `DataClass` and `AbstractModel` objects, replacing the separate lists in `SupraFitGui` (`m_data_list`) and `SupraFitCli` (`m_data_vector`).
        -   Thread-safe singleton with UUID-based project identification and caching system
    4.  ✅ **Refactor Frontends** - **COMPLETED**
        -   Modified `SupraFitGui` and `SupraFitCli` to use the `ProjectManager` for all file operations and project data access
        -   Fixed API integration issues and corrected method calls
        -   Signal-based notifications for GUI Model-View integration
- **Breaking Change:** This significant architectural change has been successfully implemented. It simplifies the frontends and improves the overall architecture.

---

## Phase 2: Consolidate Job and Task Execution

- **Problem:** The logic for initiating tasks like model fitting and statistical analysis is duplicated. The UI uses `ModelWidget` to trigger a `JobManager`, while the CLI has its own `JobManager` instance and triggering logic in `SupraFitCli::Work()` and `PerformeJobs()`.
- **Solution:** Create a unified, non-UI component for executing tasks on models.
- **Tasks:**
    1.  **Create a `TaskController` class** in `src/core/`.
    2.  **Abstract Task Execution:**
        -   The `TaskController` will take a model (`AbstractModel*`) and a job configuration (`QJsonObject`) as input.
        -   It will encapsulate the `JobManager` and the `Minimizer`.
        -   It will have public methods like `runFitting(AbstractModel*)` and `runStatisticalAnalysis(AbstractModel*, const QJsonObject& jobConfig)`.
    3.  **Refactor Frontends:**
        -   **`ModelWidget` (UI):** The "Fit" and "Statistics" buttons will now call methods on the `TaskController` instead of having their own `Minimizer` and `JobManager`. It will observe the `TaskController` for progress and completion signals.
        -   **`SupraFitCli` (CLI):** The `Work()` and `PerformeJobs()` methods will be simplified to loop through models and call the appropriate methods on the `TaskController`.
- **Justification:** This ensures that both the CLI and GUI execute tasks in the exact same way, using the same underlying logic, which reduces bugs and improves consistency.

---

## Phase 3: Unify Data Generation Logic

- **Problem:** The `DataGenerator` is already a shared capability, but the logic for invoking it and handling its output is specific to each frontend (`SupraFitCli::GenerateData()` vs. UI dialogs).
- **Solution:** Create a high-level interface in the core for data generation tasks.
- **Tasks:**
    1.  **Enhance the `ProjectManager` or create a `DataFactory` class** in `src/core/`.
    2.  **Create a `generateProject(const QJsonObject& config)` method:**
        -   This method will take a JSON configuration object (like those used in the CLI).
        -   It will use the `DataGenerator` to create the data.
        -   It will then create a new `Project` object (from Phase 1) containing the generated data.
    3.  **Refactor Frontends:**
        -   The `ImportData` dialog in the UI will call this new core method to generate data.
        -   The `SupraFitCli::GenerateData()` method will become a simple wrapper around this new core method.
- **Justification:** This provides a single, reliable way to generate new projects, whether initiated from the CLI or the GUI, ensuring consistency.