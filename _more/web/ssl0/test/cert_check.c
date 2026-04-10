#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/certificate.h"

int main() {
    FILE *f = fopen("./https/cert.pem", "r");
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
    x509_cert cert;
    int ret = x509_parse_from_pem(pem, &cert);
    printf("x509_parse_from_pem returned: %d\n", ret);
    printf("n_len=%zu, e_len=%zu\n", cert.public_key.n_len, cert.public_key.e_len);
    
    free(pem);
    return ret;
}
