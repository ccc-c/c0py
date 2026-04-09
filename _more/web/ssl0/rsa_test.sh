#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building RSA test ==="
gcc -I include -o test/rsa_test test/rsa_test.c src/bignum.c src/rsa.c

echo "=== Running RSA test ==="
./test/rsa_test

echo "=== All tests passed ==="