# UI Improvement Plan based on User Feedback

This document outlines a refactoring and improvement plan for the SupraFit user interface, based on the annotated screenshot provided by the user.

## 1. Analysis of User Feedback

The user's feedback, provided as annotations on a screenshot of the application, can be summarized as follows:

1.  **Toolbar/Menu Consolidation:** The user suggests replacing the cluttered toolbars with a more organized and space-efficient menu bar. The proposed structure is hierarchical, grouping related actions under `Data`, `Model`, and `Evaluation` menus.
2.  **Project/Log View:** The user wants to move the message log from the bottom of the window to a dedicated panel on the left, likely tabbed with the project view.
3.  **Model Widget Refinements:**
    - The user wants to make the parameter fields read-only after a fit has converged to prevent accidental changes.
    - The user suggests a clearer and more structured presentation of the local parameters, distinguishing between "Datenreihen" (data series) and "Ergebnisse des Fits" (results of the fit).

## 2. Refactoring and Improvement Plan

### Phase 1: Toolbar and Menu Refactoring (High Impact, Low Risk)

- **Goal:** De-clutter the main window by replacing the extensive toolbars with a structured menu bar, as suggested.
- **Tasks:**
    1.  **Create a new `MenuManager` class:** This class will be responsible for dynamically building the main menu bar.
    2.  **Implement the hierarchical menu structure:**
        - Create a `Data` menu with "Edit" and "Split" actions.
        - Create a `Model` menu with a nested `Experiment` submenu.
        - The `Experiment` submenu will contain further submenus for `NMR`, `ITC`, `UV/VIS`, `Fluorescence`, each containing actions to add the corresponding models.
        - Create an `Evaluation` menu with "Statistics", "Analyse", and "Compare" actions.
    3.  **Remove old toolbar buttons:** Decommission the individual `QPushButton` and `QToolButton` widgets from `MDHDockTitleBar` and `SupraFitGui` that are now represented in the menu.
    4.  **Keep essential toolbar actions:** Retain a minimal set of frequently used actions in a smaller, more focused toolbar (e.g., New, Open, Save, Optimize All).
- **Breaking Change:** This is a significant UI change. Users accustomed to the old toolbar will need to adapt to the new menu structure. However, it's a positive change for usability and screen real estate.

### Phase 2: Log and Project View Consolidation (Medium Impact, Low Risk)

- **Goal:** Improve the visibility and accessibility of the application log by moving it from the status bar to a dedicated panel.
- **Tasks:**
    1.  **Modify the left-hand dock widget:** Change the existing project view container to a `QTabWidget`.
    2.  **Move the `ProjectTree`:** Place the existing `m_project_view` inside the first tab of the new `QTabWidget`, labeled "Projects".
    3.  **Create and integrate a `LogWidget`:**
        - Create a new `LogWidget` class (e.g., a `QPlainTextEdit` or `QTextBrowser`).
        - Place this new widget in the second tab of the `QTabWidget`, labeled "Log".
    4.  **Redirect log messages:**
        - Reroute the `Message`, `Warning`, and `Info` signals from `SupraFitGui` and other classes to append text to the new `LogWidget` instead of the `MessageDock` in the status bar.
        - Remove the old `MessageDock` from the status bar.
- **Justification:** This makes the log persistent and easier to read and copy from, which is crucial for debugging and analysis.

### Phase 3: `ModelWidget` User Experience Enhancements (Medium Impact, Medium Risk)

- **Goal:** Implement the specific UI/UX improvements suggested in the annotations for the model fitting view.
- **Tasks:**
    1.  **Implement Read-Only Mode for Converged Fits:**
        - In `ModelWidget::Repaint()` or a similar update slot, check the `m_model->isConverged()` status.
        - If `true`, set all parameter spin boxes (`m_constants` and those in `m_model_elements`) to be read-only (`setReadOnly(true)`).
        - If `false`, ensure they are set to `setReadOnly(false)`.
        - Visually indicate the read-only state (e.g., with a different background color or by disabling the widgets).
    2.  **Improve Local Parameter Display:**
        - This is a more significant change. Instead of a simple grid of spin boxes in `ModelElement`, consider creating a more structured view.
        - **Proposal:** Refactor `ModelElement` to use a `QTableView` with a custom `QAbstractTableModel`.
        - The model would have columns for "Parameter Name", "Value", "Std. Error", etc.
        - Each row would represent a parameter for a specific data series ("Datenreihen"). This would provide a much clearer view of the "Ergebnisse des Fits" (fit results) for each series.
- **Breaking Change:** Changing the local parameter display from a grid of spin boxes to a table view is a significant internal change to `ModelElement` and `ModelWidget`, but it should not break the external API. It will, however, change the user experience.

### Phase 4: Documentation Update

- **Goal:** Update all relevant documentation to reflect the UI changes.
- **Tasks:**
    1.  Update `docs/UI_REFACTORING_PLAN.md` with this new, more detailed plan.
    2.  Update `src/client/usage_example.md` with new screenshots and instructions that reflect the new menu structure and UI layout.
    3.  Update any other user-facing documentation that describes the UI.
