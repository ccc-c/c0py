#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/bignum.h"

void print_bn(const char *label, bignum *a) {
    printf("%s: len=%zu, words[0]=%u\n", label, a->len, a->words[0]);
}

int main() {
    printf("=== Manual bn_mod_exp ===\n");
    
    bignum N, E, M;
    uint8_t n_bytes[] = {129};
    uint8_t e_bytes[] = {3};
    uint8_t m_bytes[] = {5};
    
    bn_from_bytes(&N, n_bytes, 1);
    bn_from_bytes(&E, e_bytes, 1);
    bn_from_bytes(&M, m_bytes, 1);
    print_bn("N", &N);
    print_bn("E", &E);
    print_bn("M", &M);
    
    bignum result;
    bn_zero(&result);
    result.words[0] = 1;
    result.len = 1;
    print_bn("result init", &result);
    
    bignum base_mod;
    printf("Computing base_mod = M mod N...\n");
    bn_div_mod(&M, &N, NULL, &base_mod);
    print_bn("base_mod", &base_mod);
    
    bignum exp_copy = E;
    print_bn("exp_copy init", &exp_copy);
    
    bignum two;
    two.words[0] = 2;
    two.len = 1;
    
    int iter = 0;
    while (exp_copy.len > 0 && !(exp_copy.len == 1 && exp_copy.words[0] == 0)) {
        printf("--- iter %d ---\n", iter);
        print_bn("exp_copy", &exp_copy);
        printf("exp_copy.words[0] & 1 = %u\n", exp_copy.words[0] & 1);
        
        if (exp_copy.words[0] & 1) {
            printf("Multiplying result by base_mod...\n");
            bn_mod_mul(&result, &result, &base_mod, &N);
            print_bn("result after mul", &result);
        }
        
        printf("Squaring base_mod...\n");
        bn_mod_mul(&base_mod, &base_mod, &base_mod, &N);
        print_bn("base_mod after sq", &base_mod);
        
        printf("Dividing exp_copy by 2...\n");
        bn_div_mod(&exp_copy, &two, NULL, &exp_copy);
        print_bn("exp_copy after div", &exp_copy);
        
        iter++;
        if (iter > 10) {
            printf("Too many iterations, breaking\n");
            break;
        }
    }
    
    printf("Final result: len=%zu, words[0]=%u\n", result.len, result.words[0]);
    
    return 0;
}