#include <stdio.h>
#include <stdlib.h>
#include "../include/certificate.h"

int main() {
    FILE *f = fopen("./https/cert.pem", "r");
    if (!f) {
        perror("fopen ./https/cert.pem");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *pem = malloc(len + 1);
    fread(pem, 1, len, f);
    pem[len] = '\0';
    fclose(f);
    
    x509_cert cert;
    int ret = x509_parse_from_pem(pem, &cert);
    printf("x509_parse_from_pem returned: %d\n", ret);
    printf("n_len=%zu, e_len=%zu\n", cert.public_key.n_len, cert.public_key.e_len);
    
    free(pem);
    return ret;
}
