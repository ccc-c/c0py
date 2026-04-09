#include "../include/ssl_socket.h"
#include "../include/rand.h"
#include "../include/rsa.h"
#include "../include/bignum.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

#define TLS_RECORD_HEADER_LEN 5
#define TLS_HANDSHAKE_HEADER_LEN 4
#define TLS_MAX_MESSAGE_LEN 16384

static int send_all(int fd, const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

static int recv_all(int fd, uint8_t *buf, size_t len) {
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(fd, buf + received, len - received, 0);
        if (n <= 0) return -1;
        received += n;
    }
    return 0;
}

static int read_until(int fd, uint8_t *buf, size_t *pos, size_t target) {
    while (*pos < target) {
        ssize_t n = recv(fd, buf + *pos, target - *pos, 0);
        if (n <= 0) return -1;
        *pos += n;
    }
    return 0;
}

int ssl_socket_init(ssl_socket *sock) {
    memset(sock, 0, sizeof(ssl_socket));
    ssl_context_init(&sock->ctx);
    sock->fd = -1;
    sock->connected = 0;
    return 0;
}

int ssl_socket_bind(ssl_socket *sock, int port) {
    sock->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock->fd < 0) return -1;
    
    int opt = 1;
    setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    if (bind(sock->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock->fd);
        sock->fd = -1;
        return -1;
    }
    
    if (listen(sock->fd, 128) < 0) {
        close(sock->fd);
        sock->fd = -1;
        return -1;
    }
    
    return 0;
}

static int build_server_hello(uint8_t *buf, size_t *len,
                              const uint8_t *server_random,
                              const uint8_t *session_id, size_t session_id_len) {
    size_t pos = 0;
    
    buf[pos++] = 0x02;
    
    uint8_t handshake_len[3];
    handshake_len[0] = ((*len - 6) >> 16) & 0xff;
    handshake_len[1] = ((*len - 6) >> 8) & 0xff;
    handshake_len[2] = (*len - 6) & 0xff;
    memcpy(buf + pos, handshake_len, 3);
    pos += 3;
    
    buf[pos++] = 0x03;
    buf[pos++] = 0x03;
    memcpy(buf + pos, server_random, 32);
    pos += 32;
    
    buf[pos++] = session_id_len;
    memcpy(buf + pos, session_id, session_id_len);
    pos += session_id_len;
    
    buf[pos++] = 0x00;
    buf[pos++] = 0x2f;
    
    buf[pos++] = 0x00;
    
    *len = pos;
    return 0;
}

static int build_certificate(uint8_t *buf, size_t *len, const uint8_t *cert_der, size_t cert_der_len) {
    size_t pos = 0;
    
    buf[pos++] = 0x0b;
    
    uint32_t total_len = cert_der_len + 3;
    buf[pos++] = (total_len >> 16) & 0xff;
    buf[pos++] = (total_len >> 8) & 0xff;
    buf[pos++] = total_len & 0xff;
    
    buf[pos++] = (cert_der_len >> 16) & 0xff;
    buf[pos++] = (cert_der_len >> 8) & 0xff;
    buf[pos++] = cert_der_len & 0xff;
    memcpy(buf + pos, cert_der, cert_der_len);
    pos += cert_der_len;
    
    *len = pos;
    return 0;
}

static int build_server_hello_done(uint8_t *buf, size_t *len) {
    size_t pos = 0;
    buf[pos++] = 0x0e;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    *len = pos;
    return 0;
}

static int build_handshake_message(uint8_t *buf, size_t *len, uint8_t type, const uint8_t *data, size_t data_len) {
    size_t pos = 0;
    buf[pos++] = type;
    buf[pos++] = (data_len >> 16) & 0xff;
    buf[pos++] = (data_len >> 8) & 0xff;
    buf[pos++] = data_len & 0xff;
    memcpy(buf + pos, data, data_len);
    pos += data_len;
    *len = pos;
    return 0;
}

static int build_tls_record(uint8_t *buf, size_t *len, uint8_t content_type, const uint8_t *handshake_data, size_t handshake_len) {
    size_t pos = 0;
    buf[pos++] = content_type;
    buf[pos++] = 0x03;
    buf[pos++] = 0x03;
    buf[pos++] = (handshake_len >> 8) & 0xff;
    buf[pos++] = handshake_len & 0xff;
    memcpy(buf + pos, handshake_data, handshake_len);
    pos += handshake_len;
    *len = pos;
    return 0;
}

