#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

int main() {
    printf("=== Testing div ===\n");
    
    bignum M, N;
    uint8_t m_bytes[] = {5};
    uint8_t n_bytes[] = {129};
    
    bn_from_bytes(&M, m_bytes, 1);
    bn_from_bytes(&N, n_bytes, 1);
    
    printf("Calling bn_div_mod...\n");
    fflush(stdout);
    bignum q, rem;
    bn_div_mod(&M, &N, &q, &rem);
    printf("bn_div_mod done\n");
    
    return 0;
}