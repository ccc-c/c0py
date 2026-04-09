#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

int main() {
    printf("=== Step by step ===\n");
    
    bignum N, E, M;
    uint8_t n_bytes[] = {129};
    uint8_t e_bytes[] = {3};
    uint8_t m_bytes[] = {5};
    
    printf("Step 1: bn_from_bytes N\n"); fflush(stdout);
    bn_from_bytes(&N, n_bytes, 1);
    printf("N.len=%zu\n", N.len);
    
    printf("Step 2: bn_from_bytes E\n"); fflush(stdout);
    bn_from_bytes(&E, e_bytes, 1);
    printf("E.len=%zu\n", E.len);
    
    printf("Step 3: bn_from_bytes M\n"); fflush(stdout);
    bn_from_bytes(&M, m_bytes, 1);
    printf("M.len=%zu\n", M.len);
    
    printf("Step 4: bn_zero result\n"); fflush(stdout);
    bignum result;
    bn_zero(&result);
    result.words[0] = 1;
    result.len = 1;
    printf("result.len=%zu\n", result.len);
    
    printf("Step 5: bn_div_mod base_mod\n"); fflush(stdout);
    bignum base_mod;
    bn_div_mod(&M, &N, NULL, &base_mod);
    printf("base_mod.len=%zu\n", base_mod.len);
    
    printf("Step 6: bn_div_mod exp_copy\n"); fflush(stdout);
    bignum exp_copy = E;
    printf("exp_copy.len=%zu\n", exp_copy.len);
    
    printf("Step 7: two\n"); fflush(stdout);
    bignum two;
    two.words[0] = 2;
    two.len = 1;
    
    printf("Step 8: first bn_div_mod(&exp_copy, &two)\n"); fflush(stdout);
    bn_div_mod(&exp_copy, &two, NULL, &exp_copy);
    printf("exp_copy.len=%zu, words[0]=%u\n", exp_copy.len, exp_copy.words[0]);
    
    printf("Step 9: second bn_div_mod(&exp_copy, &two)\n"); fflush(stdout);
    bn_div_mod(&exp_copy, &two, NULL, &exp_copy);
    printf("exp_copy.len=%zu, words[0]=%u\n", exp_copy.len, exp_copy.words[0]);
    
    printf("Step 10: third bn_div_mod(&exp_copy, &two)\n"); fflush(stdout);
    bn_div_mod(&exp_copy, &two, NULL, &exp_copy);
    printf("exp_copy.len=%zu, words[0]=%u\n", exp_copy.len, exp_copy.words[0]);
    
    printf("Done\n");
    return 0;
}