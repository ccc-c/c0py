#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "=== Building ssl_socket_test ==="
gcc -I include -o test/ssl_socket_test test/ssl_socket_test.c \
    src/ssl_socket.c src/ssl.c src/crypto.c src/sha.c src/aes.c \
    src/bignum.c src/rsa.c src/certificate.c src/rand.c

if [ $? -ne 0 ]; then
    echo "編譯失敗"
    exit 1
fi

echo "=== ssl_socket_test built ==="

SERVER_PID=""
cleanup() {
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo "=== Starting server ==="
./test/ssl_socket_test &
SERVER_PID=$!

sleep 2

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server failed to start"
    exit 1
fi

echo "=== Testing with curl ==="
RESPONSE=$(curl -k -s https://localhost:8444/ 2>&1)
CURL_EXIT=$?

if [ $CURL_EXIT -ne 0 ]; then
    echo "curl failed with exit code: $CURL_EXIT"
    echo "Response: $RESPONSE"
    exit 1
fi

echo "Response: $RESPONSE"

if [ "$RESPONSE" = "OK" ]; then
    echo "=== TEST PASSED ==="
else
    echo "=== TEST FAILED: unexpected response ==="
    exit 1
fi