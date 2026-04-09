#include "../include/certificate.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char BASE64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_decode(const char *src, size_t src_len, uint8_t *out, size_t *out_len) {
    size_t i, j = 0;
    
    for (i = 0; i < src_len; i++) {
        if (src[i] == '=') continue;
        if (src[i] == '\n' || src[i] == '\r') continue;
        
        const char *p = strchr(BASE64_TABLE, src[i]);
        if (!p) continue;
        
        int val = p - BASE64_TABLE;
        
        switch (i % 4) {
            case 0:
                out[j] = val << 2;
                break;
            case 1:
                out[j++] |= val >> 4;
                out[j] = val << 4;
                break;
            case 2:
                out[j++] |= val >> 2;
                out[j] = val << 6;
                break;
            case 3:
                out[j++] |= val;
                break;
        }
    }
    
    *out_len = j;
    return 0;
}

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

static int extract_integer(const uint8_t *data, size_t data_len, uint8_t *out, size_t *out_len) {
    if (data_len < 2 || data[0] != 0x02) return -1;
    
    size_t len, header_len;
    if (get_asn1_length(data, data_len, &len, &header_len) != 0) return -1;
    if (data_len < header_len + len) return -1;
    
    size_t skip = 0;
    if (len > 0 && data[header_len] == 0x00) {
        skip = 1;
        len--;
    }
    
    memcpy(out, data + header_len + skip, len);
    *out_len = len;
    return 0;
}

int x509_parse_from_pem(const char *pem, x509_cert *cert) {
    memset(cert, 0, sizeof(x509_cert));
    
    const char *begin = strstr(pem, "-----BEGIN");
    if (!begin) return -1;
    const char *begin_nl = strchr(begin, '\n');
    if (!begin_nl) return -1;
    begin_nl++;
    
    const char *end = strstr(pem, "-----END");
    if (!end) return -1;
    
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
    size_t der_len;
    
    if (base64_decode(b64_clean, j, der, &der_len) != 0) {
        free(b64_clean);
        free(der);
        return -1;
    }
    free(b64_clean);
    
    if (der_len < 4 || der[0] != 0x30) {
        free(der);
        return -1;
    }
    
    size_t cert_content_len, cert_header_len;
    if (get_asn1_length(der, der_len, &cert_content_len, &cert_header_len) != 0) {
        free(der);
        return -1;
    }
    
    size_t tbs_offset = cert_header_len;
    const uint8_t *tbs_cert = der + tbs_offset;
    size_t tbs_len = cert_content_len;
    
    size_t tbs_content_len, tbs_content_header_len;
    if (get_asn1_length(tbs_cert, tbs_len, &tbs_content_len, &tbs_content_header_len) != 0) {
        free(der);
        return -1;
    }
    
    const uint8_t *tbs_content = tbs_cert + tbs_content_header_len;
    size_t tbs_inner_len = tbs_content_len;
    
    size_t pos = 0;
    while (pos + 4 < tbs_inner_len) {
        uint8_t tag = tbs_content[pos];
        
        size_t len, header_len;
        if (get_asn1_length(tbs_content + pos, tbs_inner_len - pos, &len, &header_len) != 0) {
            break;
        }
        
        if (tag == 0x30 && len > 256) {
            const uint8_t *seq = tbs_content + pos + header_len;
            size_t seq_len = len;
            
            size_t pos2 = 0;
            while (pos2 + 4 < seq_len) {
                uint8_t tag2 = seq[pos2];
                size_t len2, header2;
                
                if (get_asn1_length(seq + pos2, seq_len - pos2, &len2, &header2) != 0) break;
                
                if (tag2 == 0x30 && len2 > 128) {
                    const uint8_t *inner = seq + pos2 + header2;
                    size_t inner_len = len2;
                    
                    size_t int_pos = 0;
                    int found_n = 0, found_e = 0;
                    while (int_pos + 4 < inner_len && (!found_n || !found_e)) {
                        if (inner[int_pos] == 0x02) {
                            size_t n_len;
                            uint8_t n[256];
                            if (extract_integer(inner + int_pos, inner_len - int_pos, n, &n_len) == 0) {
                                if (!found_n && n_len <= sizeof(cert->public_key.n)) {
                                    memcpy(cert->public_key.n, n, n_len);
                                    cert->public_key.n_len = n_len;
                                    found_n = 1;
                                } else if (!found_e && n_len <= sizeof(cert->public_key.e)) {
                                    memcpy(cert->public_key.e, n, n_len);
                                    cert->public_key.e_len = n_len;
                                    found_e = 1;
                                }
                            }
                        }
                        size_t step = 1;
                        if (int_pos + 1 < inner_len) {
                            if (inner[int_pos + 1] >= 0x80) {
                                step = 2 + (inner[int_pos + 1] & 0x7f);
                            } else {
                                step = 2 + inner[int_pos + 1];
                            }
                        }
                        int_pos += step;
                        if (int_pos >= inner_len) break;
                    }
                    break;
                }
                
                pos2 += header2 + len2;
                if (pos2 >= seq_len) break;
            }
            break;
        }
        
        pos += header_len + len;
        if (pos >= tbs_inner_len) break;
    }
    
    free(der);
    return (cert->public_key.n_len > 0 && cert->public_key.e_len > 0) ? 0 : -1;
}

int x509_get_public_key(const x509_cert *cert, uint8_t *n, size_t *n_len, uint8_t *e, size_t *e_len) {
    if (cert->public_key.n_len == 0) return -1;
    memcpy(n, cert->public_key.n, cert->public_key.n_len);
    *n_len = cert->public_key.n_len;
    memcpy(e, cert->public_key.e, cert->public_key.e_len);
    *e_len = cert->public_key.e_len;
    return 0;
}

void x509_free(x509_cert *cert) {
    memset(cert, 0, sizeof(x509_cert));
}