int ssl_socket_accept(ssl_socket *sock, ssl_socket *client, const char *cert_pem) {
    memset(client, 0, sizeof(ssl_socket));
    ssl_context_init(&client->ctx);
    client->fd = -1;
    client->connected = 0;
    
    x509_cert cert;
    if (x509_parse_from_pem(cert_pem, &cert) != 0) {
        fprintf(stderr, "Failed to parse certificate\n");
        return -1;
    }
    
    uint8_t recv_buf[2048];
    size_t recv_pos = 0;
    
    if (recv_all(sock->fd, recv_buf, 5) != 0) return -1;
    recv_pos = 5;
    
    uint16_t incoming_record_len = (recv_buf[3] << 8) | recv_buf[4];
    if (incoming_record_len > sizeof(recv_buf) - 5) return -1;
    
    if (read_until(sock->fd, recv_buf, &recv_pos, 5 + incoming_record_len) != 0) return -1;
    
    if (recv_buf[0] != 0x16 || recv_buf[1] != 0x03 || recv_buf[2] != 0x03) {
        return -1;
    }
    
    if (recv_pos < TLS_RECORD_HEADER_LEN + TLS_HANDSHAKE_HEADER_LEN) return -1;
    
    uint32_t handshake_len = (recv_buf[5] << 16) | (recv_buf[6] << 8) | recv_buf[7];
    if (handshake_len + TLS_RECORD_HEADER_LEN != incoming_record_len) return -1;
    
    if (recv_buf[8] != 0x01) return -1;
    
    uint8_t client_random[32];
    size_t client_random_offset = 11 + recv_buf[10];
    if (recv_pos < client_random_offset + 32) return -1;
    memcpy(client_random, recv_buf + client_random_offset, 32);
    
    uint8_t server_random[32];
    rand_bytes(server_random, 32);
    
    uint8_t session_id[32];
    uint8_t session_id_len = 32;
    rand_bytes(session_id, session_id_len);
    
    uint8_t server_hello[128];
    size_t server_hello_len = 0;
    build_server_hello(server_hello, &server_hello_len, server_random, session_id, session_id_len);
    
    uint8_t handshake_server_hello[128];
    size_t hs_len = 0;
    build_handshake_message(handshake_server_hello, &hs_len, 0x02, server_hello, server_hello_len);
    
    uint8_t cert_der[1024];
    size_t cert_der_len = 0;
    x509_get_public_key(&cert, cert_der, &cert_der_len, cert_der + 256, &cert_der_len);
    
    uint8_t cert_msg[2048];
    size_t cert_msg_len = 0;
    build_certificate(cert_msg, &cert_msg_len, cert_der, cert_der_len);
    
    uint8_t handshake_cert[2048];
    size_t hs_cert_len = 0;
    build_handshake_message(handshake_cert, &hs_cert_len, 0x0b, cert_msg, cert_msg_len);
    
    uint8_t server_hello_done[4];
    size_t shd_len = 0;
    build_server_hello_done(server_hello_done, &shd_len);
    
    uint8_t handshake_shd[4];
    size_t hs_shd_len = 0;
    build_handshake_message(handshake_shd, &hs_shd_len, 0x0e, server_hello_done, shd_len);
    
    size_t total_handshake_len = hs_len + hs_cert_len + hs_shd_len;
    uint8_t record_server_hello[2048];
    size_t record_len = 0;
    build_tls_record(record_server_hello, &record_len, 0x16, handshake_server_hello, hs_len);
    send_all(sock->fd, record_server_hello, record_len);
    
    uint8_t record_cert[2048];
    record_len = 0;
    build_tls_record(record_cert, &record_len, 0x16, handshake_cert, hs_cert_len);
    send_all(sock->fd, record_cert, record_len);
    
    uint8_t record_shd[256];
    record_len = 0;
    build_tls_record(record_shd, &record_len, 0x16, handshake_shd, hs_shd_len);
    send_all(sock->fd, record_shd, record_len);
    
    recv_pos = 0;
    if (read_until(sock->fd, recv_buf, &recv_pos, 5) != 0) return -1;
    
    uint16_t record_len2 = (recv_buf[3] << 8) | recv_buf[4];
    if (read_until(sock->fd, recv_buf, &recv_pos, 5 + record_len2) != 0) return -1;
    
    if (recv_buf[0] != 0x16) return -1;
    
    uint32_t handshake_len2 = (recv_buf[5] << 16) | (recv_buf[6] << 8) | recv_buf[7];
    size_t handshake_data_offset = 5 + 4;
    
    size_t pos = handshake_data_offset;
    while (pos < handshake_len2 + 4) {
        uint8_t msg_type = recv_buf[pos];
        uint32_t msg_len = (recv_buf[pos+1] << 16) | (recv_buf[pos+2] << 8) | recv_buf[pos+3];
        
        if (msg_type == 0x10) {
            uint8_t pre_master_secret[48];
            pre_master_secret[0] = 0x03;
            pre_master_secret[1] = 0x03;
            rand_bytes(pre_master_secret + 2, 46);
            
            bignum N, E, C;
            bn_from_bytes(&N, cert.public_key.n, cert.public_key.n_len);
            bn_from_bytes(&E, cert.public_key.e, cert.public_key.e_len);
            bn_from_bytes(&C, recv_buf + pos + 4, msg_len);
            
            bignum PMS;
            bn_mod_exp(&PMS, &C, &N, &N);
            
            bignum temp_d;
            temp_d.words[0] = 0;
            temp_d.len = 1;
            
            uint8_t pms_bytes[48];
            size_t pms_len;
            bn_to_bytes(&PMS, pms_bytes, &pms_len);
            if (pms_len < 48) {
                memmove(pms_bytes + 48 - pms_len, pms_bytes, pms_len);
                memset(pms_bytes, 0, 48 - pms_len);
            }
            
            ssl_handshake_client(&client->ctx, cert_der, cert_der_len,
                                 client_random, 32, server_random, 32,
                                 pms_bytes, 48);
            client->connected = 1;
            break;
        }
        
        pos += 4 + msg_len;
    }
    
    uint8_t finished[32];
    rand_bytes(finished, 32);
    
    uint8_t change_cipher_spec[6];
    change_cipher_spec[0] = 0x14;
    change_cipher_spec[1] = 0x03;
    change_cipher_spec[2] = 0x03;
    change_cipher_spec[3] = 0x00;
    change_cipher_spec[4] = 0x01;
    change_cipher_spec[5] = 0x01;
    send_all(sock->fd, change_cipher_spec, 6);
    
    x509_free(&cert);
    client->fd = sock->fd;
    sock->fd = -1;
    
    return 0;
}

