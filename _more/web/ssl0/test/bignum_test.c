#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
}

int main() {
    printf("=== Bignum Test ===\n");
    bignum a, b, r;
    uint8_t out[16];
    size_t len;
    
    uint8_t a_bytes[] = {10};
    uint8_t b_bytes[] = {3};
    bn_from_bytes(&a, a_bytes, 1);
    bn_from_bytes(&b, b_bytes, 1);
    printf("a = 10, b = 3\n");
    
    bn_add(&r, &a, &b);
    bn_to_bytes(&r, out, &len);
    printf("a + b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 0d)\n");
    
    bn_sub(&r, &a, &b);
    bn_to_bytes(&r, out, &len);
    printf("a - b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 07)\n");
    
    bn_mul(&r, &a, &b);
    bn_to_bytes(&r, out, &len);
    printf("a * b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 1e)\n");
    
    bignum q, rem;
    bn_div_mod(&a, &b, &q, &rem);
    bn_to_bytes(&q, out, &len);
    printf("a / b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 03)\n");
    bn_to_bytes(&rem, out, &len);
    printf("a %% b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 01)\n");

    printf("\n=== Bignum tests passed ===\n");
    return 0;
}