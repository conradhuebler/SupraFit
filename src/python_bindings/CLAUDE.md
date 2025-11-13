# Python Bindings - Modern pybind11-based Python Interface

## [Preserved Section - Permanent Documentation]

### Purpose
Modern Python bindings for SupraFit using pybind11, providing:
- Data loading and manipulation
- Model creation and fitting
- Statistical analysis (Monte Carlo, Cross-Validation, AIC)
- Result export in various formats

### Architecture

#### File Structure
```
src/python_bindings/
├── bindings_main.cpp      # Main module definition
├── py_dataclass.cpp       # DataClass/DataTable bindings
├── py_models.cpp          # Model fitting bindings
├── py_statistics.cpp      # Statistical analysis bindings
├── py_io.cpp              # File I/O operations
├── CMakeLists.txt         # Build configuration
├── README.md              # User documentation
└── CLAUDE.md              # This file
```

#### Module Structure
```
suprafit (Python module)
├── io                     # Input/Output operations
│   ├── load_data()
│   ├── save_data()
│   ├── load_model()
│   ├── save_model()
│   └── export_results()
├── data                   # Data handling
│   ├── DataClass
│   ├── DataTable
│   └── SystemParameter
├── models                 # Model fitting
│   ├── AbstractModel
│   ├── create_model()
│   ├── fit_model()
│   └── available_models()
└── statistics            # Statistical analysis
    ├── monte_carlo()
    ├── cross_validation()
    ├── calculate_aic()
    ├── compare_models()
    ├── confidence_intervals()
    └── statistical_summary()
```

### Key Design Decisions

1. **pybind11 over SWIG/Boost.Python**
   - Modern C++11/14/17 support
   - Header-only, easy integration
   - Excellent Qt compatibility
   - Automatic Eigen matrix conversion

2. **Submodule Organization**
   - Separate concerns: io, data, models, statistics
   - Clean Python API: `import suprafit as sf`
   - Intuitive usage: `sf.models.create_model()`

3. **QString/QJsonObject Conversion**
   - Helper functions for Qt ↔ Python string conversion
   - JSON objects exposed as Python strings (can parse with `json.loads()`)
   - Maintains compatibility with C++ Qt-based code

4. **Memory Management**
   - `py::return_value_policy` carefully chosen for each binding
   - Qt's parent-child ownership respected
   - DataClass owns DataTable instances

5. **Error Handling**
   - C++ exceptions automatically converted to Python exceptions
   - Descriptive error messages for file operations
   - Validation at Python/C++ boundary

### Dependencies
- pybind11 (≥2.11.0) - Auto-downloaded by CMake if not found
- Python 3.6+
- Qt6 Core and Qml
- SupraFit core and models libraries

### Build System
- CMake option: `Python_Bindings` (default: ON)
- Automatic pybind11 download via FetchContent if not installed
- Installation to Python site-packages
- Copy to build directory for testing without installation

### Usage Pattern
```python
import suprafit as sf

# Load → Fit → Analyze → Export
data = sf.io.load_data("experiment.txt")
model = sf.models.create_model("nmr_1_1", data)
result = sf.models.fit_model(model)
mc = sf.statistics.monte_carlo(model, iterations=10000)
sf.io.export_results(model, "output.csv", "csv")
```

## [Variable Section - Short-term Information]

### Status: WIP - ADDED (Task #Python-1)

### Recent Changes
- Created complete pybind11-based Python interface
- Implemented all core modules: io, data, models, statistics
- Added comprehensive examples and documentation
- Integrated into CMake build system
- Ready for compilation and testing

### Known Issues
- **TODO**: Test compilation with actual C++ codebase
- **TODO**: Verify all function signatures match C++ implementation
- **TODO**: Add unit tests using pytest
- **TODO**: Handle Qt signals/slots in Python (if needed)
- **TODO**: Add numpy array conversion for DataTable (optional enhancement)

### Compilation Status
- Not yet compiled (waiting for operator build)
- CMake configuration complete
- All source files created

## [Instructions Block - Operator-Defined Tasks]

### Immediate Tasks (WIP)

**Task #Python-1: Initial Implementation** - WIP/ADDED
- [x] Design Python API structure
- [x] Create pybind11 bindings for DataClass/DataTable
- [x] Create bindings for model fitting
- [x] Create bindings for statistical analysis
- [x] Create bindings for file I/O
- [x] Write CMake build configuration
- [x] Create user documentation (README.md)
- [x] Create example scripts
- [ ] **NEXT**: Build and test the module
- [ ] Fix compilation errors if any
- [ ] Test with real data files
- [ ] Verify memory management

### Future Enhancements (Vision)

**Task #Python-2: Advanced Features** - TODO
- Add numpy array support for faster data exchange
- Implement Qt signal/slot connections in Python
- Add plotting functionality (matplotlib integration)
- Create Jupyter notebook examples
- Add type hints (`.pyi` stub files) for better IDE support

**Task #Python-3: Performance** - TODO
- Benchmark Python vs C++ performance
- Optimize data transfer between Python and C++
- Consider GIL release for long-running operations
- Profile memory usage

**Task #Python-4: Testing** - TODO
- Create pytest test suite
- Add continuous integration tests
- Test on multiple platforms (Linux, macOS, Windows)
- Memory leak testing

**Task #Python-5: Documentation** - TODO
- Generate API docs with Sphinx
- Create tutorial notebooks
- Add to main SupraFit documentation
- Video tutorials

### Testing Instructions

After building:
```bash
cd build
export PYTHONPATH=$PYTHONPATH:$(pwd)
python3 ../examples/python/test_import.py
python3 ../examples/python/basic_example.py
```

### Integration Notes
- Python bindings are independent of the old `_Python` option (legacy PythonLibs)
- New option: `Python_Bindings` (ON by default)
- Can coexist with legacy Python interface during transition
- Uses modern CMake `find_package(Python3)` instead of deprecated `PythonLibs`

## Notes for CLAUDE

1. **When fixing compilation errors**: Check function signatures in C++ headers carefully
2. **Memory management**: Use `py::return_value_policy::reference` for Qt-owned objects
3. **QString handling**: Always use helper functions for conversion
4. **Model creation**: May need ModelHandler or factory pattern from C++ side
5. **Testing**: Run test_import.py first to verify basic functionality

## Version History
- 2025-01: Initial implementation by Claude Code AI Assistant