int ssl_socket_read(ssl_socket *sock, uint8_t *buf, size_t len) {
    if (!sock->connected) {
        return recv(sock->fd, buf, len, 0);
    }
    
    uint8_t header[5];
    if (recv_all(sock->fd, header, 5) != 0) return -1;
    
    uint16_t record_len = (header[3] << 8) | header[4];
    if (record_len > TLS_MAX_MESSAGE_LEN) return -1;
    
    uint8_t *record = (uint8_t *)malloc(record_len);
    if (read_until(sock->fd, record, &(size_t){0}, record_len) != 0) {
        free(record);
        return -1;
    }
    
    uint8_t content_type;
    uint8_t plaintext[TLS_MAX_MESSAGE_LEN];
    size_t plain_len;
    
    int ret = ssl_decrypt_record(&sock->ctx, record, record_len + 5, &content_type, plaintext, &plain_len);
    free(record);
    
    if (ret != 0) return -1;
    
    if (plain_len > len) plain_len = len;
    memcpy(buf, plaintext, plain_len);
    
    return plain_len;
}

int ssl_socket_write(ssl_socket *sock, const uint8_t *buf, size_t len) {
    if (!sock->connected) {
        return send(sock->fd, buf, len, 0);
    }
    
    uint8_t ciphertext[TLS_MAX_MESSAGE_LEN + 32];
    size_t cipher_len;
    
    if (ssl_encrypt_record(&sock->ctx, 0x17, buf, len, ciphertext, &cipher_len) != 0) {
        return -1;
    }
    
    return send_all(sock->fd, ciphertext, cipher_len);
}

void ssl_socket_close(ssl_socket *sock) {
    if (sock->fd >= 0) {
        close(sock->fd);
    }
    ssl_free(&sock->ctx);
    memset(sock, 0, sizeof(ssl_socket));
    sock->fd = -1;
}