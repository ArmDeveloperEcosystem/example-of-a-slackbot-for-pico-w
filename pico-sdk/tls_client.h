#ifndef __TLS_CLIENT_H__
#define __TLS_CLIENT_H__

#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

typedef struct {
    int sock;

    mbedtls_x509_crt cacert;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ctx;
    mbedtls_ssl_config conf;
} tls_client_t;

int tls_client_init(tls_client_t* client, const unsigned char* root_ca, size_t root_ca_len);

int tls_client_connect(tls_client_t* client, const char* host, const char* port);

int tls_client_ioctl(tls_client_t* client, long cmd, void* argp);

int tls_client_write(tls_client_t* client, const uint8_t* data, size_t len);

int tls_client_read(tls_client_t* client, uint8_t* data, size_t len);

int tls_client_close(tls_client_t* client);

#endif
