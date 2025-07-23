#!/bin/bash
# Test runner script for SupraFit
# Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>

set -e

echo "=========================================="
echo "SupraFit Test Suite"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run a test and report results
run_test() {
    local test_name="$1"
    local test_executable="$2"
    
    echo -e "${YELLOW}Running $test_name...${NC}"
    
    if [ -f "$test_executable" ]; then
        if ./"$test_executable"; then
            echo -e "${GREEN}✓ $test_name PASSED${NC}"
            return 0
        else
            echo -e "${RED}✗ $test_name FAILED${NC}"
            return 1
        fi
    else
        echo -e "${RED}✗ $test_name executable not found: $test_executable${NC}"
        return 1
    fi
}

# Check if we're in the right directory
if [ ! -f "test_datatable" ] && [ ! -f "test_dataclass" ] && [ ! -f "test_pipeline" ]; then
    echo -e "${RED}Error: Test executables not found in current directory${NC}"
    echo "Please run this script from the build directory where test executables are located"
    echo "Typically: cd build && ../src/tests/run_tests.sh"
    exit 1
fi

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=0

# Run DataTable tests
echo ""
echo "=========================================="
echo "DataTable Tests"
echo "=========================================="
total_tests=$((total_tests + 1))
if run_test "DataTable" "test_datatable"; then
    passed_tests=$((passed_tests + 1))
else
    failed_tests=$((failed_tests + 1))
fi

# Run DataClass tests
echo ""
echo "=========================================="
echo "DataClass Tests"
echo "=========================================="
total_tests=$((total_tests + 1))
if run_test "DataClass" "test_dataclass"; then
    passed_tests=$((passed_tests + 1))
else
    failed_tests=$((failed_tests + 1))
fi

# Run Pipeline tests
echo ""
echo "=========================================="
echo "Pipeline Tests"
echo "=========================================="
total_tests=$((total_tests + 1))
if run_test "Pipeline" "test_pipeline"; then
    passed_tests=$((passed_tests + 1))
else
    failed_tests=$((failed_tests + 1))
fi

# Summary
echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Total tests: $total_tests"
echo -e "Passed: ${GREEN}$passed_tests${NC}"
echo -e "Failed: ${RED}$failed_tests${NC}"

if [ $failed_tests -eq 0 ]; then
    echo ""
    echo -e "${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}Some tests failed! ✗${NC}"
    exit 1
fi