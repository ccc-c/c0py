#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

int main() {
    printf("=== Quick Test ===\n");
    
    bignum a, b, q, rem;
    uint8_t a_bytes[] = {10};
    uint8_t b_bytes[] = {3};
    
    printf("Setting up a=10, b=3...\n");
    bn_from_bytes(&a, a_bytes, 1);
    bn_from_bytes(&b, b_bytes, 1);
    
    printf("Calling bn_div_mod...\n");
    fflush(stdout);
    bn_div_mod(&a, &b, &q, &rem);
    printf("Done! q=%u, rem=%u\n", q.words[0], rem.words[0]);
    
    return 0;
}