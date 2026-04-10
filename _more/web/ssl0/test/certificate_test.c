#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/certificate.h"

static const char BASE64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int test_base64_decode(const char *src, size_t src_len, uint8_t *out, size_t *out_len);
static int test_get_asn1_length(const uint8_t *data, size_t data_len, size_t *len_out, size_t *header_len_out);
static int test_extract_integer(const uint8_t *data, size_t data_len, uint8_t *out, size_t *out_len);

void test_base64_decode_simple() {
    printf("test_base64_decode_simple...\n");
    
    const char *input = "SGVsbG8=";
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_base64_decode(input, strlen(input), output, &out_len);
    assert(ret == 0);
    assert(out_len == 5);
    assert(memcmp(output, "Hello", 5) == 0);
    
    printf("  PASSED\n");
}

void test_base64_decode_multiple_blocks() {
    printf("test_base64_decode_multiple_blocks...\n");
    
    const char *input = "QUJD";
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_base64_decode(input, strlen(input), output, &out_len);
    assert(ret == 0);
    assert(out_len == 3);
    assert(memcmp(output, "ABC", 3) == 0);
    
    printf("  PASSED\n");
}

void test_base64_decode_with_padding() {
    printf("test_base64_decode_with_padding...\n");
    
    const char *input = "AAE=";
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_base64_decode(input, strlen(input), output, &out_len);
    assert(ret == 0);
    assert(out_len == 2);
    assert(output[0] == 0x00 && output[1] == 0x01);
    
    printf("  PASSED\n");
}

void test_base64_decode_empty() {
    printf("test_base64_decode_empty...\n");
    
    const char *input = "";
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_base64_decode(input, 0, output, &out_len);
    assert(ret == 0);
    assert(out_len == 0);
    
    printf("  PASSED\n");
}

void test_get_asn1_length_short_form() {
    printf("test_get_asn1_length_short_form...\n");
    
    uint8_t data[] = {0x30, 0x10};
    size_t len, header_len;
    
    int ret = test_get_asn1_length(data, 2, &len, &header_len);
    assert(ret == 0);
    assert(len == 16);
    assert(header_len == 2);
    
    printf("  PASSED\n");
}

void test_get_asn1_length_long_form() {
    printf("test_get_asn1_length_long_form...\n");
    
    uint8_t data[] = {0x30, 0x82, 0x01, 0x00};
    size_t len, header_len;
    
    int ret = test_get_asn1_length(data, 4, &len, &header_len);
    assert(ret == 0);
    assert(len == 256);
    assert(header_len == 4);
    
    printf("  PASSED\n");
}

void test_get_asn1_length_very_long_form() {
    printf("test_get_asn1_length_very_long_form...\n");
    
    uint8_t data[] = {0x30, 0x83, 0x01, 0x00, 0x00};
    size_t len, header_len;
    
    int ret = test_get_asn1_length(data, 5, &len, &header_len);
    assert(ret == 0);
    assert(len == 65536);
    assert(header_len == 5);
    
    printf("  PASSED\n");
}

void test_get_asn1_length_invalid() {
    printf("test_get_asn1_length_invalid...\n");
    
    uint8_t data[] = {0x30};
    size_t len, header_len;
    
    int ret = test_get_asn1_length(data, 1, &len, &header_len);
    assert(ret == -1);
    
    printf("  PASSED\n");
}

void test_extract_integer_simple() {
    printf("test_extract_integer_simple...\n");
    
    uint8_t data[] = {0x02, 0x03, 0x01, 0x02, 0x03};
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_extract_integer(data, 5, output, &out_len);
    assert(ret == 0);
    assert(out_len == 3);
    assert(memcmp(output, "\x01\x02\x03", 3) == 0);
    
    printf("  PASSED\n");
}

void test_extract_integer_with_leading_zero() {
    printf("test_extract_integer_with_leading_zero...\n");
    
    uint8_t data[] = {0x02, 0x04, 0x00, 0x01, 0x02, 0x03};
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_extract_integer(data, 6, output, &out_len);
    assert(ret == 0);
    assert(out_len == 3);
    assert(memcmp(output, "\x01\x02\x03", 3) == 0);
    
    printf("  PASSED\n");
}

void test_extract_integer_not_integer() {
    printf("test_extract_integer_not_integer...\n");
    
    uint8_t data[] = {0x30, 0x03, 0x01, 0x02, 0x03};
    uint8_t output[16];
    size_t out_len;
    
    int ret = test_extract_integer(data, 5, output, &out_len);
    assert(ret == -1);
    
    printf("  PASSED\n");
}

