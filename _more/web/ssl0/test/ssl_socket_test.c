#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/ssl_socket.h"
#include "../include/rand.h"
#include "../include/certificate.h"

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
}

int main() {
    printf("=== Rand Test ===\n");
    uint8_t rand_buf[16];
    rand_bytes(rand_buf, 16);
    print_hex("Random bytes", rand_buf, 16);
    
    printf("\n=== Certificate Test ===\n");
    x509_cert cert;
    FILE *f = fopen("./https/cert.pem", "r");
    if (!f) {
        fprintf(stderr, "Cannot open ./https/cert.pem\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long pem_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *pem = (char *)malloc(pem_len + 1);
    fread(pem, 1, pem_len, f);
    pem[pem_len] = '\0';
    fclose(f);
    
    printf("PEM length: %ld\n", pem_len);
    
    char *begin = strstr(pem, "-----BEGIN");
    char *end = strstr(pem, "-----END");
    if (begin && end) {
        printf("Found BEGIN at %ld, END at %ld\n", begin - pem, end - pem);
    }
    
    int ret = x509_parse_from_pem(pem, &cert);
    printf("After parsing, n_len=%zu, e_len=%zu\n", cert.public_key.n_len, cert.public_key.e_len);
    if (cert.public_key.n_len > 0) {
        printf("n[0] = 0x%02x\n", cert.public_key.n[0]);
    }
    if (ret != 0) {
        printf("x509_parse_from_pem failed: %d\n", ret);
        free(pem);
        return 1;
    }
    print_hex("RSA n", cert.public_key.n, cert.public_key.n_len);
    print_hex("RSA e", cert.public_key.e, cert.public_key.e_len);
    
    uint8_t n[256], e[4];
    size_t n_len, e_len;
    ret = x509_get_public_key(&cert, n, &n_len, e, &e_len);
    if (ret != 0) {
        printf("x509_get_public_key failed: %d\n", ret);
        x509_free(&cert);
        free(pem);
        return 1;
    }
    print_hex("Extracted n", n, n_len);
    print_hex("Extracted e", e, e_len);
    x509_free(&cert);
    free(pem);
    
    printf("\n=== SSL Socket Test ===\n");
    ssl_socket server, client;
    ret = ssl_socket_init(&server);
    if (ret != 0) {
        printf("ssl_socket_init failed\n");
        return 1;
    }
    
    ret = ssl_socket_bind(&server, 8444);
    if (ret != 0) {
        printf("ssl_socket_bind failed: %d\n", ret);
        return 1;
    }
    
    printf("Server listening on port 8444\n");
    printf("Waiting for connection...\n");
    
    ret = ssl_socket_accept(&server, &client, "./https/cert.pem");
    if (ret != 0) {
        printf("ssl_socket_accept failed: %d\n", ret);
        return 1;
    }
    
    printf("Client connected!\n");
    printf("has_keys = %d\n", client.ctx.has_keys);
    
    uint8_t buf[1024];
    ssize_t n_read = ssl_socket_read(&client, buf, sizeof(buf) - 1);
    if (n_read > 0) {
        buf[n_read] = '\0';
        printf("Read %zd bytes: %s\n", n_read, buf);
    }
    
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    ssl_socket_write(&client, (uint8_t *)response, strlen(response));
    
    ssl_socket_close(&client);
    ssl_socket_close(&server);
    
    printf("\n=== All SSL Socket tests passed ===\n");
    return 0;
}