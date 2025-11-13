#!/usr/bin/env python3
"""
Simple test to verify SupraFit Python bindings are working.

Created by Claude Code AI Assistant
"""

import sys

def test_import():
    """Test basic import of suprafit module"""
    try:
        import suprafit as sf
        print("✓ Successfully imported suprafit")
        print(f"  Version: {sf.__version__}")
        return True
    except ImportError as e:
        print(f"✗ Failed to import suprafit: {e}")
        print("\nTroubleshooting:")
        print("1. Make sure the module is built: cd build && make suprafit")
        print("2. Add build directory to PYTHONPATH:")
        print("   export PYTHONPATH=$PYTHONPATH:/path/to/SupraFit/build")
        print("3. Or install the module: sudo make install")
        return False

def test_submodules():
    """Test that all submodules are accessible"""
    import suprafit as sf

    modules = ['io', 'data', 'models', 'statistics']
    all_ok = True

    for module_name in modules:
        if hasattr(sf, module_name):
            print(f"✓ Submodule sf.{module_name} is accessible")
        else:
            print(f"✗ Submodule sf.{module_name} is missing")
            all_ok = False

    return all_ok

def test_available_functions():
    """Test that key functions are available"""
    import suprafit as sf

    tests = [
        ('sf.io.load_data', lambda: hasattr(sf.io, 'load_data')),
        ('sf.io.save_data', lambda: hasattr(sf.io, 'save_data')),
        ('sf.models.create_model', lambda: hasattr(sf.models, 'create_model')),
        ('sf.models.available_models', lambda: hasattr(sf.models, 'available_models')),
        ('sf.statistics.monte_carlo', lambda: hasattr(sf.statistics, 'monte_carlo')),
        ('sf.statistics.cross_validation', lambda: hasattr(sf.statistics, 'cross_validation')),
    ]

    all_ok = True
    for name, test_func in tests:
        if test_func():
            print(f"✓ {name} is available")
        else:
            print(f"✗ {name} is missing")
            all_ok = False

    return all_ok

def test_model_list():
    """Test available models list"""
    import suprafit as sf

    try:
        models = sf.models.available_models()
        print(f"✓ Found {len(models)} available models:")
        for model in models:
            print(f"    - {model}")
        return True
    except Exception as e:
        print(f"✗ Error getting model list: {e}")
        return False

def test_data_creation():
    """Test basic DataClass creation"""
    import suprafit as sf

    try:
        data = sf.data.DataClass()
        print(f"✓ Created DataClass instance")
        print(f"  UUID: {data.UUID()}")
        return True
    except Exception as e:
        print(f"✗ Error creating DataClass: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    print("=" * 60)
    print("SupraFit Python Bindings - Import Test")
    print("=" * 60)
    print()

    tests = [
        ("Module Import", test_import),
        ("Submodules", test_submodules),
        ("Available Functions", test_available_functions),
        ("Model List", test_model_list),
        ("Data Creation", test_data_creation),
    ]

    results = []
    for test_name, test_func in tests:
        print(f"\nTest: {test_name}")
        print("-" * 60)
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            print(f"✗ Test failed with exception: {e}")
            import traceback
            traceback.print_exc()
            results.append((test_name, False))

    # Summary
    print()
    print("=" * 60)
    print("Test Summary")
    print("=" * 60)

    passed = sum(1 for _, result in results if result)
    total = len(results)

    for test_name, result in results:
        status = "PASS" if result else "FAIL"
        symbol = "✓" if result else "✗"
        print(f"{symbol} {test_name}: {status}")

    print()
    print(f"Total: {passed}/{total} tests passed")

    if passed == total:
        print("\n🎉 All tests passed! Python bindings are working correctly.")
        return 0
    else:
        print(f"\n⚠️  {total - passed} test(s) failed. Check the output above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
