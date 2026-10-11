# Test Plan: File Import and JSON Generation Test

## Executive Summary
This document outlines the plan for creating a test that validates the complete processing pipeline from a raw NMR data file (1_1_1_2_001.dat) to a reference JSON output containing 4 fitted models.

**Test Complexity**: Medium-High
**Implementation Scope**: 200-300 lines of code
**Integration**: src/tests/test_file_import_reference.cpp

---

## File Descriptions and Locations

### Input File
**Location**: `/home/conrad/src/SupraFit/input/1_1_1_2_001.dat`
**Format**: Tab-separated ASCII text (20 rows of data)
**Size**: 1.5 KB
**Structure**:
```
[concentration] [time] [dependent_value_1] [dependent_value_2] ... [dependent_value_7]
0.00100009      0      6.81556             6.27456             ...  3.18752
0.00100009      0.00013264  6.75827        6.24798             ...  3.20285
...
```

**Interpretation**:
- Column 1: Independent variable (concentration, "[G₀]/[H₀]")
- Column 2: Time parameter (first column reference)
- Columns 3-9: 7 dependent measured values (likely NMR chemical shift changes)
- Temperature: 298K (system parameter 4)

**Scientific Context**:
This is NMR titration data for a 1:1 binding equilibrium where the host (receptor) is titrated by a guest (ligand) with an additional 1:2 (complexes 1:1 and 1:2) competitive binding pattern.

### Expected Output File
**Location**: `/home/conrad/src/SupraFit/input/Reference_4Models.json`
**Format**: JSON (suprafit project format)
**Size**: 24 MB
**Structure**:
```json
{
  "data": {
    "DataType": 1,
    "title": "1_1_1_2_001",
    "system": {
      "4": "298",           // Temperature (K)
      "1024": "[G₀]/[H₀]",  // Independent variable label
      "1025": "First column" // Reference column
    },
    "dependent": {
      "cols": 7,
      "data": { "0": "...", "1": "...", ... "19": "..." },
      "checked": { "0": "1 1 1 1 1 1 1", ... "19": "1 1 1 1 1 1 1" }
    },
    "independent": { ... },
    "independent_raw": { ... },
    "dependent_raw": { ... }
  },
  "model_1": {
    "SSE": <double>,
    "SAE": <double>,
    "AIC": <double>,
    "AICc": <double>,
    "model": "nmr_1_1_1_2",
    "name": "¹H 1:1-1:2-Modell",
    "data": { /* model-specific data */ }
  },
  "model_2": { ... },  // 4 models total
  "model_3": { ... },
  "model_4": { ... }
}
```

**Key Components**:
1. **data section**: Raw imported data with metadata
2. **model_1 to model_4**: 4 fitted models with statistics
3. **Metadata**: System parameters, axis labels, data types

---

## Processing Logic Analysis

### Current Architecture

The processing involves these key SupraFit components:

**1. FileHandler (`src/core/filehandler.cpp`)**
- Responsible for reading raw .dat files
- Detects file type (Generic for tab-separated data)
- Parses data into matrix structure
- Supports row/column range selection

**2. DataClass (`src/core/models/dataclass.cpp`)**
- Container for a project (data + metadata)
- Stores independent and dependent tables
- Contains system parameters
- Manages UUID and JSON serialization

**3. Model System (`src/core/models/titrations/nmr/`)**
- nmr_1_1_1_2_Model: 1:1 and 1:2 competitive binding model
- Other models: fl_1_1_1_2, uv_vis_1_1_1_2, etc.
- Base: AbstractTitrationModel → AbstractModel

**4. Minimizer (`src/core/minimizer.cpp`)**
- Parameter optimization engine
- Least-squares fitting (Levenberg-Marquardt)
- Convergence detection and statistics

**5. Analysis (`src/core/analyse.cpp`)**
- Post-fit statistical analysis
- AIC, SSE, SAE, parameter uncertainty
- Monte Carlo and other methods

**6. ProjectManager (`src/core/projectmanager.cpp`)**
- Centralized project management
- Project persistence via JSON
- UUID-based identification

---

## What the Test Should Do

### Primary Validation (Core Test)
1. **File Import**: Load 1_1_1_2_001.dat and verify data structure
   - Verify 20 rows × 9 columns (1 conc + 1 time + 7 dependent values)
   - Check first and last data points
   - Validate numerical precision

2. **Data Structure Generation**: Create proper JSON representation
   - Create DataClass with independent/dependent tables
   - Generate system parameters (298K, axis labels)
   - Verify JSON serialization format

3. **Model Fitting**: Fit 4 models (or use pre-computed reference)
   - nmr_1_1_1_2_Model (NMR 1:1-1:2 competitive binding)
   - 3 alternative models for comparison
   - Generate AIC, SSE, SAE statistics

4. **Reference Validation**: Compare against Reference_4Models.json
   - Statistical match (SSE, AIC within tolerance)
   - Model parameter comparison
   - Data integrity verification

### Secondary Validations
- File format detection (Generic ASCII type)
- Data range correctness
- Numerical stability (no NaN/Inf)
- JSON schema compliance
- Model convergence status

