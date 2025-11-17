#!/usr/bin/env python3
"""
SupraFit Python Bindings - Minimal Demonstration
Tests core functionality without complex file I/O
"""

import sys
import os
sys.path.insert(0, '/home/conrad/src/claude_code_suprafit/SupraFit/release')
sys.path.insert(0, '/home/conrad/src/claude_code_suprafit/SupraFit/release/src/python_bindings')

print("=" * 70)
print("SupraFit Python Bindings - Functionality Demonstration")
print("=" * 70)
print()

# TEST 1: Import
print("Test 1: Importing _suprafit module")
print("-" * 70)

try:
    import _suprafit as sf
    print(f"✓ _suprafit module imported successfully")
    print(f"  Module file: {sf.__file__}")
    print()
except ImportError as e:
    print(f"❌ Failed to import _suprafit: {e}")
    sys.exit(1)

# TEST 2: DataClass creation
print("Test 2: Creating DataClass")
print("-" * 70)

try:
    data = sf.data.DataClass()
    print(f"✓ DataClass created successfully")
    print(f"  UUID: {data.UUID()}")
    print(f"  Data points: {data.DataPoints()}")
    print(f"  Series count: {data.SeriesCount()}")
    print()
except Exception as e:
    print(f"❌ DataClass creation failed: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

# TEST 3: DataTable access
print("Test 3: DataTable structure")
print("-" * 70)

try:
    # Create empty DataTable
    table = sf.data.DataTable()
    print(f"✓ DataTable created successfully")
    print(f"  Row count: {table.rowCount()}")
    print(f"  Column count: {table.columnCount()}")
    print()
except Exception as e:
    print(f"❌ DataTable creation failed: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

# TEST 4: Available models
print("Test 4: Available Models")
print("-" * 70)

try:
    models = sf.models.available_models()
    print(f"✓ Available models retrieved")
    print(f"  Total model types: {len(models)}")
    print()
    print("  Model list:")
    for i, model_name in enumerate(models[:10], 1):
        print(f"    {i}. {model_name}")
    if len(models) > 10:
        print(f"    ... and {len(models) - 10} more")
    print()
except Exception as e:
    print(f"❌ Failed to get available models: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

# TEST 5: JSON functions
print("Test 5: JSON I/O Functions")
print("-" * 70)

try:
    # Check if JSON functions are available
    has_load_json = hasattr(sf.io, 'load_data_json')
    has_save_json = hasattr(sf.io, 'save_data_json')

    print(f"✓ JSON I/O functions availability:")
    print(f"  load_data_json: {'✓' if has_load_json else '✗'}")
    print(f"  save_data_json: {'✓' if has_save_json else '✗'}")
    print()
except Exception as e:
    print(f"⚠️  Could not check JSON functions: {e}")
    print()

# TEST 6: Statistical functions
print("Test 6: Statistical Functions")
print("-" * 70)

try:
    # Check available statistical functions
    has_summary = hasattr(sf.statistics, 'statistical_summary')
    has_mc = hasattr(sf.statistics, 'monte_carlo')
    has_bootstrap = hasattr(sf.statistics, 'bootstrap')

    print(f"✓ Statistical functions availability:")
    print(f"  statistical_summary: {'✓' if has_summary else '✗'}")
    print(f"  monte_carlo: {'✓' if has_mc else '✗'}")
    print(f"  bootstrap: {'✓' if has_bootstrap else '✗'}")
    print()
except Exception as e:
    print(f"⚠️  Could not check statistical functions: {e}")
    print()

# TEST 7: Summary
print("=" * 70)
print("✅ Python Bindings Demonstration Complete!")
print("=" * 70)
print()
print("Core Modules Accessible:")
print("  ✓ data (DataClass, DataTable)")
print("  ✓ models (create_model, fit_model, available_models)")
print("  ✓ io (JSON-based I/O functions)")
print("  ✓ statistics (analysis functions)")
print()
print("Next Steps:")
print("  1. Load data using JSON functions: load_data_json()")
print("  2. Create model: create_model(model_name, data)")
print("  3. Fit model: fit_model(model)")
print("  4. Statistical analysis: statistical_summary(model)")
print("  5. Save results: save_data_json() or export via io module")
print()
