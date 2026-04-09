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

    printf("\n=== Larger Bignum Test (64-bit) ===\n");
    uint8_t a64[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    uint8_t b64[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    bn_from_bytes(&a, a64, 8);
    bn_from_bytes(&b, b64, 8);
    
    bn_add(&r, &a, &b);
    bn_to_bytes(&r, out, &len);
    printf("a + b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 235623789fbc8956)\n");
    
    bn_sub(&r, &a, &b);
    bn_to_bytes(&r, out, &len);
    printf("a - b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 0123456780123468)\n");
    
    bn_mul(&r, &a, &b);
    bn_to_bytes(&r, out, &len);
    printf("a * b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf("\n");
    
    bn_div_mod(&a, &b, &q, &rem);
    bn_to_bytes(&q, out, &len);
    printf("a / b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 01)\n");
    bn_to_bytes(&rem, out, &len);
    printf("a %% b = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf("\n");

    printf("\n=== Mod Exp Test (3^15 mod 100) ===\n");
    uint8_t base3[] = {3};
    uint8_t exp15[] = {15};
    uint8_t mod100[] = {100};
    bignum base, exp, mod, result;
    bn_from_bytes(&base, base3, 1);
    bn_from_bytes(&exp, exp15, 1);
    bn_from_bytes(&mod, mod100, 1);
    bn_mod_exp(&result, &base, &exp, &mod);
    bn_to_bytes(&result, out, &len);
    printf("3^15 mod 100 = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 07)\n");

    printf("\n=== Mod Exp Test (7^103 mod 143) ===\n");
    uint8_t base7[] = {7};
    uint8_t exp103[] = {103};
    uint8_t mod143[] = {0x8f};
    bn_from_bytes(&base, base7, 1);
    bn_from_bytes(&exp, exp103, 1);
    bn_from_bytes(&mod, mod143, 1);
    bn_mod_exp(&result, &base, &exp, &mod);
    bn_to_bytes(&result, out, &len);
    printf("7^103 mod 143 = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 7b)\n");

    printf("\n=== Larger Mod Exp Test (2^1000 mod 997) ===\n");
    uint8_t base2[] = {2};
    uint8_t exp1000[] = {0xe8, 0x03};
    uint8_t mod997[] = {0xe5, 0x03};
    bn_from_bytes(&base, base2, 1);
    bn_from_bytes(&exp, exp1000, 2);
    bn_from_bytes(&mod, mod997, 2);
    bn_mod_exp(&result, &base, &exp, &mod);
    bn_to_bytes(&result, out, &len);
    printf("2^1000 mod 997 = ");
    for (size_t i = 0; i < len; i++) printf("%02x", out[i]);
    printf(" (expected 10)\n");

    printf("\n=== All bignum tests passed ===\n");
    return 0;
}