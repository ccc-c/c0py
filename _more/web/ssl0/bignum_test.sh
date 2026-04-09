#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building bignum test ==="
gcc -I include -o test/bignum_test test/bignum_test.c src/bignum.c

echo "=== Running bignum test ==="
./test/bignum_test

echo "=== All tests passed ==="