#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    printf("DER length: %zu\n", der_len);
    printf("DER[0-15]: ");
    for (int i = 0; i < 16 && i < der_len; i++) printf("%02x ", der[i]);
    printf("\n");
    
    // Check if it's a valid certificate
    if (der[0] == 0x30) {
        printf("Valid SEQUENCE tag\n");
        size_t len = der[1];
        printf("Content length: %zu\n", len);
    }
    
    free(der);
    free(pem);
    return 0;
}
