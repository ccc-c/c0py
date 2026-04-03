#!/bin/bash
set -e

echo "=== JIT Compiler Test ==="
echo

cd "$(dirname "$0")"

echo "1. Cleaning old files..."
rm -f jit_func.c jit_func.so

echo
echo "2. Compiling jit.c..."
gcc -o jit jit.c -ldl

echo
echo "3. Running JIT demo..."
./jit

echo
echo "4. Verifying output files..."
if [ -f jit_func.c ]; then
    echo "  jit_func.c exists"
    echo "  Content:"
    cat jit_func.c
fi

if [ -f jit_func.so ]; then
    echo "  jit_func.so exists ($(file jit_func.so))"
fi

echo
echo "=== Test Complete ==="
