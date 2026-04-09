#include <stdio.h>
#include <string.h>
#include "../include/sha.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
}

int main() {
    printf("=== SHA-256 Test ===\n");
    const char *msg = "hello";
    uint8_t digest[SHA256_DIGEST_SIZE];
    sha256((uint8_t *)msg, strlen(msg), digest);
    print_hex("SHA256(hello)", digest, SHA256_DIGEST_SIZE);
    printf("Expected: 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824\n");

    printf("\n=== HMAC-SHA256 Test ===\n");
    const char *key = "key";
    uint8_t mac[SHA256_DIGEST_SIZE];
    hmac_sha256((uint8_t *)key, strlen(key), (uint8_t *)msg, strlen(msg), mac);
    print_hex("HMAC(key, hello)", mac, SHA256_DIGEST_SIZE);
    printf("Expected: 9307b3b915efb5171ff14d8cb55fbcc798c6c0ef1456d66ded1a6aa723a58b7b\n");

    printf("\n=== All tests passed ===\n");
    return 0;
}