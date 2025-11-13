#!/usr/bin/env python3
"""
Basic SupraFit Python Example
Demonstrates loading data, fitting a model, and performing statistical analysis.

Created by Claude Code AI Assistant
"""

import sys
import suprafit as sf

def main():
    print("=" * 60)
    print("SupraFit Python Interface - Basic Example")
    print("=" * 60)
    print()

    # Check version
    print(f"SupraFit version: {sf.__version__}")
    print()

    # Example 1: Load data
    print("Example 1: Loading Data")
    print("-" * 60)

    try:
        # Load data from file
        data = sf.io.load_data("../input/nmr_titration.txt", "txt")
        print(f"✓ Loaded {data.DataPoints()} data points")
        print(f"  - Independent variables: {data.IndependentVariableSize()}")
        print(f"  - Series count: {data.SeriesCount()}")
        print()
    except Exception as e:
        print(f"✗ Could not load data: {e}")
        print("  Using simulated data instead...")
        # In practice, you'd generate or use different data here
        return

    # Example 2: Create and fit model
    print("Example 2: Model Fitting")
    print("-" * 60)

    try:
        # Create NMR 1:1 binding model
        model = sf.models.create_model("nmr_1_1", data)
        print(f"✓ Created model: NMR 1:1")

        # Set initial parameters
        model.setGlobalParameter(0, 1000.0)  # K (binding constant)
        model.setGlobalParameter(1, 7.5)     # δ_complex
        model.setGlobalParameter(2, 8.5)     # δ_free
        print(f"✓ Set initial parameters")

        # Fit the model
        result = sf.models.fit_model(model)

        if result['success']:
            print(f"✓ Fit successful!")
            print(f"  - SSE: {result['sse']:.6f}")
            print(f"  - SEy: {result['sey']:.6f}")

            # Get fitted parameters
            print(f"\n  Fitted parameters:")
            for i in range(model.GlobalParameterSize()):
                print(f"    Global[{i}]: {model.GlobalParameter(i):.4f}")
        else:
            print(f"✗ Fit failed")
            return
        print()
    except Exception as e:
        print(f"✗ Model fitting error: {e}")
        return

    # Example 3: Statistical analysis
    print("Example 3: Statistical Analysis")
    print("-" * 60)

    try:
        # Get statistical summary
        summary = sf.statistics.statistical_summary(model)
        print(f"Model summary:")
        print(f"  - Name: {summary['name']}")
        print(f"  - Data points: {summary['data_points']}")
        print(f"  - Parameters: {summary['parameters']}")
        print(f"  - SSE: {summary['sse']:.6f}")
        print(f"  - SEy: {summary['sey']:.6f}")
        print()

        # Monte Carlo analysis
        print("Running Monte Carlo analysis (1000 iterations)...")
        mc_result = sf.statistics.monte_carlo(model, iterations=1000, confidence=0.95)

        if mc_result['success']:
            print(f"✓ Monte Carlo analysis complete")
            print(f"  - Iterations: {mc_result['iterations']}")
            print(f"  - Converged: {mc_result['converged']}")
            # Parse and display statistics
            # In practice, you'd parse the JSON string in mc_result['statistics']
        else:
            print(f"✗ Monte Carlo analysis failed")
        print()

    except Exception as e:
        print(f"✗ Statistical analysis error: {e}")
        import traceback
        traceback.print_exc()

    # Example 4: Export results
    print("Example 4: Export Results")
    print("-" * 60)

    try:
        # Export as CSV
        sf.io.export_results(model, "results.csv", "csv")
        print(f"✓ Exported results to results.csv")

        # Export as formatted text
        sf.io.export_results(model, "results.txt", "txt")
        print(f"✓ Exported results to results.txt")

        # Save model as JSON
        sf.io.save_model(model, "model.json", include_statistics=True)
        print(f"✓ Saved model to model.json")
        print()

    except Exception as e:
        print(f"✗ Export error: {e}")

    print("=" * 60)
    print("Example complete!")
    print("=" * 60)

if __name__ == "__main__":
    main()
