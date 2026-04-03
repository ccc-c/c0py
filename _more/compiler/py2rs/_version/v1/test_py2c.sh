#!/bin/bash

echo "========================================"
echo "   py2c Test Suite"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

FAILED=0
PASSED=0

test_py2c() {
    local test_name="$1"
    local input_file="$2"
    local expected_pattern="$3"
    
    echo "Testing: $test_name"
    echo "  Input: $input_file"
    
    base_name=$(basename "$input_file" .py)
    output_c="test/${base_name}.c"
    
    python3 py2c.py "$input_file" -o "$output_c" > /dev/null 2>&1
    result=$?
    
    if [ $result -ne 0 ]; then
        echo "  FAIL: Script execution failed"
        FAILED=$((FAILED + 1))
        return
    fi
    
    if [ -f "$output_c" ]; then
        if grep -q "$expected_pattern" "$output_c"; then
            echo "  PASS: C code contains '$expected_pattern'"
            
            gcc -o "test/${base_name}" "$output_c" 2>/dev/null
            if [ -x "test/${base_name}" ]; then
                exec_output=$(./test/"${base_name}" 2>&1)
                echo "    Executed: $exec_output"
                PASSED=$((PASSED + 1))
            else
                echo "  FAIL: Compilation failed"
                FAILED=$((FAILED + 1))
            fi
        else
            echo "  FAIL: C code does not contain '$expected_pattern'"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "  FAIL: C file not generated"
        FAILED=$((FAILED + 1))
    fi
    echo ""
}

if [ $# -gt 0 ]; then
    INPUT_FILE="$1"
    EXPECTED="${2:-factorial}"

    if [ ! -f "$INPUT_FILE" ]; then
        echo "Error: File '$INPUT_FILE' not found"
        exit 1
    fi

    test_py2c "$(basename "$INPUT_FILE")" "$INPUT_FILE" "$EXPECTED"
else
    echo "=== Running all tests in test/ folder ==="
    echo ""
    
    rm -f test/fact test/fact_no_anno
    
    echo "=== Testing supported features (should pass) ==="
    test_py2c "fact.py" "test/fact.py" "factorial"
    test_py2c "fact_no_anno.py" "test/fact_no_anno.py" "factorial"
    test_py2c "fib.py" "test/fib.py" "fib"
    test_py2c "factorial_while.py" "test/factorial_while.py" "factorial_while"
    
    echo "=== Testing unsupported features (should fail) ==="
    
    for test_file in test/array_test.py test/dict_test.py test/sum_array.py test/dict_func.py; do
        if [ -f "$test_file" ]; then
            echo "Testing: $(basename "$test_file") (should fail)"
            output=$(python3 py2c.py "$test_file" 2>&1)
            if echo "$output" | grep -q "cannot be converted to C"; then
                echo "  PASS: Correctly rejected unsupported feature"
                PASSED=$((PASSED + 1))
            else
                echo "  FAIL: Should have rejected unsupported feature"
                echo "$output"
                FAILED=$((FAILED + 1))
            fi
            echo ""
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