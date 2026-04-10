#include "../include/ssl.h"
#include "../include/crypto.h"
#include "../include/sha.h"
#include "../include/aes.h"
#include <string.h>

void ssl_context_init(ssl_context *ctx) {
    memset(ctx, 0, sizeof(ssl_context));
    ctx->has_keys = 0;
}

void ssl_free(ssl_context *ctx) {
    memset(ctx, 0, sizeof(ssl_context));
}

int ssl_compute_master_secret(const uint8_t *pre_master_secret, size_t pms_len,
                               const uint8_t *client_random, size_t cr_len,
                               const uint8_t *server_random, size_t sr_len,
                               uint8_t *master_secret) {
    uint8_t seed[256];
    size_t seed_len = 0;
    
    memcpy(seed + seed_len, "master secret", 13);
    seed_len += 13;
    memcpy(seed + seed_len, client_random, cr_len);
    seed_len += cr_len;
    memcpy(seed + seed_len, server_random, sr_len);
    seed_len += sr_len;
    
    tls_prf(pre_master_secret, pms_len, "master secret", seed, seed_len, master_secret, 48);
    return 0;
}

int ssl_derive_keys(const uint8_t *master_secret,
                    const uint8_t *client_random, size_t cr_len,
                    const uint8_t *server_random, size_t sr_len,
                    uint8_t *client_write_key, size_t *client_key_len,
                    uint8_t *server_write_key, size_t *server_key_len,
                    uint8_t *client_write_iv, size_t *client_iv_len,
                    uint8_t *server_write_iv, size_t *server_iv_len) {
    uint8_t seed[256];
    size_t seed_len = 0;
    
    memcpy(seed + seed_len, "key expansion", 13);
    seed_len += 13;
    memcpy(seed + seed_len, server_random, sr_len);
    seed_len += sr_len;
    memcpy(seed + seed_len, client_random, cr_len);
    seed_len += cr_len;
    
    uint8_t key_block[80];
    tls_prf(master_secret, 48, "key expansion", seed, seed_len, key_block, 80);
    
    if (client_key_len) *client_key_len = 16;
    memcpy(client_write_key, key_block, 16);
    
    if (server_key_len) *server_key_len = 16;
    memcpy(server_write_key, key_block + 16, 16);
    
    if (client_iv_len) *client_iv_len = 16;
    memcpy(client_write_iv, key_block + 32, 16);
    
    if (server_iv_len) *server_iv_len = 16;
    memcpy(server_write_iv, key_block + 48, 16);
    
    return 0;
}

int ssl_handshake_client(ssl_context *ctx,
                         const uint8_t *server_cert, size_t server_cert_len,
                         const uint8_t *client_random, size_t client_random_len,
                         const uint8_t *server_random, size_t server_random_len,
                         const uint8_t *pre_master_secret, size_t pms_len) {
    uint8_t master_secret[48];
    int ret = ssl_compute_master_secret(pre_master_secret, pms_len,
                                         client_random, client_random_len,
                                         server_random, server_random_len,
                                         master_secret);
    if (ret != 0) return ret;
    
    ret = ssl_derive_keys(master_secret,
                          client_random, client_random_len,
                          server_random, server_random_len,
                          ctx->client_write_key, NULL,
                          ctx->server_write_key, NULL,
                          ctx->client_write_iv, NULL,
                          ctx->server_write_iv, NULL);
    if (ret != 0) return ret;
    
    memcpy(ctx->master_secret, master_secret, 48);
    ctx->has_keys = 1;
    return 0;
}

int ssl_encrypt_record(ssl_context *ctx,
                       uint8_t content_type,
                       const uint8_t *plaintext, size_t plain_len,
                       uint8_t *ciphertext, size_t *cipher_len) {
    if (!ctx->has_keys) return -1;
    
    if (plain_len > SSL_MAX_PLAINTEXT_LEN) return -2;
    
    uint8_t record_header[5];
    record_header[0] = content_type;
    record_header[1] = 0x03;
    record_header[2] = 0x03;
    record_header[3] = (plain_len >> 8) & 0xff;
    record_header[4] = plain_len & 0xff;
    
    uint8_t padded[SSL_MAX_PLAINTEXT_LEN + 32];
    size_t padded_len = ((plain_len + 16) / 16) * 16;
    memcpy(padded, plaintext, plain_len);
    for (size_t i = plain_len; i < padded_len; i++) {
        padded[i] = (uint8_t)(padded_len - plain_len);
    }
    
    uint8_t *enc_key = ctx->is_server ? ctx->server_write_key : ctx->client_write_key;
    uint8_t *enc_iv = ctx->is_server ? ctx->server_write_iv : ctx->client_write_iv;
    
    uint8_t encrypted[SSL_MAX_PLAINTEXT_LEN + 32];
    aes_cbc_encrypt(enc_key, enc_iv, padded, padded_len, encrypted);
    
    memcpy(ciphertext, record_header, 5);
    memcpy(ciphertext + 5, encrypted, padded_len);
    *cipher_len = padded_len + 5;
    
    memcpy(enc_iv, encrypted + padded_len - 16, 16);
    
    return 0;
}

int ssl_decrypt_record(ssl_context *ctx,
                       const uint8_t *ciphertext, size_t cipher_len,
                       uint8_t *content_type,
                       uint8_t *plaintext, size_t *plain_len) {
    if (!ctx->has_keys) return -1;
    
    if (cipher_len < 5) return -2;
    
    size_t cipher_data_len = cipher_len - 5;
    if (cipher_data_len % 16 != 0) return -3;
    
    uint8_t *dec_key = ctx->is_server ? ctx->client_write_key : ctx->server_write_key;
    uint8_t *dec_iv = ctx->is_server ? ctx->client_write_iv : ctx->server_write_iv;
    
    uint8_t decrypted[SSL_MAX_RECORD_LEN];
    aes_cbc_decrypt(dec_key, dec_iv, ciphertext + 5, cipher_data_len, decrypted);
    
    size_t pad_len = decrypted[cipher_data_len - 1];
    if (pad_len > 16 || pad_len == 0) return -5;
    *plain_len = cipher_data_len - pad_len;
    if (*plain_len > SSL_MAX_PLAINTEXT_LEN) return -4;
    
    memcpy(plaintext, decrypted, *plain_len);
    *content_type = ciphertext[0];
    
    memcpy(dec_iv, ciphertext + 5 + cipher_data_len - 16, 16);
    
    return 0;
}