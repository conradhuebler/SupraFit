# Archived Documentation

This directory contains historical planning documents and implementation reports. They are no longer actively maintained but are preserved for reference.

**Note:** The status descriptions in these documents are often inaccurate or overly optimistic. This README has been updated based on a code audit to reflect the actual implementation status.

## Contents Overview

### Refactoring Plans (7 files)
A series of refactoring initiatives from 2024-2025.

- **REFACTORING_PLAN.md** - Initial refactoring strategy
- **DETAILED_REFACTORING_PLAN.md** - Detailed implementation plan
- **COMPREHENSIVE_REFACTORING_PLAN.md** - Comprehensive strategy overview
- **REFACTORING_ANALYSIS.md** - Analysis of refactoring requirements
- **REFACTORING_SUMMARY.md** - Summary of refactoring work
- **REFACTORING_PLAN_TYPE_C.md** - Type C (`ml_pipeline`) model elimination plan
- **ACTIONABLE_REFACTORING_PLAN.md** - Actionable task list for refactoring

**Original Status**: ✅ All refactoring work completed and integrated into codebase
**Corrected Status**: ⚠️ **Partially Implemented.**
- ✅ The `JsonUtils` class was created and is used in `mlfeatureextractor.cpp` and partially in `analyse.cpp`.
- ⚠️ The "Type C" (`ml_pipeline`) refactoring is incomplete. The new multi-project architecture exists, but the old `ml_pipeline` functionality was not removed as planned.
- ❌ The planned merging/splitting of analysis classes (e.g., `MonteCarloStatistics`, `ResampleAnalyse`) was **not** implemented.
- ❌ The planned UI optimizations (`ModelDataHolder`, `ProjectTree`) were **not** implemented.

### Implementation Reports (3 files)
Multi-run post-fit analysis implementation (November 2025).

- **IMPLEMENTATION_COMPLETE.md** - Implementation completion report
- **IMPLEMENTATION_FINAL.md** - Final implementation summary
- **CLEAN_MULTIRUN_IMPLEMENTATION_SUMMARY.md** - Clean summary with key features

**Original Status**: ✅ Feature fully integrated into codebase
**Corrected Status**: ✅ **Implemented, but with conflicting documentation.** The feature is integrated, but the documents contradict each other regarding the implementation details. The code confirms that the new, nested JSON structure was used, making `IMPLEMENTATION_FINAL.md` an inaccurate description of the final state.

### ProjectManager Migration (3 files)
ProjectManager architecture and migration documentation (January 2025).

- **PROJECT_MANAGER_ARCHITECTURE.md** - ProjectManager design and architecture
- **PROJECTMANAGER_MIGRATION_ISSUES.md** - Migration challenges and resolutions
- **M_PROJECT_LIST_ANALYSIS.md** - Analysis of legacy m_project_list replacement

**Original Status**: ✅ Migration completed, ProjectManager now primary architecture component
**Corrected Status**: ✅ **Largely Completed.** The GUI migration to the `ProjectManager` appears to be complete and functional, which was the primary goal of this effort.

### Statistical Standardization (4 files)
JSON standardization and statistical implementation (2024-2025).

- **CONRAD_ORIGINAL_JSON_STATISTICAL_STRUCTURE.md** - Original implementation reference
- **REFERENCE_IMPLEMENTATION_FLOW.md** - Statistical processing flow documentation
- **CLAUDE_DEVIATIONS_AUDIT.md** - Implementation deviation audit
- **STANDARDIZATION_PROGRESS_REPORT.md** - Standardization completion report

**Original Status**: ✅ All 8 statistical functions standardized
**Corrected Status**: ✅ **Completed.** The audit of `analyse.cpp` confirms that the `Calculate...Metrics` functions were successfully refactored to produce the standardized JSON structure, resolving the identified deviations.

## Why Archive?

These documents represent **past planning** and **historical context**. The actual code implementation is the source of truth. These documents are preserved for:
- Understanding development history.
- Referencing past design decisions.
- Contextualizing why certain architectural patterns were chosen.

---

**Last Updated**: 2025-11-03 (by Gemini Assistant based on code audit)