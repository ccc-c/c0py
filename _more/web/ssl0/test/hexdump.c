#include <stdio.h>
#include <stdlib.h>

static const char BASE64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int main() {
    FILE *f = fopen("./https/cert.pem", "r");
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
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
    
    printf("DER (%zu bytes):\n", der_len);
    for (size_t i = 0; i < der_len && i < 32; i++) {
        printf("%02x ", der[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    
    // Now let's decode the actual ASN.1 structure
    printf("\nTop-level SEQUENCE:\n");
    printf("  tag=0x%02x\n", der[0]);
    printf("  len=0x%02x (short form)\n", der[1]);
    printf("  content starts at offset 2\n");
    
    // TBSCertificate starts at offset 2
    // TBS is SEQUENCE, so tag=0x30
    printf("\nTBSCertificate at offset 2:\n");
    printf("  tag=0x%02x (should be 0x30)\n", der[2]);
    printf("  len=0x%02x\n", der[3]);
    if (der[3] > 0x80) {
        size_t num_bytes = der[3] & 0x7f;
        printf("  (long form, %zu length bytes)\n", num_bytes);
        size_t tbs_content_len = 0;
        for (size_t k = 0; k < num_bytes; k++) {
            tbs_content_len = (tbs_content_len << 8) | der[4 + k];
        }
        printf("  content length = %zu\n", tbs_content_len);
    }
    
    free(der);
    free(pem);
    return 0;
}