---

## Assertions and Validations Needed

### Data Import Assertions
```cpp
// File loading
QVERIFY(dataPoints.size() == 20);
QCOMPARE(dataPoints[0].cols(), 9);

// Numerical validation
QCOMPARE(dataPoints[0][0], 0.00100009);    // Concentration
QCOMPARE(dataPoints[0][2], 6.81556);       // First dependent value
QVERIFY(std::isfinite(dataPoints[19][8])); // No NaN/Inf

// Data structure
QCOMPARE(dataClass->IndependentTable()->cols(), 2);      // conc + time
QCOMPARE(dataClass->DependentTable()->cols(), 7);        // 7 measured values
QCOMPARE(dataClass->DependentTable()->rows(), 20);
```

### Model Fitting Assertions
```cpp
// Model configuration
QCOMPARE(model->GlobalParameterSize(), 2);  // K11, K12
QCOMPARE(model->LocalParameterSize(), 7);   // Chemical shifts

// Convergence
QVERIFY(model->converged());
QVERIFY(model->SSE() > 0);
QVERIFY(model->SSE() < 1000.0);  // Sanity check

// Statistics
QVERIFY(model->AIC() > -1000);
QVERIFY(model->AICc() >= model->AIC());  // AICc >= AIC always

// Comparison with reference
QCOMPARE_CUSTOM(model->SSE(), referenceMoel.SSE(), tolerance);
QCOMPARE_CUSTOM(model->AIC(), referenceModel.AIC(), 0.1);
```

### JSON Output Assertions
```cpp
// Structure
QVERIFY(outputJson.contains("data"));
QVERIFY(outputJson.contains("model_1"));
QVERIFY(outputJson.contains("model_2"));
QVERIFY(outputJson.contains("model_3"));
QVERIFY(outputJson.contains("model_4"));

// Data section
QJsonObject data = outputJson["data"].toObject();
QCOMPARE(data["DataType"].toInt(), 1);
QCOMPARE(data["title"].toString(), "1_1_1_2_001");
QVERIFY(data.contains("dependent"));
QVERIFY(data.contains("independent"));

// Model sections
for (int i = 1; i <= 4; ++i) {
    QString key = QString("model_%1").arg(i);
    QJsonObject model = outputJson[key].toObject();
    QVERIFY(model.contains("SSE"));
    QVERIFY(model.contains("AIC"));
    QVERIFY(model.contains("name"));
}
```

### Reference Matching
```cpp
// Load reference JSON
QJsonDocument refDoc = loadJsonFile("input/Reference_4Models.json");
QJsonObject reference = refDoc.object();

// Compare data sections
compareJsonData(generatedJson["data"], reference["data"], tolerance);

// Compare model statistics (with tolerance for numerical differences)
for (int i = 1; i <= 4; ++i) {
    compareModelStatistics(
        generatedJson[QString("model_%1").arg(i)].toObject(),
        reference[QString("model_%1").arg(i)].toObject(),
        0.01  // 1% tolerance for SSE/AIC
    );
}
```

---

## Test Type Classification

### PRIMARY: Data Import and Processing Test
- **Category**: Integration test combining FileHandler + DataClass + Analysis
- **Scope**: Complete pipeline from raw file to fitted models
- **Validates**: File I/O → Data structure → Model fitting → JSON output

### SECONDARY: Reference Validation Test
- **Category**: Regression test against known good output
- **Scope**: Output accuracy and reproducibility
- **Validates**: Deterministic behavior and numerical stability

### This is NOT:
- A simple CLI command test (requires actual model fitting)
- A pure file format conversion test (involves computation)
- A model-specific test (multi-model comparison)

---

## Integration into Existing Test Suite

### Location
`/home/conrad/src/SupraFit/src/tests/test_file_import_reference.cpp`

### Test Class Structure
```cpp
class TestFileImportReference : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();           // Setup
    void cleanupTestCase();        // Teardown
    
    // Core tests
    void testDatFileLoading();
    void testDataStructureCreation();
    void testModelFitting();
    void testJsonGeneration();
    void testReferenceMatching();
    
    // Supporting tests
    void testDataNumericalAccuracy();
    void testSystemParameterHandling();
    void testModelConvergence();
    void testJsonSchema();
    
private:
    DataClass* createDataFromFile(const QString& filename);
    QJsonObject generateProjectJson(DataClass* data);
    QJsonObject loadReferenceJson();
    double calculateTolerance(const QString& modelKey);
};
```

### CMakeLists.txt Integration
```cmake
add_executable(test_file_import_reference
    src/tests/test_file_import_reference.cpp
    src/tests/test_utils.cpp
)

target_link_libraries(test_file_import_reference
    Qt6::Core Qt6::Test
    libcore libmodels
    fmt
)

add_test(NAME test_file_import_reference
         COMMAND test_file_import_reference)
```

### Test Execution
```bash
# Run this test specifically
cd /home/conrad/src/SupraFit/build/debug
./src/tests/test_file_import_reference

# Run with verbose output
./src/tests/test_file_import_reference -v

# Run within full test suite
make test
```

---

## Implementation Scope and Effort

