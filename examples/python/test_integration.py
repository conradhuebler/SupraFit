#!/usr/bin/env python3
"""
Integration test for SupraFit Python bindings using existing test suite data.
Tests the Python interface against the same data used by C++ CLI tests.

Created by Claude Code AI Assistant
"""

import sys
import os
import json
from pathlib import Path

# Add build directory to path if running without installation
build_path = Path(__file__).parent.parent.parent / "build"
if build_path.exists():
    sys.path.insert(0, str(build_path))

try:
    import suprafit as sf
except ImportError as e:
    print(f"ERROR: Cannot import suprafit module: {e}")
    print("\nPlease build the Python bindings first:")
    print("  cd build && cmake .. -DPython_Bindings=ON && make")
    print("\nOr set PYTHONPATH:")
    print("  export PYTHONPATH=$PYTHONPATH:/path/to/SupraFit/build")
    sys.exit(1)

# Test configuration
INPUT_DIR = Path(__file__).parent.parent.parent / "input"
TEST_RESULTS = []

def log_test(name, passed, message=""):
    """Log test result"""
    status = "✓ PASS" if passed else "✗ FAIL"
    TEST_RESULTS.append((name, passed, message))
    print(f"{status}: {name}")
    if message and not passed:
        print(f"  └─ {message}")

def test_module_import():
    """Test 1: Module import and version"""
    try:
        version = sf.__version__
        log_test("Module import and version check", True, f"Version: {version}")
        return True
    except Exception as e:
        log_test("Module import and version check", False, str(e))
        return False

def test_submodules():
    """Test 2: Submodule availability"""
    try:
        required = ['io', 'data', 'models', 'statistics']
        missing = [mod for mod in required if not hasattr(sf, mod)]

        if missing:
            log_test("Submodule availability", False, f"Missing: {missing}")
            return False

        log_test("Submodule availability", True, f"All {len(required)} submodules present")
        return True
    except Exception as e:
        log_test("Submodule availability", False, str(e))
        return False

def test_available_models():
    """Test 3: Available models list"""
    try:
        models = sf.models.available_models()

        if not isinstance(models, list) or len(models) == 0:
            log_test("Available models list", False, "No models returned")
            return False

        log_test("Available models list", True, f"{len(models)} models available")
        return True
    except Exception as e:
        log_test("Available models list", False, str(e))
        return False

def test_data_class_creation():
    """Test 4: DataClass instantiation"""
    try:
        data = sf.data.DataClass()
        uuid = data.UUID()

        if not uuid or len(uuid) == 0:
            log_test("DataClass creation", False, "Invalid UUID")
            return False

        log_test("DataClass creation", True, f"UUID: {uuid[:16]}...")
        return True
    except Exception as e:
        log_test("DataClass creation", False, str(e))
        return False

def test_load_nmr_data():
    """Test 5: Load NMR test data (1_1_1_2_001.dat)"""
    try:
        data_file = INPUT_DIR / "1_1_1_2_001.dat"

        if not data_file.exists():
            log_test("Load NMR test data", False, f"File not found: {data_file}")
            return None

        data = sf.io.load_data(str(data_file), "txt")
        points = data.DataPoints()
        series = data.SeriesCount()

        if points == 0:
            log_test("Load NMR test data", False, "No data points loaded")
            return None

        log_test("Load NMR test data", True, f"{points} points, {series} series")
        return data

    except Exception as e:
        log_test("Load NMR test data", False, str(e))
        return None

def test_create_nmr_model(data):
    """Test 6: Create NMR 1:1 model"""
    if data is None:
        log_test("Create NMR model", False, "No data available")
        return None

    try:
        model = sf.models.create_model("nmr_1_1", data)

        if model is None:
            log_test("Create NMR model", False, "Model creation returned None")
            return None

        global_params = model.GlobalParameterSize()
        local_params = model.LocalParameterSize()

        log_test("Create NMR model", True,
                f"Global params: {global_params}, Local params: {local_params}")
        return model

    except Exception as e:
        log_test("Create NMR model", False, str(e))
        return None

def test_set_parameters(model):
    """Test 7: Set model parameters"""
    if model is None:
        log_test("Set model parameters", False, "No model available")
        return False

    try:
        # Set initial parameters for NMR 1:1
        model.setGlobalParameter(0, 1000.0)  # K (binding constant)

        # Verify
        K = model.GlobalParameter(0)

        if abs(K - 1000.0) > 1e-6:
            log_test("Set model parameters", False, f"Parameter mismatch: {K} != 1000.0")
            return False

        log_test("Set model parameters", True, f"K = {K}")
        return True

    except Exception as e:
        log_test("Set model parameters", False, str(e))
        return False

def test_fit_model(model):
    """Test 8: Fit model to data"""
    if model is None:
        log_test("Fit model", False, "No model available")
        return None

    try:
        result = sf.models.fit_model(model)

        if not isinstance(result, dict):
            log_test("Fit model", False, "Invalid result type")
            return None

        success = result.get('success', False)
        sse = result.get('sse', float('inf'))
        sey = result.get('sey', float('inf'))

        if not success:
            log_test("Fit model", False, f"Fit failed: SSE={sse}, SEy={sey}")
            return None

        log_test("Fit model", True, f"SSE={sse:.6f}, SEy={sey:.6f}")
        return result

    except Exception as e:
        log_test("Fit model", False, str(e))
        import traceback
        traceback.print_exc()
        return None

