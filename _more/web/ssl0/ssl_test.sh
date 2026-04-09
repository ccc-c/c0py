#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building SSL test ==="
gcc -I include -o test/ssl_test test/ssl_test.c src/ssl.c src/crypto.c src/sha.c src/aes.c src/bignum.c src/rsa.c

echo "=== Running SSL test ==="
./test/ssl_test

echo "=== All tests passed ==="