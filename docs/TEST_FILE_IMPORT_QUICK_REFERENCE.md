# Test File Import - Quick Reference

## Files at a Glance

| Item | Location | Size | Format | Status |
|------|----------|------|--------|--------|
| **Input Data** | `/home/conrad/src/SupraFit/input/1_1_1_2_001.dat` | 1.5 KB | Tab-separated ASCII | ✅ Exists |
| **Reference Output** | `/home/conrad/src/SupraFit/input/Reference_4Models.json` | 24 MB | JSON (project) | ✅ Exists |
| **Test File** | `src/tests/test_file_import_reference.cpp` | TBD | Qt Test | To create |
| **Documentation** | `docs/TEST_FILE_IMPORT_REFERENCE_PLAN.md` | 14.5 KB | Markdown | ✅ Created |

## Input File Structure

### 1_1_1_2_001.dat
```
[9 columns] x [20 rows]
Col 1: Concentration (0.00100009)
Col 2: Time parameter (0 to ~0.004)
Cols 3-9: 7 dependent measured values (chemical shifts)
```

**Key metadata**:
- Title: "1_1_1_2_001"
- Temperature: 298K
- Independent label: "[G₀]/[H₀]"
- Data type: NMR titration (type 1)

## Expected Output JSON Structure

```
Reference_4Models.json
├── data (raw imported data + metadata)
│   ├── DataType: 1
│   ├── title: "1_1_1_2_001"
│   ├── system: { "4": "298", "1024": "[G₀]/[H₀]", ... }
│   ├── dependent: { cols: 7, data: {...}, checked: {...} }
│   ├── independent: {...}
│   ├── independent_raw: {...}
│   └── dependent_raw: {...}
├── model_1 (NMR 1:1-1:2 model)
│   ├── SSE: <sum of squared errors>
│   ├── AIC: <Akaike information criterion>
│   ├── AICc: <corrected AIC>
│   ├── SAE: <sum of absolute errors>
│   └── ... (model-specific data)
├── model_2 (alternative model)
├── model_3 (alternative model)
└── model_4 (alternative model)
```

## Processing Pipeline

```
1_1_1_2_001.dat
    ↓
[FileHandler::LoadFile()]
    ↓
DataTable (9 cols x 20 rows)
    ↓
[DataClass::setIndependentTable() + setDependentTable()]
    ↓
DataClass with metadata (system params, title, etc.)
    ↓
[AddModel() × 4 models]
    ↓
4 × AbstractModel (each with 2 global + 7 local params)
    ↓
[Model::Calculate() + Minimizer]
    ↓
4 fitted models with SSE/AIC/etc.
    ↓
[JsonHandler::ToJson()]
    ↓
Reference_4Models.json
```

## Test Implementation Checklist

### Phase 1: Foundation
- [ ] Create test class `TestFileImportReference`
- [ ] Implement `initTestCase()` and `cleanupTestCase()`
- [ ] Add include guards and headers
- [ ] Compile without errors

### Phase 2: Data Import
- [ ] Implement `testDatFileLoading()`
- [ ] Verify file exists and readable
- [ ] Check dimensions (20 rows × 9 cols)
- [ ] Validate first/last numerical values
- [ ] Test data range correctness

### Phase 3: Data Structure
- [ ] Implement `testDataStructureCreation()`
- [ ] Create DataClass from loaded data
- [ ] Set independent/dependent tables (cols: 2 and 7)
- [ ] Add system parameters (298K, axis labels)
- [ ] Verify JSON serialization format

### Phase 4: Model Fitting
- [ ] Implement `testModelFitting()`
- [ ] Instantiate 4 models (NMR 1:1-1:2 + 3 alternatives)
- [ ] Initialize parameters
- [ ] Run model calculations
- [ ] Verify convergence
- [ ] Extract SSE/AIC/SAE statistics

### Phase 5: Validation
- [ ] Implement `testReferenceMatching()`
- [ ] Load Reference_4Models.json
- [ ] Compare data sections
- [ ] Compare model statistics (with tolerance)
- [ ] Validate JSON schema
- [ ] Check for NaN/Inf values

