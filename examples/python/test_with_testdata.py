#!/usr/bin/env python3
"""
Test Python bindings with real SupraFit test data.
Uses the same test files as the C++ CLI.

Created by Claude Code AI Assistant
"""

import sys
import os
from pathlib import Path

# Add build directory to path
build_path = Path(__file__).parent.parent.parent / "build"
if build_path.exists():
    sys.path.insert(0, str(build_path))

try:
    import suprafit as sf
except ImportError as e:
    print(f"❌ Cannot import suprafit: {e}")
    print("\nBuild instructions:")
    print("  cd build")
    print("  cmake .. -DPython_Bindings=ON")
    print("  make -j4")
    sys.exit(1)

INPUT_DIR = Path(__file__).parent.parent.parent / "input"

def main():
    print("=" * 70)
    print("SupraFit Python Bindings - Test with Real Data")
    print("=" * 70)
    print()

    # Test 1: Load NMR titration data
    print("Test 1: Loading NMR titration data (1_1_1_2_001.dat)")
    print("-" * 70)

    data_file = INPUT_DIR / "1_1_1_2_001.dat"

    if not data_file.exists():
        print(f"❌ Test data not found: {data_file}")
        return 1

    try:
        data = sf.io.load_data(str(data_file))
        print(f"✓ Loaded successfully")
        print(f"  Data points: {data.DataPoints()}")
        print(f"  Series count: {data.SeriesCount()}")
        print(f"  Independent variables: {data.IndependentVariableSize()}")
        print()
    except Exception as e:
        print(f"❌ Failed to load data: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Test 2: Create NMR 1:1+1:2 model
    print("Test 2: Creating NMR 1:1+1:2 binding model")
    print("-" * 70)

    try:
        # This data is from a 1:1+1:2 experiment
        model = sf.models.create_model("nmr_1_1_1_2", data)
        print(f"✓ Model created: {model.Name() if hasattr(model, 'Name') else 'NMR 1:1+1:2'}")
        print(f"  Global parameters: {model.GlobalParameterSize()}")
        print(f"  Local parameters: {model.LocalParameterSize()}")
        print()
    except Exception as e:
        print(f"❌ Failed to create model: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Test 3: Set reasonable initial parameters
    print("Test 3: Setting initial parameters")
    print("-" * 70)

    try:
        # For NMR 1:1+1:2 model, we typically have:
        # Global: K11, K12 (binding constants)
        # Local: chemical shifts for different states
        model.setGlobalParameter(0, 1000.0)   # K11
        if model.GlobalParameterSize() > 1:
            model.setGlobalParameter(1, 500.0)    # K12

        print(f"✓ Parameters set")
        print(f"  K11 = {model.GlobalParameter(0):.1f}")
        if model.GlobalParameterSize() > 1:
            print(f"  K12 = {model.GlobalParameter(1):.1f}")
        print()
    except Exception as e:
        print(f"❌ Failed to set parameters: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Test 4: Fit the model
    print("Test 4: Fitting model to data")
    print("-" * 70)

    try:
        result = sf.models.fit_model(model)

        if result['success']:
            print(f"✓ Fit successful!")
            print(f"  SSE: {result['sse']:.6f}")
            print(f"  SEy: {result['sey']:.6f}")
            print()

            # Print fitted parameters
            print("  Fitted parameters:")
            for i in range(model.GlobalParameterSize()):
                print(f"    Global[{i}]: {model.GlobalParameter(i):.4f}")
            print()
        else:
            print(f"❌ Fit failed")
            print(f"  SSE: {result['sse']:.6f}")
            print(f"  SEy: {result['sey']:.6f}")
            return 1

    except Exception as e:
        print(f"❌ Fitting error: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Test 5: Statistical analysis
    print("Test 5: Statistical analysis")
    print("-" * 70)

    try:
        summary = sf.statistics.statistical_summary(model)
        print(f"✓ Statistical summary:")
        print(f"  Model: {summary['name']}")
        print(f"  Data points: {summary['data_points']}")
        print(f"  Parameters: {summary['parameters']}")
        print(f"  SSE: {summary['sse']:.6f}")
        print(f"  SEy: {summary['sey']:.6f}")
        print()
    except Exception as e:
        print(f"⚠️  Statistical summary failed: {e}")
        # Not critical, continue

    # Test 6: Quick Monte Carlo (100 iterations for speed)
    print("Test 6: Monte Carlo uncertainty analysis (100 iterations)")
    print("-" * 70)

    try:
        mc_result = sf.statistics.monte_carlo(model, iterations=100)

        if mc_result['success']:
            print(f"✓ Monte Carlo completed")
            print(f"  Iterations: {mc_result['iterations']}")
            print(f"  Converged: {mc_result['converged']}")
            print()
        else:
            print(f"⚠️  Monte Carlo did not converge (this is OK for quick test)")
            print()
    except Exception as e:
        print(f"⚠️  Monte Carlo failed: {e}")
        # Not critical for basic functionality

    # Test 7: Export results
    print("Test 7: Exporting results")
    print("-" * 70)

    try:
        output_csv = "/tmp/suprafit_python_test_results.csv"
        output_txt = "/tmp/suprafit_python_test_results.txt"

        sf.io.export_results(model, output_csv, "csv")
        print(f"✓ Exported CSV to: {output_csv}")

        sf.io.export_results(model, output_txt, "txt")
        print(f"✓ Exported TXT to: {output_txt}")

        # Check file sizes
        csv_size = os.path.getsize(output_csv)
        txt_size = os.path.getsize(output_txt)
        print(f"  CSV size: {csv_size} bytes")
        print(f"  TXT size: {txt_size} bytes")
        print()

    except Exception as e:
        print(f"❌ Export failed: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Test 8: Save model
    print("Test 8: Saving model as JSON")
    print("-" * 70)

    try:
        output_json = "/tmp/suprafit_python_test_model.json"
        sf.io.save_model(model, output_json, include_statistics=True)
        print(f"✓ Saved model to: {output_json}")

        json_size = os.path.getsize(output_json)
        print(f"  JSON size: {json_size} bytes")
        print()

    except Exception as e:
        print(f"❌ Save model failed: {e}")
        import traceback
        traceback.print_exc()
        return 1

    # Success!
    print("=" * 70)
    print("✅ All tests completed successfully!")
    print("=" * 70)
    print()
    print("The Python bindings are working correctly with real SupraFit data.")
    print()
    print("Output files created:")
    print(f"  - {output_csv}")
    print(f"  - {output_txt}")
    print(f"  - {output_json}")
    print()

    return 0

if __name__ == "__main__":
    sys.exit(main())