### Estimated Size
- **Main test file**: 250-350 lines
- **Helper utilities**: 50-100 lines (if new)
- **CMakeLists.txt update**: 10-15 lines
- **Total**: ~320-465 lines of code

### Implementation Phases

**Phase 1: Foundation (50 lines)**
- Setup test class structure
- FileHandler integration
- Basic data import validation
- Expected: 1-2 hours

**Phase 2: Data Verification (80 lines)**
- DataClass creation from file
- Table structure validation
- Numerical accuracy checks
- Expected: 2-3 hours

**Phase 3: Model Fitting (100 lines)**
- Model instantiation (4 different models)
- Parameter initialization
- Minimization and convergence
- Statistics extraction
- Expected: 3-5 hours

**Phase 4: JSON Generation and Validation (70 lines)**
- Project JSON serialization
- Schema validation
- Reference file loading and comparison
- Expected: 2-3 hours

**Phase 5: Integration and Polish (20 lines)**
- CMakeLists.txt integration
- Error handling and edge cases
- Documentation
- Expected: 1-2 hours

**Total Estimated Effort**: 9-15 hours of implementation

---

## Dependencies and Prerequisites

### Required Classes/Functions
1. **FileHandler** - `LoadFile()`, data access methods
2. **DataClass** - constructor, table setters, JSON export
3. **AddModel()** function - CLI model creation
4. **Minimizer** - Parameter optimization
5. **analyse.cpp** - Statistical analysis functions
6. **JsonHandler** - JSON serialization

### Required Headers
```cpp
#include "src/core/filehandler.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"
#include "src/core/jsonhandler.h"
#include "src/core/analyse.h"

#include <QtTest/QTest>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
```

### External Dependencies
- **Qt6**: Core, Test modules
- **input/1_1_1_2_001.dat**: Test data file (exists)
- **input/Reference_4Models.json**: Reference output (exists)

### Build Prerequisites
- ✅ libcore.a compiled
- ✅ libmodels.a compiled
- ✅ Qt6 Test framework available
- ✅ Input files present in expected location

---

## Potential Challenges and Mitigations

### Challenge 1: Numerical Precision in Model Fitting
**Problem**: Different builds/systems may produce slightly different SSE/AIC values
**Solution**: Use relative tolerance (1-5%) instead of exact matching

### Challenge 2: Model Convergence Uncertainty
**Problem**: Some models may converge to different local minima
**Solution**: Test convergence (bool converged()) rather than specific parameter values

### Challenge 3: Large Reference File (24 MB)
**Problem**: Loading entire JSON in memory for comparison
**Solution**: Selective JSON parsing or partial comparison of key sections

### Challenge 4: Model-Specific Initialization
**Problem**: Each model type (nmr_1_1_1_2, etc.) has different parameter requirements
**Solution**: Use helper function with model-specific initialization logic

### Challenge 5: Test Execution Time
**Problem**: 4 model fits could take 10-30 seconds per test run
**Solution**: Cache results or use lighter-weight validation (SSE-only vs full stats)

---

## Success Criteria

### Minimal Success (MVP)
- ✅ File loads without errors
- ✅ Data structure contains expected dimensions (20×9)
- ✅ At least 1 model fits successfully
- ✅ JSON output validates against schema
- **Expected time**: ~5 hours

### Full Success
- ✅ All criteria from MVP
- ✅ 4 models fit with convergence
- ✅ Statistics match reference within 1-5% tolerance
- ✅ All data fields populated correctly
- ✅ Test runs in <10 seconds
- **Expected time**: 12-15 hours

### Extended Success
- ✅ All criteria from Full Success
- ✅ Parametric test variants (different models)
- ✅ Performance benchmarking
- ✅ Edge case handling (missing data, invalid ranges)
- **Expected time**: 18-20 hours

---

## Recommended Next Steps

1. **Quick Validation** (30 minutes)
   - Try loading the .dat file via FileHandler in isolation
   - Print dimensions and first/last values
   - Verify file is readable

2. **Prototype** (2-3 hours)
   - Create basic test class structure
   - Implement Phase 1 & 2
   - Get basic assertions passing

3. **Model Fitting** (3-5 hours)
   - Implement Phase 3
   - Test with simplest model first
   - Debug convergence if needed

4. **Validation & Integration** (3-5 hours)
   - Implement Phase 4 & 5
   - Compare against reference
   - Integrate into CMakeLists.txt

5. **Polish & Documentation** (1-2 hours)
   - Error handling
   - Test documentation
   - Performance optimization if needed

---

## Related Documentation

- `/home/conrad/src/SupraFit/CLAUDE.md` - Project standards and guidelines
- `/home/conrad/src/SupraFit/src/core/CLAUDE.md` - Core component documentation
- `/home/conrad/src/SupraFit/src/tests/CLAUDE.md` - Test infrastructure documentation
- `/home/conrad/src/SupraFit/src/core/models/MODEL_ID_REFERENCE.md` - Model IDs and specifications
- `/home/conrad/src/SupraFit/src/capabilities/STATISTICAL_POSTPROCESSING.md` - Analysis methods

