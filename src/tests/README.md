# SupraFit Test Suite

This directory contains comprehensive tests for the SupraFit application, focusing on data loading, saving, and ML pipeline functionality.

## Test Structure

### 1. DataTable Tests (`test_datatable.cpp`)

Tests for the core `DataTable` class functionality:

- **Basic Operations**: Creation, dimension handling, data access/modification
- **JSON Export/Import**: Serialization and deserialization of table data
- **File I/O**: Loading and saving table data to/from files
- **Header Management**: Setting and retrieving column headers
- **Checked Data**: Row/column checking functionality
- **Edge Cases**: Empty tables, large datasets, invalid data handling
- **Memory Management**: Proper cleanup and resource management

### 2. DataClass Tests (`test_dataclass.cpp`)

Tests for the `DataClass` (Project) functionality:

- **Project Creation**: Empty and populated project creation
- **Data Types**: Table, Thermogram, Spectrum, Simulation types
- **Table Management**: Independent/dependent table handling
- **System Parameters**: Parameter addition, retrieval, and persistence
- **Project Export/Import**: Full project serialization
- **File Operations**: Project loading and saving
- **Metadata**: UUID generation, project titles, content
- **Data Filtering**: Range settings and checked state handling
- **Complex Scenarios**: Large projects, multiple tables, project cloning

### 3. Pipeline Tests (`test_pipeline.cpp`)

Tests for the ML Pipeline functionality:

- **Configuration**: JSON configuration parsing and validation
- **Data Generation**: Simulated experimental data creation
- **Model Testing**: Multiple model fitting and comparison
- **Job Management**: Statistical analysis job execution
- **ML Pipeline Manager**: Batch processing and feature extraction
- **File I/O**: Configuration and result file handling
- **Integration**: Full pipeline execution tests
- **Performance**: Large dataset handling and timing tests
- **Error Handling**: Invalid configurations and missing files

## Building and Running Tests

### Prerequisites

- Qt6 with Test module
- CMake 3.16 or higher
- Eigen3 library
- C++17 compatible compiler

### Building Tests

From the main project directory:

```bash
mkdir build
cd build
cmake ..
make
```

### Running Tests

#### Option 1: Using CMake/CTest

```bash
cd build
make run_tests
```

or

```bash
cd build
ctest --verbose
```

#### Option 2: Using the Test Runner Script

```bash
cd build
../src/tests/run_tests.sh
```

#### Option 3: Running Individual Tests

```bash
cd build
./test_datatable
./test_dataclass
./test_pipeline
```

## Test Coverage

The tests cover the following areas:

### DataTable Coverage
- ✅ Basic table operations (create, access, modify)
- ✅ JSON serialization/deserialization
- ✅ File I/O operations
- ✅ Header management
- ✅ Checked data functionality
- ✅ Edge cases and error handling
- ✅ Memory management

### DataClass Coverage
- ✅ Project creation and management
- ✅ System parameter handling
- ✅ Table management (independent/dependent)
- ✅ Project export/import
- ✅ File operations
- ✅ Metadata management
- ✅ Data filtering and range settings
- ✅ Complex project scenarios

### Pipeline Coverage
- ✅ Configuration parsing
- ✅ Data generation with various parameters
- ✅ Model creation and fitting
- ✅ Multiple model testing
- ✅ Job management and execution
- ✅ ML Pipeline Manager functionality
- ✅ File I/O operations
- ✅ Integration testing
- ✅ Performance testing
- ✅ Error handling

## Test Data

The tests use dynamically generated test data to ensure consistency and avoid dependencies on external files. Key test data includes:

- **Concentration Tables**: 2-column host/guest concentration data
- **NMR Titration Data**: Multi-signal experimental data
- **Model Parameters**: Various binding model configurations
- **Statistical Jobs**: Monte Carlo, Cross-validation, Reduction analysis
- **Pipeline Configurations**: Complete ML pipeline setups

## Expected Test Results

### DataTable Tests
- **Basic Operations**: ~15 tests covering table creation, access, modification
- **JSON Operations**: ~10 tests covering export/import functionality
- **File Operations**: ~8 tests covering file I/O
- **Advanced Features**: ~12 tests covering headers, checking, edge cases

### DataClass Tests
- **Project Management**: ~20 tests covering project lifecycle
- **System Parameters**: ~8 tests covering parameter handling
- **File Operations**: ~10 tests covering project file I/O
- **Complex Scenarios**: ~15 tests covering advanced functionality

### Pipeline Tests
- **Configuration**: ~10 tests covering config parsing and validation
- **Data Generation**: ~15 tests covering simulated data creation
- **Model Testing**: ~12 tests covering model fitting and comparison
- **Pipeline Management**: ~8 tests covering ML pipeline functionality
- **Integration**: ~10 tests covering full pipeline execution

## Troubleshooting

### Common Issues

1. **Test executable not found**
   - Ensure tests are built: `make` in build directory
   - Check that all dependencies are installed

2. **Qt Test module not found**
   - Install Qt6 development packages
   - Ensure Qt6::Test is available

3. **Eigen3 errors**
   - Install Eigen3 development packages
   - Check CMake can find Eigen3

4. **Memory errors**
   - Run tests with valgrind for detailed analysis
   - Check for proper cleanup in test destructors

### Debug Mode

For debugging tests, build in debug mode:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Test Coverage Analysis

If gcov is available, generate coverage reports:

```bash
make coverage
```

## Contributing

When adding new functionality to SupraFit:

1. **Add corresponding tests** for new features
2. **Update existing tests** if behavior changes
3. **Ensure all tests pass** before submitting changes
4. **Add integration tests** for complex features
5. **Document test cases** in the test file comments

### Test Naming Convention

- Test methods: `test<Functionality>()`
- Test classes: `Test<Component>`
- Test files: `test_<component>.cpp`

### Test Structure

Each test should:
1. Set up test data in `initTestCase()`
2. Clean up in `cleanupTestCase()`
3. Use meaningful assertions with `QVERIFY` and `QCOMPARE`
4. Test both success and failure cases
5. Include performance considerations for large datasets

## Integration with CI/CD

The test suite is designed to integrate with continuous integration systems:

- **Exit codes**: 0 for success, non-zero for failures
- **Timeout handling**: Tests have reasonable timeouts
- **Resource cleanup**: Proper cleanup prevents test interference
- **Parallel execution**: Tests can run in parallel when properly isolated

## Performance Benchmarks

The test suite includes performance benchmarks:

- **Large dataset handling**: Tests with 1000+ data points
- **Memory usage**: Monitoring for memory leaks
- **Execution time**: Reasonable time limits for operations
- **Scalability**: Tests with varying dataset sizes

Expected performance (on modern hardware):
- DataTable tests: < 5 seconds
- DataClass tests: < 10 seconds  
- Pipeline tests: < 60 seconds

## Future Enhancements

Planned test improvements:

- [ ] Property-based testing for edge cases
- [ ] Integration with external test data
- [ ] Performance regression testing
- [ ] Automated test report generation
- [ ] Cross-platform testing validation