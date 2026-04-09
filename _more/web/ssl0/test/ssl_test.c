#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/ssl.h"
#include "../include/sha.h"
#include "../include/aes.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
}

int main() {
    printf("=== SSL Test ===\n");
    
    ssl_context ctx;
    ssl_context_init(&ctx);
    
    uint8_t client_random[48] = "123456789012345678901234567890123456789012345678";
    uint8_t server_random[48] = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKL";
    uint8_t pre_master_secret[] = {0x03, 0x01};
    
    printf("Testing: ssl_compute_master_secret\n");
    uint8_t master_secret[48] = {0};
    int ret = ssl_compute_master_secret(pre_master_secret, 2,
                                         client_random, 48,
                                         server_random, 48,
                                         master_secret);
    if (ret != 0) {
        printf("ssl_compute_master_secret failed: %d\n", ret);
        return 1;
    }
    print_hex("Master Secret", master_secret, 48);
    
    printf("\nTesting: ssl_derive_keys\n");
    uint8_t client_write_key[16], server_write_key[16];
    uint8_t client_write_iv[16], server_write_iv[16];
    size_t client_key_len, server_key_len, client_iv_len, server_iv_len;
    ret = ssl_derive_keys(master_secret,
                          client_random, 48,
                          server_random, 48,
                          client_write_key, &client_key_len,
                          server_write_key, &server_key_len,
                          client_write_iv, &client_iv_len,
                          server_write_iv, &server_iv_len);
    if (ret != 0) {
        printf("ssl_derive_keys failed: %d\n", ret);
        return 1;
    }
    print_hex("Client Write Key", client_write_key, 16);
    print_hex("Server Write Key", server_write_key, 16);
    print_hex("Client Write IV", client_write_iv, 16);
    print_hex("Server Write IV", server_write_iv, 16);
    
    printf("\nTesting: ssl_handshake_client\n");
    ret = ssl_handshake_client(&ctx, NULL, 0, client_random, 48, server_random, 48, pre_master_secret, 2);
    if (ret != 0) {
        printf("ssl_handshake_client failed: %d\n", ret);
        return 1;
    }
    print_hex("Master Secret in ctx", ctx.master_secret, 48);
    printf("has_keys = %d\n", ctx.has_keys);
    
    printf("\n=== Direct AES-CBC Test ===\n");
    uint8_t test_plain[16] = "Hello TLS!";
    for (int i = 11; i < 16; i++) test_plain[i] = 5;
    uint8_t test_key[16] = {0};
    uint8_t test_iv[16] = {0};
    uint8_t test_enc[16], test_dec[16];
    aes_cbc_encrypt(test_key, test_iv, test_plain, 16, test_enc);
    print_hex("Encrypted", test_enc, 16);
    aes_cbc_decrypt(test_key, test_iv, test_enc, 16, test_dec);
    test_dec[15] = '\0';
    printf("Decrypted: %s\n", test_dec);
    
    printf("\nTesting: ssl_encrypt_record\n");
    uint8_t plaintext[] = "Hello TLS!";
    uint8_t iv_before[16];
    memcpy(iv_before, ctx.client_write_iv, 16);
    uint8_t ciphertext[64];
    size_t cipher_len;
    ret = ssl_encrypt_record(&ctx, SSL_CONTENT_TYPE_APPLICATION_DATA,
                             plaintext, 11,
                             ciphertext, &cipher_len);
    if (ret != 0) {
        printf("ssl_encrypt_record failed: %d\n", ret);
        return 1;
    }
    print_hex("Ciphertext", ciphertext, cipher_len);
    
    printf("\nTesting: ssl_decrypt_record (using same keys)\n");
    uint8_t decrypted[64];
    size_t plain_len;
    uint8_t content_type;
    memcpy(ctx.server_write_key, ctx.client_write_key, 16);
    memcpy(ctx.server_write_iv, iv_before, 16);
    
    ret = ssl_decrypt_record(&ctx, ciphertext, cipher_len,
                             &content_type, decrypted, &plain_len);
    if (ret != 0) {
        printf("ssl_decrypt_record failed: %d\n", ret);
        return 1;
    }
    decrypted[plain_len] = '\0';
    printf("Content Type: %d\n", content_type);
    printf("Decrypted: %s\n", decrypted);
    
    if (plain_len != 11 || memcmp(decrypted, plaintext, plain_len) != 0) {
        printf("\n=== SSL test FAILED ===\n");
        return 1;
    }
    
    ssl_free(&ctx);
    
    printf("\n=== All SSL tests passed ===\n");
    return 0;
}