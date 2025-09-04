# Refactoring and Improvement Plan for `suprafit_cli` and Related Classes

This document outlines the necessary changes to refactor and improve the code quality of the `suprafit_cli` and its related classes. The goal is to create a more readable, maintainable, robust, and performant command-line interface for SupraFit.

## Phase 1: Code Cleanup and Removal of Dead Code

- **Goal:** Simplify the codebase by removing unused and redundant code.
- **Tasks:**
    1.  **Remove the `Analyser` class:**
        - Delete `src/client/analyser.h` and `src/client/analyser.cpp`.
        - Remove any includes or references to the `Analyser` class in other files.
        - **Breaking Change:** This is a breaking change if any external code is using the `Analyser` class. However, since the class is empty, this is unlikely.
    2.  **Remove the `Simulator` class:**
        - Delete `src/client/simulator.h` and `src/client/simulator.cpp`.
        - Remove any includes or references to the `Simulator` class in other files.
        - **Breaking Change:** This is a breaking change if any external code is using the `Simulator` class. The `PerformeJobs` method in `Simulator` is very similar to the one in `SupraFitCli`, so any external code that uses the `Simulator` class can be updated to use the `SupraFitCli` class instead.
- **Justification:** Removing dead code reduces the size of the codebase, making it easier to understand and maintain.

## Phase 2: Refactoring of the `SupraFitCli` Class

- **Goal:** Improve the readability, maintainability, and robustness of the `SupraFitCli` class.
- **Tasks:**
    1.  **Refactor `setControlJson()`:**
        - Create separate private methods for parsing each section of the input JSON (e.g., `parseMainSection()`, `parseModelsSection()`, `parseJobsSection()`, etc.).
        - Use a data-driven approach to parse the "Tasks" section (e.g., using a `QHash<QString, bool&>`).
        - **Breaking Change:** This is an internal refactoring and should not affect the public API of the `SupraFitCli` class.
    2.  **Improve Error Handling in `LoadFile()`:**
        - Add a `try-catch` block to handle JSON parsing errors.
        - Provide more specific error messages to the user.
        - **Breaking Change:** This change might affect the error messages that are displayed to the user, but it will not break the public API of the `SupraFitCli` class.
    3.  **Simplify `GenerateData()`:**
        - Create a helper function that takes a `DataGenerator` object and a set of parameters and returns a `DataClass` object.
        - Use more descriptive variable names.
        - **Breaking Change:** This is an internal refactoring and should not affect the public API of the `SupraFitCli` class.
    4.  **Refactor `AnalyzeFile()`:**
        - Create separate private methods for analyzing each section of the input file.
        - Separate the analysis logic from the presentation logic. The analysis functions should return a data structure that contains the results of the analysis, and then a separate function should be used to format the results for display.
        - **Breaking Change:** This is an internal refactoring and should not affect the public API of the `SupraFitCli` class.
    5.  **Use Smart Pointers:**
        - Replace raw pointers with smart pointers (e.g., `QSharedPointer`, `QScopedPointer`) where appropriate.
        - **Breaking Change:** This change might require changes to the code that uses the `SupraFitCli` class, but it will make the code safer and more robust.
- **Justification:** These changes will make the `SupraFitCli` class shorter, simpler, more modular, and easier to understand and maintain.

## Phase 3: Improvement of the `MLPipelineManager` Class

- **Goal:** Improve the readability and maintainability of the `MLPipelineManager` class.
- **Tasks:**
    1.  **Add Comments:**
        - Add comments to the `ml_pipeline_manager.h` and `ml_pipeline_manager.cpp` files to explain the purpose of each function and class.
    2.  **Improve Error Handling:**
        - Add more specific error messages to the `runSinglePipeline()` and `onPipelineError()` functions.
- **Justification:** These changes will make the `MLPipelineManager` class easier to understand and use.

## Phase 4: Addition of Unit Tests

- **Goal:** Improve the robustness of the code and prevent regressions.
- **Tasks:**
    1.  **Add Unit Tests for `SupraFitCli`:**
        - Add unit tests for the `setControlJson()`, `LoadFile()`, `GenerateData()`, and `AnalyzeFile()` functions.
    2.  **Add Unit Tests for `MLPipelineManager`:**
        - Add unit tests for the `runBatchPipeline()` and `runSinglePipeline()` functions.
- **Justification:** Unit tests are essential for ensuring the quality of the code and for preventing regressions.