def test_statistical_summary(model):
    """Test 9: Get statistical summary"""
    if model is None:
        log_test("Statistical summary", False, "No model available")
        return False

    try:
        summary = sf.statistics.statistical_summary(model)

        required_keys = ['name', 'sse', 'sey', 'data_points', 'parameters']
        missing = [k for k in required_keys if k not in summary]

        if missing:
            log_test("Statistical summary", False, f"Missing keys: {missing}")
            return False

        log_test("Statistical summary", True,
                f"Name: {summary['name']}, Points: {summary['data_points']}")
        return True

    except Exception as e:
        log_test("Statistical summary", False, str(e))
        return False

def test_monte_carlo_quick(model):
    """Test 10: Monte Carlo analysis (quick, 100 iterations)"""
    if model is None:
        log_test("Monte Carlo (quick)", False, "No model available")
        return False

    try:
        # Quick test with only 100 iterations
        mc_result = sf.statistics.monte_carlo(model, iterations=100, confidence=0.95)

        if not isinstance(mc_result, dict):
            log_test("Monte Carlo (quick)", False, "Invalid result type")
            return False

        success = mc_result.get('success', False)
        iterations = mc_result.get('iterations', 0)

        if not success:
            log_test("Monte Carlo (quick)", False, "MC analysis failed")
            return False

        log_test("Monte Carlo (quick)", True, f"{iterations} iterations completed")
        return True

    except Exception as e:
        log_test("Monte Carlo (quick)", False, str(e))
        import traceback
        traceback.print_exc()
        return False

def test_export_results(model):
    """Test 11: Export results to CSV"""
    if model is None:
        log_test("Export results", False, "No model available")
        return False

    try:
        import tempfile

        # Export to temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            temp_csv = f.name

        sf.io.export_results(model, temp_csv, "csv")

        # Check file exists and has content
        if not os.path.exists(temp_csv):
            log_test("Export results", False, "File not created")
            return False

        file_size = os.path.getsize(temp_csv)

        # Clean up
        os.unlink(temp_csv)

        if file_size == 0:
            log_test("Export results", False, "Empty file")
            return False

        log_test("Export results", True, f"CSV file: {file_size} bytes")
        return True

    except Exception as e:
        log_test("Export results", False, str(e))
        return False

def test_save_load_model(model):
    """Test 12: Save and load model as JSON"""
    if model is None:
        log_test("Save/Load model", False, "No model available")
        return False

    try:
        import tempfile

        # Save to temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_json = f.name

        sf.io.save_model(model, temp_json, include_statistics=True)

        # Check file exists
        if not os.path.exists(temp_json):
            log_test("Save/Load model", False, "File not created")
            return False

        # Try to load it back
        loaded = sf.io.load_model(temp_json)

        # Clean up
        os.unlink(temp_json)

        log_test("Save/Load model", True, "Model saved and loaded successfully")
        return True

    except Exception as e:
        # This might fail if load_model is not fully implemented
        log_test("Save/Load model", False, f"Expected: {str(e)}")
        return False

def run_all_tests():
    """Run complete test suite"""
    print("=" * 70)
    print("SupraFit Python Bindings - Integration Test Suite")
    print("Testing against existing C++ test data")
    print("=" * 70)
    print()

    # Run tests in sequence
    print("Phase 1: Module and API Tests")
    print("-" * 70)
    test_module_import()
    test_submodules()
    test_available_models()
    test_data_class_creation()
    print()

    print("Phase 2: Data Loading and Model Creation")
    print("-" * 70)
    data = test_load_nmr_data()
    model = test_create_nmr_model(data)
    test_set_parameters(model)
    print()

    print("Phase 3: Model Fitting")
    print("-" * 70)
    fit_result = test_fit_model(model)
    print()

    print("Phase 4: Statistical Analysis")
    print("-" * 70)
    test_statistical_summary(model)
    test_monte_carlo_quick(model)
    print()

    print("Phase 5: Export and Persistence")
    print("-" * 70)
    test_export_results(model)
    test_save_load_model(model)
    print()

    # Summary
    print("=" * 70)
    print("Test Summary")
    print("=" * 70)

    passed = sum(1 for _, p, _ in TEST_RESULTS if p)
    total = len(TEST_RESULTS)

    for name, result, _ in TEST_RESULTS:
        status = "✓" if result else "✗"
        print(f"{status} {name}")

    print()
    print(f"Results: {passed}/{total} tests passed ({100*passed/total:.1f}%)")

    if passed == total:
        print("\n🎉 All tests passed! Python bindings are fully functional.")
        return 0
    else:
        print(f"\n⚠️  {total - passed} test(s) failed. Check output above for details.")
        return 1

if __name__ == "__main__":
    sys.exit(run_all_tests())
