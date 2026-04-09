#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building SSL Socket test ==="
gcc -I include -o test/ssl_socket_test test/ssl_socket_test.c \
    src/ssl_socket.c src/ssl.c src/crypto.c src/sha.c src/aes.c \
    src/bignum.c src/rsa.c src/certificate.c src/rand.c

echo "=== Running SSL Socket test ==="
./test/ssl_socket_test

echo "=== All tests passed ==="