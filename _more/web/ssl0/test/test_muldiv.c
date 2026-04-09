#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

int main() {
    bignum result, base_copy, mod;
    uint8_t r_bytes[] = {5};
    uint8_t b_bytes[] = {5};
    uint8_t n_bytes[] = {129};
    
    bn_from_bytes(&result, r_bytes, 1);
    bn_from_bytes(&base_copy, b_bytes, 1);
    bn_from_bytes(&mod, n_bytes, 1);
    
    printf("Before mul: result.words[0]=%u\n", result.words[0]);
    bn_mul(&result, &result, &base_copy);
    printf("After mul: result.words[0]=%u\n", result.words[0]);
    
    printf("Calling bn_div_mod...\n");
    bn_div_mod(&result, &mod, NULL, &result);
    printf("After div: result.words[0]=%u\n", result.words[0]);
    
    return 0;
}