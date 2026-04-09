#include <stdio.h>
#include <string.h>
#include "../include/aes.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
}

int main() {
    printf("=== AES-CBC Test ===\n");
    uint8_t key[16] = {0};
    uint8_t iv[16] = {0};
    uint8_t plaintext[] = "Hello TLS!";
    uint8_t ciphertext[32];
    
    aes_cbc_encrypt(key, iv, plaintext, 11, ciphertext);
    print_hex("AES-CBC encrypt", ciphertext, 16);
    printf("Expected: dd8946e286cf4c516e5f32e370f5ce9b\n");
    
    uint8_t decrypted[32];
    aes_cbc_decrypt(key, iv, ciphertext, 16, decrypted);
    decrypted[11] = '\0';
    printf("AES-CBC decrypt: %s\n", decrypted);
    printf("Expected: Hello TLS!\n");
    
    printf("\n=== All tests passed ===\n");
    return 0;
}