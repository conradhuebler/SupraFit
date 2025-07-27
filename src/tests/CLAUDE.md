# Tests - SupraFit Test Suite

## Overview
Comprehensive test suite for SupraFit functionality using Qt Test framework. Ensures reliability and correctness of core components, data generation, and analytical models.

## Test Structure

### Core Functionality Tests
- **test_simple.cpp**: Basic functionality validation (4/4 tests pass ✅)
- **test_datatable_simple.cpp**: DataTable basics (4/4 tests pass ✅)
- **test_datatable.cpp**: Comprehensive DataTable testing (18/25 tests pass)
- **test_dataclass.cpp**: DataClass functionality (crashes in one test)

### Advanced Component Tests
- **test_datagenerator.cpp**: DataGenerator comprehensive testing (9/9 tests pass ✅)
- **test_pipeline.cpp**: ML pipeline integration tests (30/35 tests pass)

## Test Categories

### DataGenerator Testing (Claude Generated)
Comprehensive validation of equation-based and model-based data generation:

```cpp
// Basic functionality tests
void testBasicEvaluation();
void testEquationParsing();
void testParameterInjection();

// Random parameter generation tests  
void testRandomParameterGeneration();
void testLegacyMatrixStringParsing();
void testModernParameterFormat();

// Integration tests
void testModelBasedGeneration();
void testConfigurationValidation();
void testMemoryManagement();
```

### DataTable Testing
Matrix operations, data validation, and performance:
- ✅ Basic matrix operations
- ✅ Data import/export
- ✅ Statistical calculations
- ⚠️ Some edge cases with large datasets
- ⚠️ Memory management in complex scenarios

### DataClass Testing
Project-level functionality and integration:
- ✅ Data loading and validation
- ✅ Model association
- ✅ JSON serialization
- ❌ One crash test (investigation needed)

### Pipeline Testing
End-to-end workflow validation:
- ✅ Data generation pipelines
- ✅ Model fitting workflows
- ✅ Statistical analysis
- ⚠️ Some ML integration edge cases

## Test Execution

### Individual Test Runs
```bash
cd /home/conrad/src/SupraFit/build/debug

# Core functionality
./src/tests/test_simple
./src/tests/test_datatable_simple

# Comprehensive testing
./src/tests/test_datatable
./src/tests/test_dataclass
./src/tests/test_datagenerator
./src/tests/test_pipeline
```

### Automated Test Suite
```bash
# Run all tests
make run_tests

# Specific test execution via script
./src/tests/run_tests.sh
```

## Test Results Summary

### ✅ Fully Passing
- **test_simple**: 4/4 - Basic functionality validation
- **test_datatable_simple**: 4/4 - Core DataTable operations  
- **test_datagenerator**: 9/9 - Complete DataGenerator functionality

### ⚠️ Mostly Passing
- **test_datatable**: 18/25 - Core functionality working, edge cases failing
- **test_pipeline**: 30/35 - Main workflows functional, some ML edge cases

### ❌ Issues Identified
- **test_dataclass**: Crashes in one test case (memory management issue)

## Testing Infrastructure

### Qt Test Framework
- Automated test discovery and execution
- Detailed failure reporting and diagnostics
- Performance benchmarking capabilities
- Memory leak detection integration

### Test Data Management
- Sample configuration files in `/input/` directory
- Generated test data cleanup
- Deterministic random seeding for reproducible results

### CI/CD Integration
- CMake test target integration
- Automated build verification
- Test result reporting

## Current Testing Focus

### DataGenerator Validation (Claude Generated)
Extensive testing of recent enhancements:
- ✅ Random parameter injection via JavaScript
- ✅ Model-based dependent data generation
- ✅ Legacy matrix-string format parsing
- ✅ Enhanced content creation with input configuration storage
- ✅ Memory safety with QPointer usage

### Regression Testing
- Model calculation accuracy
- File I/O consistency
- JSON serialization integrity
- Memory management stability

## Dependencies
- **Qt6**: Test framework
- **Core libraries**: All SupraFit components
- **Test data**: Sample configurations and datasets

## Usage Guidelines

### Running Tests During Development
```bash
# Quick validation
make test_simple test_datatable_simple test_datagenerator

# Full regression testing
make run_tests

# Individual test debugging
./src/tests/test_dataclass --verbose
```

### Adding New Tests
1. Create test file in `src/tests/`
2. Include in `CMakeLists.txt`
3. Follow Qt Test framework patterns
4. Document test purpose and scope

---

## Variable Section (Short-term information, regularly updated)

### Recent Test Updates
- ✅ 2025-01-27: DataGenerator test suite comprehensive (9/9 passing)
- ✅ 2025-01-27: Modular structure integration tests added
- ✅ 2025-01-27: Memory safety validation with JSON data transfer

### Current Test Status
- **Build**: ✅ All tests compile without errors
- **Core Functionality**: ✅ Basic operations stable
- **DataGenerator**: ✅ All advanced features validated
- **Integration**: ✅ CLI and pipeline workflows functional

### Known Test Issues
- **test_dataclass**: One crash test needs investigation (likely memory management)
- **test_datatable**: 7 edge case failures (large datasets, complex operations)
- **test_pipeline**: 5 ML integration edge cases

### Testing Priorities
1. Investigate and fix test_dataclass crash
2. Address test_datatable edge case failures
3. Improve test_pipeline ML integration robustness
4. Add more modular structure integration tests

### Performance Benchmarks
- DataGenerator: ~1ms per equation evaluation
- DataTable operations: Sub-millisecond for typical datasets
- Model calculations: Scales linearly with data points
- File I/O: Optimized for large datasets

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks
<!-- Add tasks here as needed by operator/programmer -->

### Vision
<!-- Add long-term architectural goals here -->