#!/bin/bash

echo "========================================"
echo "   py0jit Test Suite"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

FAILED=0
PASSED=0

test_jit() {
    local test_name="$1"
    local input_file="$2"
    local expected_output="$3"
    
    echo "Testing: $test_name"
    echo "  Input: $input_file"
    
    base_name=$(basename "$input_file" .py)
    rm -f "${base_name}.so"
    
    output=$(python3 py0jit.py "$input_file" 2>&1)
    result=$?
    
    if [ $result -ne 0 ]; then
        echo "  FAIL: Script execution failed"
        echo "$output"
        FAILED=$((FAILED + 1))
        return
    fi
    
    if [[ "$output" == *"$expected_output"* ]]; then
        echo "  PASS: Output contains '$expected_output'"
        PASSED=$((PASSED + 1))
    else
        echo "  FAIL: Expected '$expected_output', got:"
        echo "$output"
        FAILED=$((FAILED + 1))
    fi
    echo ""
}

if [ $# -gt 0 ]; then
    INPUT_FILE="$1"
    EXPECTED="${2:-factorial(5)=120}"

    if [ ! -f "$INPUT_FILE" ]; then
        echo "Error: File '$INPUT_FILE' not found"
        exit 1
    fi

    test_jit "$(basename "$INPUT_FILE")" "$INPUT_FILE" "$EXPECTED"
else
    echo "=== Running all tests in test/ folder ==="
    echo ""
    
    for test_file in test/*.py; do
        if [ -f "$test_file" ]; then
            test_jit "$(basename "$test_file")" "$test_file" "factorial(5)=120"
        fi
    done
fi

echo "========================================"
echo "   Test Results"
echo "========================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed!"
    exit 1
fi