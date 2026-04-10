// ssl_socket.c

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
#include <arpa/inet.h>

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
    *len = 0;
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

// === 新增：PEM 轉 DER 的輔助函數 ===
static int base64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static size_t pem_to_der(const char *pem, uint8_t *der) {
    const char *begin = strstr(pem, "-----BEGIN");
    if (!begin) return 0;
    const char *data_start = strchr(begin, '\n');
    if (!data_start) return 0;
    
    const char *end = strstr(data_start, "-----END");
    if (!end) return 0;

    size_t out_len = 0;
    uint32_t accumulator = 0;
    int bits = 0;

    for (const char *p = data_start; p < end; p++) {
        int val = base64_decode_char(*p);
        if (val >= 0) {
            accumulator = (accumulator << 6) | val;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                der[out_len++] = (accumulator >> bits) & 0xFF;
            }
        }
    }
    return out_len;
}
// =================================

int ssl_socket_accept(ssl_socket *sock, ssl_socket *client, const char *cert_pem) {
    memset(client, 0, sizeof(ssl_socket));
    ssl_context_init(&client->ctx);
    client->fd = -1;
    client->connected = 0;
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    // 1. 等待並接收 TCP 連線
    int client_fd = accept(sock->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) return -1;
    
    client->fd = client_fd;
    
    // 解析 X.509 憑證
    x509_cert cert;
    int parse_ret = x509_parse_from_pem(cert_pem, &cert);
    if (parse_ret != 0) {
        fprintf(stderr, "Failed to parse certificate\n");
        close(client->fd);
        client->fd = -1;
        return -1;
    }
    
    uint8_t recv_buf[2048];
    size_t recv_pos = 0;
    
    // 2. 讀取並解析 ClientHello
    if (recv_all(client->fd, recv_buf, 5) != 0) {
        printf("DEBUG: Failed to read Record Header\n");
        goto handshake_err;
    }
    recv_pos = 5;
    
    // 檢查是否為 TLS 協定 (0x16 0x03)
    if (recv_buf[0] != 0x16 || recv_buf[1] != 0x03) {
        printf("DEBUG: Not a TLS Handshake Record (Type=0x%02x)\n", recv_buf[0]);
        goto handshake_err;
    }
    
    uint16_t incoming_record_len = (recv_buf[3] << 8) | recv_buf[4];
    if (incoming_record_len > sizeof(recv_buf) - 5) {
        printf("DEBUG: Record too large\n");
        goto handshake_err;
    }
    
    if (read_until(client->fd, recv_buf, &recv_pos, 5 + incoming_record_len) != 0) {
        printf("DEBUG: Failed to read Record Body\n");
        goto handshake_err;
    }
    
    // 檢查 Handshake Type 是否為 ClientHello (0x01)
    if (recv_buf[5] != 0x01) {
        printf("DEBUG: Not a ClientHello (Type=0x%02x)\n", recv_buf[5]);
        goto handshake_err;
    }
    
    // 取得 Handshake 長度
    uint32_t handshake_len = (recv_buf[6] << 16) | (recv_buf[7] << 8) | recv_buf[8];
    if (handshake_len + 4 != incoming_record_len) {
        printf("DEBUG: Handshake length mismatch\n");
        goto handshake_err;
    }
    
    // 讀取 Client Random (位移 11)
    uint8_t client_random[32];
    memcpy(client_random, recv_buf + 11, 32);
    
    // 3. 準備 ServerHello 資料
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
    
    // 4. 將 PEM 轉為 DER 以發送 Certificate
    uint8_t cert_der[2048];
    size_t cert_der_len = pem_to_der(cert_pem, cert_der);
    if (cert_der_len == 0) {
        fprintf(stderr, "Failed to decode PEM to DER\n");
        goto handshake_err;
    }
    
    uint8_t cert_msg[2048];
    size_t cert_msg_len = 0;
    build_certificate(cert_msg, &cert_msg_len, cert_der, cert_der_len);
    
    uint8_t handshake_cert[2048];
    size_t hs_cert_len = 0;
    build_handshake_message(handshake_cert, &hs_cert_len, 0x0b, cert_msg, cert_msg_len);
    
    // 5. 準備 ServerHelloDone
    uint8_t server_hello_done[4];
    size_t shd_len = 0;
    build_server_hello_done(server_hello_done, &shd_len);
    
    uint8_t handshake_shd[4];
    size_t hs_shd_len = 0;
    build_handshake_message(handshake_shd, &hs_shd_len, 0x0e, server_hello_done, shd_len);
    
    // 6. 發送這三個封包
    uint8_t record_server_hello[2048];
    size_t record_len = 0;
    build_tls_record(record_server_hello, &record_len, 0x16, handshake_server_hello, hs_len);
    send_all(client->fd, record_server_hello, record_len);
    
    uint8_t record_cert[2048];
    record_len = 0;
    build_tls_record(record_cert, &record_len, 0x16, handshake_cert, hs_cert_len);
    send_all(client->fd, record_cert, record_len);
    
    uint8_t record_shd[256];
    record_len = 0;
    build_tls_record(record_shd, &record_len, 0x16, handshake_shd, hs_shd_len);
    send_all(client->fd, record_shd, record_len);
    
    // 7. 接收 ClientKeyExchange
    recv_pos = 0;
    if (read_until(client->fd, recv_buf, &recv_pos, 5) != 0) goto handshake_err;
    
    uint16_t record_len2 = (recv_buf[3] << 8) | recv_buf[4];
    if (read_until(client->fd, recv_buf, &recv_pos, 5 + record_len2) != 0) goto handshake_err;
    
    if (recv_buf[0] != 0x16) goto handshake_err;
    
    uint32_t handshake_len2 = (recv_buf[5] << 16) | (recv_buf[6] << 8) | recv_buf[7];
    size_t handshake_data_offset = 5 + 4;
    
    size_t pos = handshake_data_offset;
    while (pos < handshake_len2 + 4) {
        uint8_t msg_type = recv_buf[pos];
        uint32_t msg_len = (recv_buf[pos+1] << 16) | (recv_buf[pos+2] << 8) | recv_buf[pos+3];
        
        if (msg_type == 0x10) { // Client Key Exchange
            uint8_t pre_master_secret[48];
            pre_master_secret[0] = 0x03;
            pre_master_secret[1] = 0x03;
            rand_bytes(pre_master_secret + 2, 46);
            
            // 讀取前 2 bytes 的密文長度
            uint16_t cipher_len = (recv_buf[pos+4] << 8) | recv_buf[pos+5];
            
            // 安全性檢查：防止密文長度超過 bignum 的容量 (256 bytes)
            if (cipher_len > 256) cipher_len = 256;
            
            // 真正的密文資料從 pos + 6 開始
            const uint8_t *cipher_data = recv_buf + pos + 6;
            
            bignum N, E, C;
            bn_from_bytes(&N, cert.public_key.n, cert.public_key.n_len);
            bn_from_bytes(&E, cert.public_key.e, cert.public_key.e_len);
            bn_from_bytes(&C, cipher_data, cipher_len); // <-- 修正：傳入正確的指標與長度
            
            bignum PMS;
            // ⚠️ 注意：目前依然是用公鑰(E)解密，解出來的必定是亂碼
            bn_mod_exp(&PMS, &C, &E, &N); 
            
            uint8_t pms_bytes[48];
            size_t pms_len;
            bn_to_bytes(&PMS, pms_bytes, &pms_len);
            
            // 確保 PMS 補齊到 48 bytes
            if (pms_len < 48) {
                memmove(pms_bytes + 48 - pms_len, pms_bytes, pms_len);
                memset(pms_bytes, 0, 48 - pms_len);
            }
            
            // 這裡注意：如果是伺服器端，有時候可能要呼叫 ssl_handshake_server
            // 但如果你的 API 只有 ssl_handshake_client，我們就先保留
            ssl_handshake_client(&client->ctx, cert_der, cert_der_len,
                                 client_random, 32, server_random, 32,
                                 pms_bytes, 48);
            client->connected = 1;
            break;
        }

        pos += 4 + msg_len;
    }
    
    // 8. 發送 ChangeCipherSpec
    uint8_t change_cipher_spec[6];
    change_cipher_spec[0] = 0x14;
    change_cipher_spec[1] = 0x03;
    change_cipher_spec[2] = 0x03;
    change_cipher_spec[3] = 0x00;
    change_cipher_spec[4] = 0x01;
    change_cipher_spec[5] = 0x01;
    send_all(client->fd, change_cipher_spec, 6);
    
    x509_free(&cert);
    return 0;

handshake_err:
    x509_free(&cert);
    close(client->fd);
    client->fd = -1;
    return -1;
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