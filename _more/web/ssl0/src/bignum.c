#include "../include/bignum.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const bignum temp_zero = {.words = {0}, .len = 0};

void bn_zero(bignum *a) {
    a->len = 0;
    for (size_t i = 0; i < MAX_BIGNUM_WORDS; i++) a->words[i] = 0;
}

void bn_from_bytes(bignum *a, const uint8_t *bytes, size_t len) {
    bn_zero(a);
    for (size_t i = 0; i < len && i < MAX_BIGNUM_WORDS * 4; i++) {
        size_t word_idx = i / 4;
        size_t byte_idx = i % 4;
        a->words[word_idx] |= ((uint32_t)bytes[i] << (byte_idx * 8));
    }
    a->len = (len + 3) / 4;
    if (a->len == 0) a->len = 1;
    while (a->len > 0 && a->words[a->len - 1] == 0) a->len--;
}

void bn_to_bytes(const bignum *a, uint8_t *bytes, size_t *len) {
    *len = a->len * 4;
    if (*len == 0) *len = 1;
    for (size_t i = 0; i < *len; i++) {
        size_t word_idx = i / 4;
        size_t byte_idx = i % 4;
        bytes[i] = (a->words[word_idx] >> (byte_idx * 8)) & 0xff;
    }
    while (*len > 0 && bytes[*len - 1] == 0) (*len)--;
    if (*len == 0) *len = 1;
}

int bn_cmp(const bignum *a, const bignum *b) {
    if (a->len != b->len) return a->len > b->len ? 1 : -1;
    for (size_t i = a->len; i > 0; i--) {
        if (a->words[i-1] != b->words[i-1]) {
            return a->words[i-1] > b->words[i-1] ? 1 : -1;
        }
    }
    return 0;
}

void bn_add(bignum *r, const bignum *a, const bignum *b) {
    uint64_t carry = 0;
    size_t max_len = a->len > b->len ? a->len : b->len;
    for (size_t i = 0; i < max_len || carry; i++) {
        uint64_t sum = carry + (i < a->len ? a->words[i] : 0) + (i < b->len ? b->words[i] : 0);
        r->words[i] = sum & 0xffffffff;
        carry = sum >> 32;
    }
    r->len = max_len;
    if (carry) r->words[r->len++] = (uint32_t)carry;
    while (r->len > 0 && r->words[r->len - 1] == 0) r->len--;
}

void bn_sub(bignum *r, const bignum *a, const bignum *b) {
    int64_t borrow = 0;
    size_t max_len = a->len > b->len ? a->len : b->len;
    for (size_t i = 0; i < max_len; i++) {
        int64_t diff = (int64_t)(i < a->len ? a->words[i] : 0) - (int64_t)(i < b->len ? b->words[i] : 0) - borrow;
        if (diff < 0) {
            diff += 0x100000000LL;
            borrow = 1;
        } else {
            borrow = 0;
        }
        if (i < MAX_BIGNUM_WORDS) r->words[i] = (uint32_t)diff;
    }
    r->len = max_len;
    if (r->len > MAX_BIGNUM_WORDS) r->len = MAX_BIGNUM_WORDS;
    while (r->len > 0 && r->words[r->len - 1] == 0) r->len--;
    if (r->len == 0) r->len = 1;
}

void bn_mul(bignum *r, const bignum *a, const bignum *b) {
    bn_zero(r);
    for (size_t i = 0; i < a->len; i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b->len || carry; j++) {
            if (i + j >= MAX_BIGNUM_WORDS) break;
            uint64_t prod = (uint64_t)a->words[i] * (j < b->len ? b->words[j] : 0) + r->words[i + j] + carry;
            r->words[i + j] = prod & 0xffffffff;
            carry = prod >> 32;
        }
    }
    r->len = a->len + b->len;
    if (r->len == 0) r->len = 1;
    if (r->len > MAX_BIGNUM_WORDS) r->len = MAX_BIGNUM_WORDS;
    while (r->len > 1 && r->words[r->len - 1] == 0) r->len--;
}