void test_x509_parse_valid_cert() {
    printf("test_x509_parse_valid_cert...\n");
    
    FILE *f = fopen("./https/cert.pem", "r");
    if (!f) {
        fprintf(stderr, "Cannot open ./https/cert.pem\n");
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
    x509_cert cert;
    int ret = x509_parse_from_pem(pem, &cert);
    assert(ret == 0);
    assert(cert.public_key.n_len == 256);
    assert(cert.public_key.e_len == 3);
    assert(cert.public_key.n[0] == 0xa5);
    
    x509_free(&cert);
    free(pem);
    
    printf("  PASSED\n");
}

void test_x509_get_public_key() {
    printf("test_x509_get_public_key...\n");
    
    FILE *f = fopen("./https/cert.pem", "r");
    if (!f) {
        fprintf(stderr, "Cannot open ./https/cert.pem\n");
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
    x509_cert cert;
    int ret = x509_parse_from_pem(pem, &cert);
    assert(ret == 0);
    
    uint8_t n[256], e[4];
    size_t n_len, e_len;
    ret = x509_get_public_key(&cert, n, &n_len, e, &e_len);
    assert(ret == 0);
    assert(n_len == 256);
    assert(e_len == 3);
    
    x509_free(&cert);
    free(pem);
    
    printf("  PASSED\n");
}

void test_x509_parse_invalid_pem() {
    printf("test_x509_parse_invalid_pem...\n");
    
    const char *invalid_pem = "INVALID";
    x509_cert cert;
    int ret = x509_parse_from_pem(invalid_pem, &cert);
    assert(ret != 0);
    
    printf("  PASSED\n");
}

void test_x509_parse_no_begin() {
    printf("test_x509_parse_no_begin...\n");
    
    const char *invalid_pem = "-----END CERTIFICATE-----\nMII...\n-----END CERTIFICATE-----";
    x509_cert cert;
    int ret = x509_parse_from_pem(invalid_pem, &cert);
    assert(ret != 0);
    
    printf("  PASSED\n");
}

void test_x509_free() {
    printf("test_x509_free...\n");
    
    x509_cert cert;
    cert.public_key.n_len = 100;
    cert.public_key.e_len = 3;
    
    x509_free(&cert);
    
    assert(cert.public_key.n_len == 0);
    assert(cert.public_key.e_len == 0);
    
    printf("  PASSED\n");
}

static int test_base64_decode(const char *src, size_t src_len, uint8_t *out, size_t *out_len) {
    size_t i, j = 0;
    int val;
    int bits = 0;
    int buffer = 0;
    
    for (i = 0; i < src_len; i++) {
        if (src[i] == '=') continue;
        if (src[i] == '\n' || src[i] == '\r') continue;
        if (src[i] == ' ') continue;
        
        const char *p = strchr(BASE64_TABLE, src[i]);
        if (!p) continue;
        
        val = p - BASE64_TABLE;
        buffer = (buffer << 6) | val;
        bits += 6;
        
        if (bits >= 8) {
            bits -= 8;
            out[j++] = (buffer >> bits) & 0xFF;
        }
    }
    
    *out_len = j;
    return 0;
}

static int test_get_asn1_length(const uint8_t *data, size_t data_len, size_t *len_out, size_t *header_len_out) {
    if (data_len < 2) return -1;
    
    if (data[1] < 0x80) {
        *len_out = data[1];
        *header_len_out = 2;
        return 0;
    }
    
    size_t num_bytes = data[1] & 0x7f;
    if (data_len < 2 + num_bytes) return -1;
    
    *len_out = 0;
    for (size_t i = 0; i < num_bytes; i++) {
        *len_out = (*len_out << 8) | data[2 + i];
    }
    *header_len_out = 2 + num_bytes;
    return 0;
}

static int test_extract_integer(const uint8_t *data, size_t data_len, uint8_t *out, size_t *out_len) {
    if (data_len < 2 || data[0] != 0x02) return -1;
    
    size_t len, header_len;
    if (test_get_asn1_length(data, data_len, &len, &header_len) != 0) return -1;
    if (data_len < header_len + len) return -1;
    
    size_t skip = 0;
    if (len > 0 && data[header_len] == 0x00) {
        skip = 1;
        len--;
    }
    
    memcpy(out, data + header_len + skip, len);
    *out_len = len;
    return 0;
}

int main() {
    printf("=== Certificate Tests ===\n\n");
    
    printf("--- base64_decode tests ---\n");
    test_base64_decode_simple();
    test_base64_decode_multiple_blocks();
    test_base64_decode_with_padding();
    test_base64_decode_empty();
    
    printf("\n--- get_asn1_length tests ---\n");
    test_get_asn1_length_short_form();
    test_get_asn1_length_long_form();
    test_get_asn1_length_very_long_form();
    test_get_asn1_length_invalid();
    
    printf("\n--- extract_integer tests ---\n");
    test_extract_integer_simple();
    test_extract_integer_with_leading_zero();
    test_extract_integer_not_integer();
    
    printf("\n--- x509 tests ---\n");
    test_x509_parse_valid_cert();
    test_x509_get_public_key();
    test_x509_parse_invalid_pem();
    test_x509_parse_no_begin();
    test_x509_free();
    
    printf("\n=== All Certificate Tests Passed ===\n");
    return 0;
}