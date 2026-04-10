#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE *f = fopen("./https/cert.pem", "r");
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
    // Get the base64 content only (between -----BEGIN and -----END)
    const char *begin = strstr(pem, "-----BEGIN");
    const char *begin_nl = strchr(begin, '\n');
    const char *end = strstr(pem, "-----END");
    
    printf("After '-----BEGIN' + newline, before '-----END':\n");
    printf("First 64 chars: %.*s\n", 64, begin_nl + 1);
    
    // Try decoding with openssl to see what we should get
    free(pem);
    return 0;
}
