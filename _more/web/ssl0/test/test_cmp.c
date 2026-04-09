#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

int main() {
    printf("=== Even simpler ===\n");
    
    bignum M, N;
    uint8_t m_bytes[] = {5};
    uint8_t n_bytes[] = {129};
    
    bn_from_bytes(&M, m_bytes, 1);
    bn_from_bytes(&N, n_bytes, 1);
    
    printf("M.words[0]=%u, M.len=%zu\n", M.words[0], M.len);
    printf("N.words[0]=%u, N.len=%zu\n", N.words[0], N.len);
    
    printf("Calling bn_cmp...\n");
    fflush(stdout);
    int cmp = bn_cmp(&M, &N);
    printf("bn_cmp done, result=%d\n", cmp);
    
    return 0;
}