### Phase 6: Integration
- [ ] Update CMakeLists.txt
- [ ] Link required libraries (libcore, libmodels, Qt6::Test)
- [ ] Add test to CTest registry
- [ ] Verify execution with `make test`

## Key Assertions to Implement

```cpp
// Data dimensions
QCOMPARE(fileData->rows(), 20);
QCOMPARE(fileData->cols(), 9);

// Numerical accuracy
QCOMPARE_CUSTOM(fileData[0][0], 0.00100009, 1e-8);
QCOMPARE_CUSTOM(fileData[0][2], 6.81556, 1e-5);

// Model convergence
QVERIFY(model->converged());
QVERIFY(model->GlobalParameterSize() == 2);
QVERIFY(model->LocalParameterSize() == 7);

// Statistics sanity checks
QVERIFY(model->SSE() > 0 && model->SSE() < 1000);
QVERIFY(model->AICc() >= model->AIC());

// Reference matching (with 1-5% tolerance)
QCOMPARE_CUSTOM(genSSE, refSSE, relTol);
QCOMPARE_CUSTOM(genAIC, refAIC, relTol);

// JSON structure
QVERIFY(output.contains("data"));
QVERIFY(output.contains("model_1"));
QCOMPARE(output["data"]["DataType"].toInt(), 1);
QCOMPARE(output["data"]["title"].toString(), "1_1_1_2_001");
```

## Critical Files to Reference

- `src/core/filehandler.cpp/h` - File I/O
- `src/core/models/dataclass.cpp/h` - Project container
- `src/core/models/titrations/nmr/nmr_1_1_1_2_Model.cpp/h` - Model implementation
- `src/core/models/models.h` - Model factory
- `src/client/suprafit_cli.cpp` - Example usage patterns
- `src/tests/test_utils.cpp/h` - Test infrastructure

## Expected Execution Time

| Phase | Time | Notes |
|-------|------|-------|
| Foundation | 1-2 h | Basic setup |
| Data Import | 1-2 h | FileHandler integration |
| Data Structure | 1-2 h | DataClass verification |
| Model Fitting | 3-5 h | Most complex phase |
| Validation | 1-3 h | Reference comparison |
| Integration | 1-2 h | CMakeLists, build, polish |
| **Total** | **9-15 h** | Depends on prior codebase knowledge |

## Tolerance Parameters

| Metric | Tolerance | Rationale |
|--------|-----------|-----------|
| SSE (Sum Squared Error) | ±5% | Numerical precision differences |
| AIC (Akaike Criterion) | ±0.1 | Model selection robustness |
| SAE (Sum Absolute Error) | ±3% | Less sensitive than SSE |
| Parameters | ±1% | Convergence variation |
| Data values | ±1e-5 | Float precision |

## Common Pitfalls to Avoid

1. **Hardcoded Paths**: Use relative paths from build directory
2. **Model-Specific Parameters**: Each model type needs specific initialization
3. **Numerical Comparison**: Use approximation functions, not exact equality
4. **Convergence Assumptions**: Some models may not converge; check status flag
5. **Memory Management**: Use QPointer for safety
6. **Large File Loading**: 24 MB JSON may cause memory issues; consider selective parsing

## Quick Start

```bash
# 1. Navigate to build directory
cd /home/conrad/src/SupraFit/build/debug

# 2. Verify input files exist
ls -la ../../input/1_1_1_2_001.dat
ls -la ../../input/Reference_4Models.json

# 3. Build the test
make test_file_import_reference

# 4. Run the test
./src/tests/test_file_import_reference -v

# 5. Run all tests
make test
```

## References

- Full Plan: `docs/TEST_FILE_IMPORT_REFERENCE_PLAN.md`
- Project Standards: `CLAUDE.md`
- Model Reference: `src/core/models/MODEL_ID_REFERENCE.md`
- Test Framework: `src/tests/CLAUDE.md`
- CLI Examples: `src/client/usage_example.md`

---

**Last Updated**: 2025-11-04  
**Document Status**: Complete - Ready for Implementation  
**Next Action**: Begin Phase 1 (Foundation) implementation