void bn_div_mod(const bignum *a, const bignum *b, bignum *q, bignum *rem) {
    bignum a_copy;
    const bignum *a_ptr = a;
    
    if (a == rem) {
        bn_copy(&a_copy, a); 
        a_ptr = &a_copy;
    }
    
    if (q) bn_zero(q);
    if (rem) bn_zero(rem);
    
    if (bn_cmp(a_ptr, b) < 0) {
        if (rem) bn_copy(rem, a_ptr);
        return;
    }
    
    int a_bitlen = 0;
    for (int i = MAX_BIGNUM_WORDS - 1; i >= 0; i--) {
        if (a_ptr->words[i] != 0) {
            for (int j = 31; j >= 0; j--) {
                if (a_ptr->words[i] & (1U << j)) {
                    a_bitlen = i * 32 + j + 1;
                    break;
                }
            }
            break;
        }
    }
    
    if (rem) {
        for (int bit = a_bitlen - 1; bit >= 0; bit--) {
            for (int i = MAX_BIGNUM_WORDS - 1; i > 0; i--) {
                rem->words[i] = (rem->words[i] << 1) | (rem->words[i - 1] >> 31);
            }
            rem->words[0] <<= 1;
            
            uint32_t word = a_ptr->words[bit / 32];
            uint32_t word_bit = (word >> (bit % 32)) & 1;
            rem->words[0] |= word_bit;
            
            rem->len = MAX_BIGNUM_WORDS;
            while (rem->len > 1 && rem->words[rem->len - 1] == 0) rem->len--;
            
            if (q) {
                for (int i = MAX_BIGNUM_WORDS - 1; i > 0; i--) {
                    q->words[i] = (q->words[i] << 1) | (q->words[i - 1] >> 31);
                }
                q->words[0] <<= 1;
            }
            
            int cmp = bn_cmp(rem, b);
            while (cmp >= 0) {
                bn_sub(rem, rem, b);
                if (q) q->words[0] |= 1;
                cmp = bn_cmp(rem, b);
            }
        }
    }
    
    if (q) {
        int last_nonzero = -1;
        for (int i = MAX_BIGNUM_WORDS - 1; i >= 0; i--) {
            if (q->words[i] != 0) {
                last_nonzero = i;
                break;
            }
        }
        q->len = (last_nonzero >= 0) ? last_nonzero + 1 : 1;
    }
}

void bn_ext_gcd(const bignum *a, const bignum *b, bignum *gcd, bignum *x, bignum *y) {
    bignum temp_zero;
    bn_zero(&temp_zero);
    
    if (bn_cmp(b, &temp_zero) == 0) {
        if (gcd) bn_copy(gcd, a);
        if (x) {
            bn_zero(x);
            x->words[0] = 1;
            x->len = 1;
        }
        if (y) bn_zero(y);
        return;
    }
    
    bignum rem, q;
    bn_div_mod(a, b, &q, &rem);
    
    bignum x1, y1;
    bn_ext_gcd(b, &rem, gcd, &x1, &y1);
    
    bignum temp_qy1;
    bn_mul(&temp_qy1, &q, &y1);
    
    if (x) {
        bn_copy(x, &y1);
    }
    if (y) {
        bn_sub(y, &x1, &temp_qy1); 
    }
}

void bn_mod_mul(bignum *r, const bignum *a, const bignum *b, const bignum *mod) {
    bn_mul(r, a, b);
    bn_div_mod(r, mod, NULL, r);
}

void bn_copy(bignum *dst, const bignum *src) {
    for (size_t i = 0; i < MAX_BIGNUM_WORDS; i++) {
        dst->words[i] = src->words[i];
    }
    dst->len = src->len;
}

void bn_mod_exp(bignum *r, const bignum *base, const bignum *exp, const bignum *mod) {
    bignum result;
    bn_zero(&result);
    result.words[0] = 1;
    result.len = 1;
    
    bignum base_copy;
    bn_copy(&base_copy, base); 
    
    bignum e;
    bn_copy(&e, exp); 
    
    while (e.len > 0 && !(e.len == 1 && e.words[0] == 0)) {
        if (e.words[0] & 1) {
            bignum temp;
            bn_mul(&temp, &result, &base_copy);
            bn_div_mod(&temp, mod, NULL, &temp);
            bn_copy(&result, &temp);
        }
        
        bignum sq;
        bn_mul(&sq, &base_copy, &base_copy);
        bn_div_mod(&sq, mod, NULL, &sq);
        bn_copy(&base_copy, &sq);
        
        uint32_t carry = 0;
        for (int i = e.len - 1; i >= 0; i--) {
            uint32_t old = e.words[i];
            e.words[i] = (old >> 1) | (carry << 31);
            carry = old & 1;
        }
        while (e.len > 1 && e.words[e.len - 1] == 0) {
            e.len--;
        }
    }
    
    bn_copy(r, &result);
}