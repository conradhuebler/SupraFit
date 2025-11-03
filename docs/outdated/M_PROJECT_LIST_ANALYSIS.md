# Analysis of `m_project_list` Usage

## 1. Introduction

This document provides a detailed analysis of the usage of the legacy data structure `m_project_list` within the SupraFit GUI. The analysis is based on a search for all occurrences of `m_project_list` in the codebase. The goal is to understand the scope of the refactoring required to complete the migration to the new `ProjectManager`-based architecture.

## 2. Analysis of Occurrences

A search for `m_project_list` revealed 31 occurrences, all of which are located in `src/ui/mainwindow/suprafitgui.h` and `src/ui/mainwindow/suprafitgui.cpp`. This confirms that the usage of this legacy data structure is confined to the main GUI class, but it is deeply integrated into its logic.

The usage can be categorized as follows:

### 2.1. Declaration

- **File:** `src/ui/mainwindow/suprafitgui.h`
- **Code:** `QVector<QPointer<MainWindow>> m_project_list;`
- **Analysis:** The list is declared as a member of the `SupraFitGui` class. It holds pointers to `MainWindow` instances, where each `MainWindow` represents an open project in the GUI.

### 2.2. Adding and Removing Projects

- **Files:** `src/ui/mainwindow/suprafitgui.cpp`
- **Analysis:** The list is directly manipulated when projects are added or removed from the GUI. This includes:
    - `m_project_list.insert(...)`: Used when setting data via the legacy `SetData` function.
    - `m_project_list.append(...)`: Used when adding a new meta model.
    - `m_project_list.takeAt(...)`: Used when a project is closed or removed by the user.

### 2.3. UI Actions and Interaction Logic

- **Files:** `src/ui/mainwindow/suprafitgui.cpp`
- **Analysis:** This is the most critical category, as it shows how deeply the UI logic is tied to the old data structure. Almost all user actions that target a specific project rely on `m_project_list`.
    - **Saving and Exporting:** Functions like `SaveProjectAction`, `SaveAsProjectAction`, and `ExportAllPlain` iterate through `m_project_list` to access the data of each open project.
    - **ProjectTree Interaction:** Functions like `TreeRemoveRequest`, `SaveData`, `Duplicate`, and `AddUpData` use the index of the selected item in the `ProjectTree` to directly access an element in `m_project_list`. **This is the primary cause of the current crashes**, as the tree is now populated by the `ProjectManager`, but the interaction logic still uses the old, unsynchronized list.
    - **Model Operations:** `CopyModel` also uses an index to access `m_project_list` to copy a model to the correct project.

### 2.4. Synchronization with `ProjectManager`

- **Files:** `src/ui/mainwindow/suprafitgui.cpp`
- **Analysis:** The presence of the `updateDataListFromProjectManager` function indicates an attempt to bridge the gap between the new and old systems. This function synchronizes the `m_project_list` with the projects loaded in the `ProjectManager`. While this is a useful temporary fix, it is not a long-term solution and adds complexity to the code.

## 3. Conclusion and Implementation Status

**✅ COMPLETED (January 2025):**

The `m_project_list` dependency has been completely eliminated from the SupraFit GUI. All recommended refactoring steps have been implemented:

1.  **✅ New mapping structure implemented:** `QHash<QString, QPointer<MainWindow>> m_project_windows;` successfully maps project UUIDs to MainWindow instances.

2.  **✅ UI action logic updated:** All functions (`SaveData`, `Duplicate`, `TreeRemoveRequest`, `AddUpData`, `CopyModel`, etc.) now use the UUID-based hash map instead of legacy list indices.

3.  **✅ Legacy code eliminated:** The `m_project_list` member variable has been removed from the header file, and all array access patterns have been migrated to the new architecture.

**Key fixes implemented:**
- **Critical missing function call:** `setDataFromProjectManager()` is now properly called in `onProjectAdded()` slot
- **Complete ProjectManager integration:** Projects loaded via ProjectManager are now visible in the GUI
- **Crash prevention:** Tree interactions now use synchronized ProjectManager data instead of unsynchronized legacy indices
- **Clean architecture:** Single UUID-based system throughout the GUI

**Result:** The application now has a robust, unified architecture where the ProjectManager serves as the single source of truth for all project data, eliminating the previous fragile half-migrated state.