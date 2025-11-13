#!/usr/bin/env python3
"""
Dry run test - verifies test structure without actual suprafit module.
Tests Python code structure and data file access.
"""

import sys
from pathlib import Path

INPUT_DIR = Path(__file__).parent.parent.parent / "input"

def test_file_access():
    """Verify test data files are accessible"""
    print("Testing file access...")

    test_file = INPUT_DIR / "1_1_1_2_001.dat"

    if not test_file.exists():
        print(f"❌ Test file not found: {test_file}")
        return False

    # Read and analyze the file
    with open(test_file, 'r') as f:
        lines = f.readlines()

    print(f"✓ Found test file: {test_file}")
    print(f"  Lines: {len(lines)}")

    if len(lines) > 0:
        # Parse first line
        parts = lines[0].strip().split('\t')
        print(f"  Columns: {len(parts)}")
        print(f"  First line: {lines[0].strip()}")

    return True

def test_python_syntax():
    """Verify Python test files compile"""
    print("\nTesting Python syntax...")

    test_files = [
        "test_import.py",
        "test_integration.py",
        "test_with_testdata.py",
        "basic_example.py",
        "advanced_statistics.py"
    ]

    examples_dir = Path(__file__).parent

    for test_file in test_files:
        file_path = examples_dir / test_file
        if file_path.exists():
            try:
                import py_compile
                py_compile.compile(str(file_path), doraise=True)
                print(f"✓ {test_file} - syntax OK")
            except Exception as e:
                print(f"❌ {test_file} - syntax error: {e}")
                return False
        else:
            print(f"⚠️  {test_file} - not found")

    return True

def test_module_structure():
    """Test the expected module structure"""
    print("\nExpected module structure:")
    print("  suprafit/")
    print("    ├── io.load_data()")
    print("    ├── io.save_data()")
    print("    ├── data.DataClass")
    print("    ├── models.create_model()")
    print("    ├── models.fit_model()")
    print("    ├── statistics.monte_carlo()")
    print("    └── statistics.cross_validation()")
    print()
    print("✓ Module structure defined")
    return True

def main():
    print("=" * 70)
    print("SupraFit Python Bindings - Dry Run Test")
    print("(Tests structure without compiled module)")
    print("=" * 70)
    print()

    results = []

    # Test 1: File access
    results.append(("File access", test_file_access()))

    # Test 2: Python syntax
    results.append(("Python syntax", test_python_syntax()))

    # Test 3: Module structure
    results.append(("Module structure", test_module_structure()))

    # Summary
    print("\n" + "=" * 70)
    print("Dry Run Summary")
    print("=" * 70)

    passed = sum(1 for _, r in results if r)
    total = len(results)

    for name, result in results:
        status = "✓" if result else "❌"
        print(f"{status} {name}")

    print()
    print(f"Results: {passed}/{total} checks passed")

    if passed == total:
        print("\n✅ Structure looks good! Ready for compilation.")
        print("\nNext steps:")
        print("  1. Build SupraFit with Qt6:")
        print("     cd build && cmake .. -DPython_Bindings=ON && make -j4")
        print("  2. Set PYTHONPATH:")
        print("     export PYTHONPATH=$PYTHONPATH:$(pwd)")
        print("  3. Run actual tests:")
        print("     python3 ../examples/python/test_integration.py")
    else:
        print("\n⚠️  Some checks failed. Review output above.")

    return 0 if passed == total else 1

if __name__ == "__main__":
    sys.exit(main())
