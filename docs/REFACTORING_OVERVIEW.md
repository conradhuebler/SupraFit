# SupraFit Refactoring Overview

This document provides a high-level overview of the refactoring efforts for the SupraFit project. It serves as a central point of reference for the various detailed refactoring plans.

## 1. Refactoring Plans

This section provides a summary of each of the individual refactoring plans, with a link to the full plan.

### 1.1. CLI and UI Consolidation

- **Goal:** To create a single, unified "Core" layer that contains all shared business logic, eliminating code duplication between the CLI and GUI frontends.
- **Status:** ✅ **Phase 1 Complete** - ProjectManager implemented (January 2025)
- **Progress:** 
  - ✅ Phase 1: ProjectManager centralized system - **COMPLETED**
  - ❌ Phase 2: TaskController for unified job execution - **PENDING**  
  - ❌ Phase 3: Data Generation unification - **PENDING**
- **Details:** [CLI and UI Consolidation Plan](./CLI_UI_CONSOLIDATION_PLAN.md)

### 1.2. Main Window Refactoring

- **Goal:** To refactor and optimize the code for the main window of SupraFit, creating a more readable, maintainable, robust, and performant user interface.
- **Status:** In Progress
- **Details:** [Refactoring and Optimization Plan for SupraFit Main Window](./REFACTORING_MAIN_WINDOW.md)

### 1.3. Scripted Models Refactoring

- **Goal:** To improve the implementation of scripted models in SupraFit.
- **Status:** In Progress
- **Details:** [Refactoring Plan for Scripted Models](./REFACTORING_SCRIPTED_MODELS.md)

### 1.4. Dialogs Refactoring

- **Goal:** To improve the dialogs in SupraFit.
- **Status:** In Progress
- **Details:** [Refactoring and Improvement Plan for SupraFit Dialogs](./REFACTORING_DIALOGS.md)

### 1.5. CLI Refactoring

- **Goal:** To improve the code quality of the `suprafit_cli` and its related classes.
- **Status:** In Progress
- **Details:** [Refactoring and Improvement Plan for `suprafit_cli` and Related Classes](./REFACTORING_CLI.md)

## 2. JSON Documentation Consolidation

- **Goal:** To create a single, authoritative source of documentation for the SupraFit JSON data structure.
- **Status:** Completed
- **Details:** The content of `input/NMR_JSON_Structure_Documentation.md` has been merged into `JSON_datastruture.md`, and the former has been deleted.

## 3. README Files Consolidation

- **Goal:** To create a single, authoritative README file for the project.
- **Status:** In Progress
- **Details:** The content of the various README files will be merged into the root `README.md` file.

## 4. `CLAUDE.md` Files Review and Refinement

- **Goal:** To ensure that each `CLAUDE.md` file contains relevant, up-to-date, and non-redundant information for its specific context.
- **Status:** In Progress
- **Details:** Each `CLAUDE.md` file will be reviewed individually to remove boilerplate text and ensure context-specificity.
