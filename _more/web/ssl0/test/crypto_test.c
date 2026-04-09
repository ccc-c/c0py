#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/crypto.h"

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
    const char *hmac_key = "key";
    uint8_t mac[SHA256_DIGEST_SIZE];
    hmac_sha256((uint8_t *)hmac_key, strlen(hmac_key), (uint8_t *)msg, strlen(msg), mac);
    print_hex("HMAC(key, hello)", mac, SHA256_DIGEST_SIZE);

    printf("\n=== TLS PRF Test ===\n");
    uint8_t secret[] = {0x01, 0x02, 0x03};
    uint8_t seed[] = {0x04, 0x05, 0x06};
    uint8_t prf_output[32];
    tls_prf(secret, 3, "test", seed, 3, prf_output, 32);
    print_hex("TLS-PRF", prf_output, 32);

    printf("\n=== AES-CBC Test ===\n");
    uint8_t aes_key[16] = {0};
    uint8_t iv[16] = {0};
    uint8_t plaintext[] = "Hello TLS!";
    uint8_t ciphertext[32];
    aes_cbc_encrypt(aes_key, iv, plaintext, 11, ciphertext);
    print_hex("AES-CBC encrypt", ciphertext, 16);
    uint8_t decrypted[32];
    aes_cbc_decrypt(aes_key, iv, ciphertext, 16, decrypted);
    decrypted[11] = '\0';
    printf("AES-CBC decrypt: %s\n", decrypted);

    printf("\n=== All tests passed ===\n");
    return 0;
}