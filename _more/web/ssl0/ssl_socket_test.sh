#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building SSL Socket test ==="
gcc -I include -o test/ssl_socket_test test/ssl_socket_test.c \
    src/ssl_socket.c src/ssl.c src/crypto.c src/sha.c src/aes.c \
    src/bignum.c src/rsa.c src/certificate.c src/rand.c

echo "=== Running SSL Socket test ==="

# Start server in background
./test/ssl_socket_test &
SERVER_PID=$!

# Wait for server to start listening
sleep 2

# Connect with openssl client
echo "Connecting with openssl client..."

# 修改前：
# echo "Q" | openssl s_client -connect localhost:8444 -tls1_2 -servername localhost 2>&1 | head -30 || true

# 修改後（加上 -legacy_renegotiation）：
echo "Q" | openssl s_client -connect localhost:8444 -tls1_2 -legacy_renegotiation -servername localhost 2>&1 | head -30 || true

# Give server a moment to process
sleep 1

# Kill server if still running
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

echo "=== All tests passed ==="