#!/usr/bin/env python3
"""
Advanced SupraFit Python Example - Statistical Analysis
Demonstrates Monte Carlo, Cross-Validation, and Model Comparison.

Created by Claude Code AI Assistant
"""

import sys
import json
import suprafit as sf

def format_json(json_str):
    """Pretty print JSON string"""
    try:
        obj = json.loads(json_str)
        return json.dumps(obj, indent=2)
    except:
        return json_str

def main():
    print("=" * 70)
    print("SupraFit Python Interface - Advanced Statistical Analysis")
    print("=" * 70)
    print()

    # Load data
    try:
        data = sf.io.load_data("../input/itc_experiment.txt", "txt")
        print(f"✓ Loaded {data.DataPoints()} data points for ITC analysis")
    except:
        print("✗ Could not load data file")
        print("  Please ensure input data exists or modify the path")
        return

    print()

    # Create and fit multiple models for comparison
    print("Creating and fitting multiple models...")
    print("-" * 70)

    models = []
    model_names = ["itc_1_1", "itc_1_2", "itc_2_1"]

    for model_type in model_names:
        try:
            print(f"  Model: {model_type}")
            model = sf.models.create_model(model_type, data)

            # Set reasonable initial parameters
            for i in range(model.GlobalParameterSize()):
                model.setGlobalParameter(i, 1000.0 if i == 0 else 10.0)

            # Fit
            result = sf.models.fit_model(model)

            if result['success']:
                print(f"    ✓ Fit successful (SSE: {result['sse']:.4f})")
                models.append(model)
            else:
                print(f"    ✗ Fit failed")

        except Exception as e:
            print(f"    ✗ Error: {e}")

    print()

    if len(models) == 0:
        print("✗ No models fitted successfully")
        return

    # Detailed analysis of best model
    best_model = models[0]  # Assuming first is best for demo
    print("Detailed Analysis of Primary Model")
    print("-" * 70)

    # 1. Statistical Summary
    print("\n1. Statistical Summary:")
    summary = sf.statistics.statistical_summary(best_model)
    print(f"   Model: {summary['name']}")
    print(f"   Data points: {summary['data_points']}")
    print(f"   Parameters: {summary['parameters']}")
    print(f"   SSE: {summary['sse']:.6f}")
    print(f"   SEy: {summary['sey']:.6f}")

    print(f"\n   Global parameters:")
    for i, param in enumerate(summary['global_parameters']):
        print(f"     [{i}]: {param:.4f}")

    # 2. Monte Carlo Analysis
    print("\n2. Monte Carlo Uncertainty Analysis:")
    print("   Running 10000 iterations (this may take a moment)...")

    mc_result = sf.statistics.monte_carlo(
        best_model,
        iterations=10000,
        confidence=0.95
    )

    if mc_result['success']:
        print(f"   ✓ Analysis complete")
        print(f"   - Total iterations: {mc_result['iterations']}")
        print(f"   - Converged: {mc_result['converged']}")
        print(f"\n   Statistics (JSON):")
        print(format_json(mc_result['statistics']))
    else:
        print(f"   ✗ Monte Carlo failed")

    # 3. Confidence Intervals
    print("\n3. Confidence Intervals (95%, percentile-based):")
    ci_result = sf.statistics.confidence_intervals(
        best_model,
        iterations=10000,
        lower=0.025,
        upper=0.975
    )

    if ci_result['success']:
        print(f"   ✓ Confidence intervals calculated")
        print(f"   Details:")
        print(format_json(ci_result['intervals']))
    else:
        print(f"   ✗ CI calculation failed")

    # 4. Cross-Validation
    print("\n4. Cross-Validation:")
    print("   Performing leave-one-out cross-validation...")

    cv_result = sf.statistics.cross_validation(
        best_model,
        cv_type=1,  # Leave-one-out
        folds=5
    )

    if cv_result['success']:
        print(f"   ✓ Cross-validation complete")
        print(f"   - CV Score: {cv_result['cv_score']:.6f}")
        print(f"   - CV Error: {cv_result['cv_error']:.6f}")
        print(f"   Details:")
        print(format_json(cv_result['details']))
    else:
        print(f"   ✗ Cross-validation failed")

    # 5. Model Comparison
    if len(models) > 1:
        print("\n5. Model Comparison (AIC and other criteria):")
        print("-" * 70)

        comparison = sf.statistics.compare_models(models)

        print("\n   AIC Comparison:")
        print(format_json(comparison['aic']))

        print("\n   Model Statistics:")
        for i, stats in enumerate(comparison['models']):
            print(f"   Model {i+1}: {stats['name']}")
            print(f"     - SSE: {stats['sse']:.6f}")
            print(f"     - SEy: {stats['sey']:.6f}")
            print(f"     - Parameters: {stats['parameters']}")

    # 6. Export comprehensive results
    print("\n6. Exporting Results:")
    print("-" * 70)

    try:
        # Export model
        sf.io.save_model(best_model, "analysis_model.json", include_statistics=True)
        print("   ✓ Saved model to: analysis_model.json")

        # Export data
        sf.io.export_results(best_model, "analysis_results.csv", "csv")
        print("   ✓ Saved results to: analysis_results.csv")

        sf.io.export_results(best_model, "analysis_results.txt", "txt")
        print("   ✓ Saved formatted report to: analysis_results.txt")

    except Exception as e:
        print(f"   ✗ Export error: {e}")

    print()
    print("=" * 70)
    print("Advanced analysis complete!")
    print("=" * 70)

if __name__ == "__main__":
    main()
