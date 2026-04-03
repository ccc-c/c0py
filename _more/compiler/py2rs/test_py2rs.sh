#!/bin/bash

echo "========================================"
echo "   py2rs Test Suite"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

FAILED=0
PASSED=0

test_py2rs() {
    local test_name="$1"
    local input_file="$2"
    local expected_pattern="$3"
    
    echo "Testing: $test_name"
    echo "  Input: $input_file"
    
    base_name=$(basename "$input_file" .py)
    output_rs="test/${base_name}.rs"
    output_bin="test/${base_name}_bin"
    
    python3 py2rs.py "$input_file" -o "$output_rs" > /dev/null 2>&1
    result=$?
    
    if [ $result -ne 0 ]; then
        echo "  FAIL: Script execution failed"
        FAILED=$((FAILED + 1))
        return
    fi
    
    if [ -f "$output_rs" ]; then
        if grep -q "$expected_pattern" "$output_rs"; then
            echo "  PASS: Rust code contains '$expected_pattern'"
            
            rustc "$output_rs" -o "$output_bin" 2>/dev/null
            if [ -x "$output_bin" ]; then
                exec_output=$("$output_bin" 2>&1)
                echo "    Executed: $exec_output"
                PASSED=$((PASSED + 1))
            else
                echo "  FAIL: Rust compilation failed"
                FAILED=$((FAILED + 1))
            fi
        else
            echo "  FAIL: Rust code does not contain '$expected_pattern'"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "  FAIL: Rust file not generated"
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

    test_py2rs "$(basename "$INPUT_FILE")" "$INPUT_FILE" "$EXPECTED"
else
    echo "=== Running all tests in test/ folder ==="
    echo ""
    
    rm -f test/*_bin
    
    echo "=== Testing functions ==="
    test_py2rs "fact.py" "test/fact.py" "factorial"
    test_py2rs "fib.py" "test/fib.py" "fib"
    test_py2rs "factorial_while.py" "test/factorial_while.py" "factorial_while"
    
    echo "=== Testing list (array) ==="
    test_py2rs "array_test.py" "test/array_test.py" "vec!"
    test_py2rs "sum_array.py" "test/sum_array.py" "sum_array"
    
    echo "=== Testing dict (HashMap) ==="
    test_py2rs "dict_test.py" "test/dict_test.py" "HashMap"
    test_py2rs "dict_func.py" "test/dict_func.py" "get_value"
    
    echo "=== Testing imports ==="
    test_py2rs "circle_area.py" "test/circle_area.py" "circle_area"
    
    echo "=== Testing complex (requires LLM) ==="
    test_py2rs "numpy_example.py" "test/numpy_example.py" "matrix_multiply"
    test_py2rs "class_example.py" "test/class_example.py" "Counter"
    test_py2rs "lambda_example.py" "test/lambda_example.py" "squared"
    test_py2rs "complex_list.py" "test/complex_list.py" "even_squares"
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