#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

void print_bn(const char *label, bignum *a) {
    printf("%s: len=%zu, words[0]=%u\n", label, a->len, a->words[0]);
}

int main() {
    printf("=== Debug bn_div_mod ===\n");
    
    bignum a, b, q, rem;
    uint8_t out[16];
    size_t len;
    
    uint8_t a_bytes[] = {10};
    uint8_t b_bytes[] = {3};
    
    bn_from_bytes(&a, a_bytes, 1);
    bn_from_bytes(&b, b_bytes, 1);
    print_bn("a", &a);
    print_bn("b", &b);
    
    printf("Calling bn_div_mod...\n");
    bn_div_mod(&a, &b, &q, &rem);
    print_bn("q", &q);
    print_bn("rem", &rem);
    
    bn_to_bytes(&q, out, &len);
    printf("a / b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 03)\n");
    
    bn_to_bytes(&rem, out, &len);
    printf("a %% b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 01)\n");
    
    return 0;
}