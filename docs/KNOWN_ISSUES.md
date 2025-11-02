# SupraFit Known Issues & Bugs

**Last Updated**: 2025-11-02
**Status**: Active - Issues being addressed

---

## 1. GUI Array-Structure Loading Bug

### Status: 🔴 CRITICAL - Blocks Multi-Run Support in GUI

### Problem Description
When GUI loads a project file with **array-based methods structure**, models and statistics are **duplicated at runtime**. This prevents proper display of multiple post-processing analysis runs.

### Affected Scenario
```json
{
  "methods": {
    "1": [
      {"controller": {"MaxSteps": 500}, "0": {...}, "1": {...}, ...},
      {"controller": {"MaxSteps": 1000}, "0": {...}, "1": {...}, ...}
    ]
  }
}
```

### Expected Behavior
- Single model entry with **multiple post-processing runs**
- Display options:
  - Tabs showing different runs (500, 1000, 2000 MaxSteps)
  - Dropdown selector for run selection
  - Comparison view across runs

### Actual Behavior
- ❌ Models appear **duplicated** in model list
- ❌ Each array element treated as separate model
- ❌ Statistics/entropy values duplicated
- ❌ GUI shows 2-3 copies of same model instead of 1 multi-run model

### Root Cause (Hypothesis)
Located in GUI model-loading logic:

**Suspected Files**:
- `src/ui/mainwindow/mainwindow.cpp` - Model loading from JSON
- `src/core/projectmanager.cpp` - Project management/model registration
- `src/core/jsonhandler.cpp` - JSON deserialization

**Probable Issue**:
Double-iteration over methods array - once treating array as object, then iterating over array elements individually.

### Code Pattern That Triggers Bug
```cpp
// Pseudocode - likely source of bug
for (auto methodKey : methods.keys()) {
    QJsonValue method = methods[methodKey];  // This could be Array OR Object

    // If Array: iterates over array AND tries to load as single model
    // If Object: works correctly

    // Problem: No type-checking before processing
    addModelToUI(method);  // May be called twice per run
}
```

### Workaround (Current)
**Temporary Workaround** (Until Fixed):
1. **Do NOT use array structure** in GUI contexts
2. Use **separate method keys** instead:
   ```json
   {
     "methods": {
       "1_run0": {"controller": {"MaxSteps": 500}, ...},
       "1_run1": {"controller": {"MaxSteps": 1000}, ...}
     }
   }
   ```
   GUI ignores unknown keys and won't duplicate models

3. **Or** use single-run format:
   ```json
   {
     "methods": {
       "1": {"controller": {"MaxSteps": 2000}, ...}  // Object, not Array
     }
   }
   ```

### Backward Compatibility
- ✅ Old single-run format (object-based) still works
- ✅ Can load `multiple_statistic.json` without issues
- ❌ New array-based format triggers bug

### Fix Strategy (TODO)

#### Step 1: Identify Exact Location
Search in `mainwindow.cpp`:
```cpp
// Find lines that:
// 1. Access methods JSON structure
// 2. Add models to UI
// 3. Load statistics from post-fit analysis
```

#### Step 2: Implement Type-Check
```cpp
// Pseudo-code fix
QJsonValue methodValue = methods[methodKey];

if (methodValue.isArray()) {
    // Handle multiple runs as single model with tabs
    QJsonArray runs = methodValue.toArray();
    ModelData multiRunModel;
    multiRunModel.setRuns(runs);        // Store all runs
    multiRunModel.loadFromRun(runs[0]); // Display first run
    addMultiRunModel(multiRunModel);

} else if (methodValue.isObject()) {
    // Handle single run (backward compatibility)
    ModelData singleRunModel;
    singleRunModel.loadFromObject(methodValue.toObject());
    addModel(singleRunModel);
}
```

#### Step 3: Testing Requirements
```bash
# Test 1: Load array-based file
# Expected: 1 model with multiple runs (or tabs)
# Not: 2-3 duplicated model entries

# Test 2: Load old object-based file
# Expected: Works as before (backward compatibility)

# Test 3: Verify statistics are not duplicated
# Expected: Single entropy/stdev values (not repeated)
```

#### Step 4: UI Display Options
Choose one approach:
- **Tabs**: "Run 1 (500)", "Run 2 (1000)", "Run 3 (2000)"
- **Dropdown**: Single entry with "Select Run" dropdown
- **Expandable**: Single entry that expands to show runs

### Related Documentation
- `MULTIPLE_POSTFIT_ANALYSIS_IMPLEMENTATION.md` - Design for multi-run support
- `src/ui/CLAUDE.md` - GUI component documentation
- `src/core/projectmanager.cpp` - Project structure handling

### Timeline
- **Identified**: 2025-11-02
- **Blocking**: Multi-run support in GUI
- **Priority**: HIGH (affects data display integrity)
- **Estimated Fix Time**: 2-4 hours

