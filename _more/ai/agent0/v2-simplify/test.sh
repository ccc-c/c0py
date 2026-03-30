#!/bin/bash
set -e

echo "=== Test 1: Read file (cat) ==="
result=$(python3 -m py_compile agent0.py && echo "Syntax OK")
echo "$result"

echo ""
echo "=== Test 2: Write file (echo >) ==="
echo "hello world" > /tmp/agent0_test.txt
result=$(cat /tmp/agent0_test.txt)
echo "Content: $result"

echo ""
echo "=== Test 3: Write file (printf) ==="
printf "line1\nline2\nline3" > /tmp/agent0_test.txt
result=$(cat /tmp/agent0_test.txt)
echo "Content: $result"

echo ""
echo "=== All tests passed! ==="
