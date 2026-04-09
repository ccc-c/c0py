#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

int main() {
    printf("=== Very simple test ===\n");
    
    bignum a, b;
    uint8_t a_bytes[] = {5};
    uint8_t b_bytes[] = {129, 0, 0, 0};  // 129 as 4-byte
    
    printf("Setting up a=5...\n"); fflush(stdout);
    bn_from_bytes(&a, a_bytes, 1);
    printf("a.len=%zu\n", a.len);
    
    printf("Setting up b=129...\n"); fflush(stdout);
    bn_from_bytes(&b, b_bytes, 4);
    printf("b.len=%zu\n", b.len);
    
    printf("bn_cmp(a, b)...\n"); fflush(stdout);
    int cmp = bn_cmp(&a, &b);
    printf("cmp = %d\n", cmp);
    
    printf("bn_div_mod...\n"); fflush(stdout);
    bignum q, rem;
    bn_div_mod(&a, &b, &q, &rem);
    printf("q.len=%zu, rem.len=%zu\n", q.len, rem.len);
    
    printf("Done\n");
    return 0;
}