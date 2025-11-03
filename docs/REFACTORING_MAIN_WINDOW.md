# Refactoring and Optimization Plan for SupraFit Main Window

This document outlines the necessary changes to refactor and optimize the code for the main window of SupraFit. The goal is to create a more readable, maintainable, robust, and performant user interface.

## Phase 1: Code Cleanup and Removal of Dead Code

- **Goal:** Simplify the codebase by removing unused and redundant code.
- **Tasks:**
    1.  **Remove the `m_model_toolbar` and `m_system_toolbar` members:** These toolbars are created but never added to the main window. They are dead code and should be removed.
    2.  **Remove the `m_hasData` member:** This member is not used anywhere in the code and should be removed.
    3.  **Remove the `m_logfile` and `m_printlevel` members:** These members are not used anywhere in the code and should be removed.
    4.  **Remove the `LogFile()` function:** This function is not used anywhere in the code and should be removed.
    5.  **Remove the `EditData()` slot:** This slot is commented out and is not used anywhere in the code. It should be removed.
- **Justification:** Removing dead code reduces the size of the codebase, making it easier to understand and maintain.

## Phase 2: Refactoring of the `SupraFitGui` Class

- **Goal:** Improve the readability, maintainability, and robustness of the `SupraFitGui` class.
- **Tasks:**
    1.  **Break the `SupraFitGui` class down into smaller, more manageable classes:**
        - Create a `ProjectManager` class that is responsible for managing the list of open projects.
        - Create a `ViewManager` class that is responsible for managing the different views (e.g., the project tree, the chart widget, etc.).
        - Create a `MenuManager` class that is responsible for creating and managing the menus and toolbars.
    2.  **Use a more data-driven approach to creating the actions:**
        - Instead of creating each action individually, you could use a data structure (e.g., a `QList` of `QJsonObject`s) to define the actions and then create them in a loop.
    3.  **Improve the error handling:**
        - Add more specific error messages to the `LoadFile()` and `LoadProject()` functions.
        - Use a `try-catch` block to handle JSON parsing errors.
    4.  **Use smart pointers:**
        - Replace raw pointers with smart pointers (e.g., `QSharedPointer`, `QScopedPointer`) where appropriate.
- **Justification:** These changes will make the `SupraFitGui` class shorter, simpler, more modular, and easier to understand and maintain.

## Phase 3: Refactoring of the `MainWindow` Class

- **Goal:** Improve the readability, maintainability, and robustness of the `MainWindow` class.
- **Tasks:**
    1.  **Remove the `m_meta_model` member:** This member is only used to check if the current model is a meta model. This can be done by checking the model's type directly.
    2.  **Use a more descriptive name for the `SetData()` function:** For example, you could call it `loadProject()`.
    3.  **Add more comments to the code:** The code is sparsely commented, which makes it difficult to understand.
- **Justification:** These changes will make the `MainWindow` class easier to understand and maintain.

## Phase 4: Performance Optimization

- **Goal:** Improve the performance of the main window, especially when loading large projects.
- **Tasks:**
    1.  **Use a more efficient data structure for storing the projects:** Instead of using a `QVector` of `QPointer`s, you could use a `QHash` or a `QMap` to store the projects. This would make it faster to look up a project by its UUID.
    2.  **Load the data on demand:** Instead of loading all of the data for a project when it is opened, you could load the data on demand when it is needed. This would make the application more responsive, especially when opening large projects.
    3.  **Use a more efficient way to update the views:** Instead of updating the entire view when the data changes, you could only update the parts of the view that have changed.
- **Justification:** These changes will make the main window more responsive and improve the user experience.

## Phase 5: Refactoring of `ModelWidget` and `ModelDataHolder`

- **Goal:** To improve the modularity, reusability, and maintainability of the `ModelWidget` and `ModelDataHolder` classes.
- **Tasks:**
    1.  **Refactor `ModelDataHolder`:**
        - Create a `WidgetFactory` class that is responsible for creating the `DataWidget`, `MetaModelWidget`, and `ModelWidget` instances. This will decouple the `ModelDataHolder` from the concrete widget classes.
        - Use the command pattern to handle user actions. Instead of connecting the slots of the `ModelDataHolder` directly to the `MDHDockTitleBar`, you could use the command pattern to encapsulate each user action as an object. This will make the code more modular and easier to test.
        - Move the `Compare...()` functions to a separate `ModelComparer` class.
    2.  **Refactor `ModelWidget`:**
        - Break the `ModelWidget` class down into smaller, more manageable classes:
            - Create a `ParameterView` class that is responsible for displaying the model parameters.
            - Create a `ResultsView` class that is responsible for displaying the model results.
            - Create a `MinimizerController` class that is responsible for managing the minimization process.
        - Use a more data-driven approach to creating the UI. Instead of creating each UI element individually, you could use a data structure (e.g., a `QList` of `QJsonObject`s) to define the UI and then create it in a loop.
        - Improve the error handling in the `MinimizeModel()` function.
        - Use smart pointers to manage the lifetime of objects.
    3.  **Add Unit Tests:**
        - Add unit tests for the `ModelDataHolder` and `ModelWidget` classes.
- **Justification:** These changes will make the `ModelWidget` and `ModelDataHolder` classes more modular, reusable, and easier to understand and maintain.