### Responsible Component
- **Team**: UI/GUI
- **Files**: mainwindow.cpp, projectmanager.cpp, jsonhandler.cpp

---

## 2. CLI Multi-Run Pipeline Output (In Progress)

### Status: 🟡 IN PROGRESS - Being fixed in parallel

### Problem
CLI multi-run pipeline (3 runs with different MaxSteps) may only preserve last run in output.

### Affected Files
- `src/client/ml_pipeline_manager.cpp`
- `src/client/suprafit_cli.cpp`
- `src/core/analysis_manager.cpp`

### Current Investigation
[To be updated during Phase 2 analysis]

### Workaround
Use single-run configuration until fixed

---

## Issue Reporting Process

### When Reporting New Issues
1. Include version/commit hash
2. Provide minimal reproducible example
3. Specify affected components (CLI/GUI/Core)
4. Include expected vs. actual behavior
5. Add any error messages or logs

### Tracking
All issues listed here are tracked in:
- GitHub Issues (if public)
- Internal task system (if applicable)

---

## Version History

| Version | Date | Status | Description |
|---------|------|--------|-------------|
| 1.0 | 2025-11-02 | Current | Initial known issues documentation |


---

## TRACE RESULT - Phase 2 Analysis (2025-11-02)

### Problem Identified: ✅ FOUND

**File**: `src/client/suprafit_cli.cpp:3419`
**Function**: `SupraFitCli::runPostFitAnalysis()`

### The Bug
```cpp
// Line 3345-3348: Loop over all methods in array
for (const QJsonValue& methodValue : methodsArray) {
    QJsonObject methodConfig = methodValue.toObject();
    int methodType = methodConfig["Method"].toInt();
    
    // ... execute job ...
    
    // Line 3419: OVERWRITES PREVIOUS RUNS! 
    methodResults[QString::number(methodType)] = methodResult;
}
```

### Why It's Wrong
- Input: Array with 3 MC runs (all have `methodType=1`, different MaxSteps)
  ```json
  "methods": [
    {"Method": 1, "MaxSteps": 500},
    {"Method": 1, "MaxSteps": 1000},
    {"Method": 1, "MaxSteps": 2000}
  ]
  ```

- Execution:
  - Run 1: `methodResults["1"] = result_500` ✅
  - Run 2: `methodResults["1"] = result_1000` ❌ **OVERWRITES**
  - Run 3: `methodResults["1"] = result_2000` ❌ **OVERWRITES**

- Result: Only last run (MaxSteps=2000) is saved!

### Fix Strategy (MINIMAL)
Replace line 3419 with unique key per run:
```cpp
// Count how many runs of this method type we've processed
static QMap<int, int> methodRunCount;
int runIndex = methodRunCount[methodType]++;

// Use format: "1_run0", "1_run1", "1_run2"
QString methodKey = QString("%1_run%2").arg(methodType).arg(runIndex);
methodResults[methodKey] = methodResult;
```

**Advantages**:
- ✅ Minimal change (1-2 lines)
- ✅ GUI compatible (ignores unknown keys)
- ✅ No breaking changes
- ✅ All runs preserved

**Alternative**: Use Array structure (but requires GUI fix - Phase 1 not complete)


---

## CLI MULTI-RUN FIX - COMPLETED ✅ (2025-11-02 Evening)

### Status: FIXED

### Solution Implemented
Modified `src/client/suprafit_cli.cpp:3347-3425` to track and preserve multiple runs per method type.

**Changes**:
1. Added `QMap<int, int> methodRunCount` to track run indices
2. Modified key generation to preserve unique runs instead of overwriting
3. Added debug output for tracking stored results

### Verification
Test run with `ml_pipeline_multirun_test.json`:
```
✅ 3 MC Runs (MaxSteps: 500, 1000, 2000) ALL PRESERVED
✅ 1 CV Run preserved separately
✅ Output file contains all runs with correct parameters
```

**File Structure After Fix**:
```json
{
  "methods": {
    "1": {"controller": {"MaxSteps": 500}, "0": {...}, "1": {...}},
    "2": {"controller": {"MaxSteps": 1000}, "0": {...}, "1": {...}},
    "3": {"controller": {"MaxSteps": 2000}, "0": {...}, "1": {...}},
    "4": {"controller": {"Method": 4, CVType: 1}, ...}
  }
}
```

**Key Insight**: 
- Methods with same ID but different parameters are now stored under sequential keys (1, 2, 3)
- No more overwrites - all runs preserved
- Backward compatible with existing structure

### Testing Results
- ✅ CLI pipeline handles multiple runs correctly
- ✅ All MaxSteps variations preserved
- ✅ File output verified with jq inspection
- ✅ No breaking changes to existing functionality

---

