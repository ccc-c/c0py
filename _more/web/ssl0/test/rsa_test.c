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
    } else {
        printf("\n=== RSA test FAILED ===\n");
        return 1;
    }

    printf("\n=== Larger RSA Test (p=11, q=13, n=143, e=7, d=103) ===\n");
    uint8_t n2[] = {0x8f};
    uint8_t e2[] = {7};
    uint8_t plain2[] = {5};
    uint8_t cipher2[16];
    size_t cipher2_len;
    uint8_t d2[] = {103};
    uint8_t recovered2[16];
    size_t plain2_len;

    ret = rsa_encrypt(n2, 1, e2, 1, plain2, 1, cipher2, &cipher2_len);
    if (ret != 0) {
        printf("rsa_encrypt failed: %d\n", ret);
        return 1;
    }
    print_hex("Ciphertext", cipher2, cipher2_len);

    ret = rsa_decrypt(n2, 1, d2, 1, cipher2, cipher2_len, recovered2, &plain2_len);
    if (ret != 0) {
        printf("rsa_decrypt failed: %d\n", ret);
        return 1;
    }
    print_hex("Decrypted", recovered2, plain2_len);

    int pass = 1;
    if (plain2_len != 1 || recovered2[0] != 5) {
        printf("Large RSA test FAILED\n");
        pass = 0;
    } else {
        printf("Large RSA test passed\n");
    }

    printf("\n=== Larger RSA Test 2 (p=61, q=53, n=3233, e=17, d=2753) ===\n");
    uint8_t n3[] = {0xa1, 0x0c};
    uint8_t e3[] = {0x11};
    uint8_t plain3[] = {65};
    uint8_t cipher3[32];
    size_t cipher3_len;
    uint8_t d3[] = {0xc1, 0x0a};
    uint8_t recovered3[32];
    size_t plain3_len;

    ret = rsa_encrypt(n3, 2, e3, 1, plain3, 1, cipher3, &cipher3_len);
    if (ret != 0) {
        printf("rsa_encrypt failed: %d\n", ret);
        return 1;
    }
    print_hex("Ciphertext", cipher3, cipher3_len);

    ret = rsa_decrypt(n3, 2, d3, 2, cipher3, cipher3_len, recovered3, &plain3_len);
    if (ret != 0) {
        printf("rsa_decrypt failed: %d\n", ret);
        return 1;
    }
    print_hex("Decrypted", recovered3, plain3_len);

    if (plain3_len != 1 || recovered3[0] != 65) {
        printf("Large RSA test 2 FAILED\n");
        pass = 0;
    } else {
        printf("Large RSA test 2 passed\n");
    }

    if (pass) {
        printf("\n=== All RSA tests passed ===\n");
        return 0;
    } else {
        printf("\n=== RSA tests FAILED ===\n");
        return 1;
    }
}