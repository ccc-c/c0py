#include <stdio.h>
#include <string.h>
#include "../include/rsa.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
}

int main() {
    printf("=== RSA Test ===\n");
    
    uint8_t n[] = {55};
    uint8_t e[] = {3};
    uint8_t plain[] = {2};
    uint8_t cipher[16];
    size_t cipher_len;
    
    printf("Testing: 2^3 mod 55\n");
    printf("Expected: 8\n\n");
    
    int ret = rsa_encrypt(n, 1, e, 1, plain, 1, cipher, &cipher_len);
    if (ret != 0) {
        printf("rsa_encrypt failed: %d\n", ret);
        return 1;
    }
    print_hex("Ciphertext", cipher, cipher_len);
    
    uint8_t d[] = {27};
    uint8_t recovered[16];
    size_t plain_len;
    
    printf("\nTesting: 8^27 mod 55 (should be 2)\n");
    ret = rsa_decrypt(n, 1, d, 1, cipher, cipher_len, recovered, &plain_len);
    if (ret != 0) {
        printf("rsa_decrypt failed: %d\n", ret);
        return 1;
    }
    
    printf("Decrypted: ");
    for (size_t i = 0; i < plain_len; i++) printf("%02x", recovered[i]);
    printf("\n");
    
    if (plain_len > 0 && recovered[plain_len-1] == 2) {
        printf("\n=== RSA test passed ===\n");
        return 0;
    } else {
        printf("\n=== RSA test FAILED ===\n");
        return 1;
    }
}