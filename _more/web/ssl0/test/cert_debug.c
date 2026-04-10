#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/certificate.h"

static int get_asn1_length(const uint8_t *data, size_t data_len, size_t *len_out, size_t *header_len_out) {
    if (data_len < 2) return -1;
    
    if (data[1] < 0x80) {
        *len_out = data[1];
        *header_len_out = 2;
        return 0;
    }
    
    size_t num_bytes = data[1] & 0x7f;
    if (data_len < 2 + num_bytes) return -1;
    
    *len_out = 0;
    for (size_t i = 0; i < num_bytes; i++) {
        *len_out = (*len_out << 8) | data[2 + i];
    }
    *header_len_out = 2 + num_bytes;
    return 0;
}

int main() {
    FILE *f = fopen("./https/cert.pem", "r");
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
    // Base64 decode
    static const char BASE64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    const char *begin_nl = strstr(pem, "-----BEGIN") + strlen("-----BEGIN\n");
    const char *end = strstr(pem, "-----END");
    size_t b64_len = end - begin_nl;
    
    char *b64_clean = (char *)malloc(b64_len + 1);
    size_t j = 0;
    for (size_t i = 0; i < b64_len; i++) {
        if (begin_nl[i] != '\n' && begin_nl[i] != '\r') {
            b64_clean[j++] = begin_nl[i];
        }
    }
    b64_clean[j] = '\0';
    
    uint8_t *der = (uint8_t *)malloc(j * 3 / 4 + 3);
    size_t der_len = 0;
    int val;
    int bits = 0;
    int buffer = 0;
    
    for (size_t i = 0; i < j; i++) {
        if (b64_clean[i] == '=') continue;
        const char *p = strchr(BASE64_TABLE, b64_clean[i]);
        if (!p) continue;
        
        int val = p - BASE64_TABLE;
        buffer = (buffer << 6) | val;
        bits += 6;
        
        if (bits >= 8) {
            bits -= 8;
            der[der_len++] = (buffer >> bits) & 0xFF;
        }
    }
    free(b64_clean);
    
    printf("DER length: %zu\n", der_len);
    printf("DER[0] = 0x%02x (should be 0x30)\n", der[0]);
    
    // Find TBSCertificate
    size_t tbs_len, tbs_header;
    get_asn1_length(der, der_len, &tbs_len, &tbs_header);
    printf("TBSCertificate length: %zu, header: %zu\n", tbs_len, tbs_header);
    
    const uint8_t *tbs = der + tbs_header;
    size_t tbs_content_len, tbs_content_header;
    get_asn1_length(tbs, tbs_len, &tbs_content_len, &tbs_content_header);
    printf("TBS content length: %zu, header: %zu\n", tbs_content_len, tbs_content_header);
    
    const uint8_t *tbs_content = tbs + tbs_content_header;
    printf("tbs_content[0] = 0x%02x\n", tbs_content[0]);
    
    // Find SubjectPublicKeyInfo (SEQUENCE with len > 256)
    size_t pos = 0;
    while (pos + 4 < tbs_content_len) {
        uint8_t tag = tbs_content[pos];
        size_t len, header_len;
        get_asn1_length(tbs_content + pos, tbs_content_len - pos, &len, &header_len);
        
        if (tag == 0x30 && len > 256) {
            printf("Found SubjectPublicKeyInfo at pos %zu, len %zu\n", pos, len);
            const uint8_t *spki = tbs_content + pos + header_len;
            
            // Inside SPKI: AlgorithmIdentifier (SEQUENCE) + BitString
            size_t alg_len, alg_header;
            get_asn1_length(spki, len, &alg_len, &alg_header);
            printf("  SPKI inner: len=%zu, header=%zu, tag=0x%02x\n", alg_len, alg_header, spki[0]);
            
            const uint8_t *alg_content = spki + alg_header;
            printf("  alg_content[0] = 0x%02x (should be 0x30 for AlgorithmIdentifier)\n", alg_content[0]);
            
            // Skip AlgorithmIdentifier to get to BitString
            size_t alg_content_len, alg_content_header;
            get_asn1_length(alg_content, alg_len, &alg_content_len, &alg_content_header);
            const uint8_t *bit_string = alg_content + alg_content_header + alg_content_len;
            // Actually bit_string starts at alg_content + alg_content_header, but need to check
            
            // The BitString should be after AlgorithmIdentifier
            const uint8_t *after_alg = spki + alg_header;
            printf("  After alg, tag=0x%02x\n", after_alg[0]); // Should be 0x03 for BitString
            
            size_t bs_len, bs_header;
            get_asn1_length(after_alg, len - alg_header, &bs_len, &bs_header);
            printf("  BitString len=%zu, header=%zu\n", bs_len, bs_header);
            
            // BitString contains: [0] EXPLICIT tag, then length, then SEQUENCE of integers
            const uint8_t *bs_content = after_alg + bs_header;
            printf("  bs_content[0] = 0x%02x\n", bs_content[0]);
            
            // First byte might be number of unused bits
            const uint8_t *rsakey_seq = bs_content;
            if (bs_content[0] == 0x00) {
                rsakey_seq = bs_content + 1;
                printf("  Skipped unused bits byte\n");
            }
            
            printf("  RSA key SEQUENCE tag = 0x%02x\n", rsakey_seq[0]);
            
            size_t rsa_len, rsa_header;
            get_asn1_length(rsakey_seq, len, &rsa_len, &rsa_header);
            printf("  RSA key SEQUENCE len=%zu, header=%zu\n", rsa_len, rsa_header);
            
            const uint8_t *rsa_content = rsakey_seq + rsa_header;
            
            // First integer is n (modulus)
            if (rsa_content[0] == 0x02) {
                size_t n_len, n_header;
                get_asn1_length(rsa_content, rsa_len, &n_len, &n_header);
                printf("  n length = %zu, header = %zu\n", n_len, n_header);
                
                const uint8_t *n_data = rsa_content + n_header;
                printf("  n[0] = 0x%02x\n", n_data[0]);
                if (n_data[0] == 0x00) {
                    printf("  n[1] = 0x%02x\n", n_data[1]);
                }
                
                // Second integer is e (exponent)
                const uint8_t *e_data = rsa_content + n_header + n_len;
                printf("  e tag = 0x%02x\n", e_data[0]);
                
                size_t e_len, e_header;
                get_asn1_length(e_data, rsa_len - n_header - n_len, &e_len, &e_header);
                printf("  e length = %zu, header = %zu\n", e_len, e_header);
                
                const uint8_t *e_value = e_data + e_header;
                printf("  e = ");
                for (size_t k = 0; k < e_len && k < 4; k++) printf("%02x", e_value[k]);
                printf("\n");
            }
            break;
        }
        pos += header_len + len;
    }
    
    free(der);
    free(pem);
    return 0;
}
