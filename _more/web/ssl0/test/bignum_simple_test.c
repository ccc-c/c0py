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
    printf("=== Simple Bignum Test ===\n");
    
    bignum a, b, r;
    uint8_t out[16];
    size_t len;
    
    uint8_t a_bytes[] = {10};
    uint8_t b_bytes[] = {3};
    
    printf("bn_from_bytes a=10...\n");
    bn_from_bytes(&a, a_bytes, 1);
    printf("a.len=%zu, a.words[0]=%u\n", a.len, a.words[0]);
    
    printf("bn_from_bytes b=3...\n");
    bn_from_bytes(&b, b_bytes, 1);
    printf("b.len=%zu, b.words[0]=%u\n", b.len, b.words[0]);
    
    printf("bn_add...\n");
    bn_add(&r, &a, &b);
    printf("r.len=%zu, r.words[0]=%u\n", r.len, r.words[0]);
    
    bn_to_bytes(&r, out, &len);
    printf("a + b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 0d)\n");
    
    printf("\n=== Test done ===\n");
    return 0;
}