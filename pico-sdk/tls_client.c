#include <lwip/ip4_addr.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

#include "logging.h"

#include "tls_client.h"

static int mbedtls_ssl_lwip_send(void* ctx, const unsigned char* buf, size_t len)
{
    int sock = (int)ctx;
    int result = lwip_write(sock, buf, len);

    if (result == -1) {
        if (errno == EAGAIN) {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }
    }

    return result;
}

static int mbedtls_ssl_lwip_recv(void* ctx, unsigned char* buf, size_t len)
{
    int sock = (int)ctx;
    int result = lwip_read(sock, buf, len);

    if (result == -1) {
        if (errno == EAGAIN) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
    }

    return result;
}

int tls_client_init(tls_client_t* client, const unsigned char* root_ca, size_t root_ca_len)
{
    mbedtls_ssl_init(&client->ctx);
    mbedtls_ssl_config_init(&client->conf);
    mbedtls_x509_crt_init(&client->cacert);
    mbedtls_ctr_drbg_init(&client->ctr_drbg);
    mbedtls_entropy_init(&client->entropy);

    if (mbedtls_ctr_drbg_seed(&client->ctr_drbg, mbedtls_entropy_func, &client->entropy, NULL, 0) != 0 ) {
        LogError(("tls_client_init: mbedtls_ctr_drbg_seed failed!"));
        return -1;
    }

    if (mbedtls_x509_crt_parse_der_nocopy(&client->cacert, root_ca, root_ca_len) != 0) {
        LogError(("tls_client_init: mbedtls_x509_crt_parse_der_nocopy failed!"));
        return -1;
    }

    if(mbedtls_ssl_config_defaults(&client->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        LogError(("tls_client_init: mbedtls_ssl_config_defaults failed!"));
        return -1;
    }

    mbedtls_ssl_conf_authmode(&client->conf, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_ca_chain(&client->conf, &client->cacert, NULL);
    mbedtls_ssl_conf_rng(&client->conf, mbedtls_ctr_drbg_random, &client->ctr_drbg);
    mbedtls_ssl_setup(&client->ctx, &client->conf);

    client->sock = -1;

    return 0;
}

int tls_client_connect(tls_client_t* client, const char* host, const char* port)
{
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        LogError(("tls_client_connect: getaddrinfo failed!"));
        return -1;
    }

    client->sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client->sock == -1) {
        LogError(("tls_client_connect: socket failed, errno = %d"));
        return -1;
    }

    if (connect(client->sock, res->ai_addr, res->ai_addrlen) != 0) {
        LogError(("tls_client_connect: connect failed!"));
        return -1;
    }
    freeaddrinfo(res);

    if (mbedtls_ssl_set_hostname(&client->ctx, host) != 0) {
        LogError(("tls_client_connect: mbedtls_ssl_set_hostname failed!"));
        return -1;
    }

    mbedtls_ssl_set_bio(&client->ctx, (void*)client->sock, mbedtls_ssl_lwip_send, mbedtls_ssl_lwip_recv, NULL);

    return 0;
}

int tls_client_ioctl(tls_client_t* client, long cmd, void* argp)
{
    return lwip_ioctl(client->sock, cmd, argp);
}

int tls_client_write(tls_client_t* client, const uint8_t* data, size_t len)
{
    return mbedtls_ssl_write(&client->ctx, data, len);
}

int tls_client_read(tls_client_t* client, uint8_t* data, size_t len)
{
    return mbedtls_ssl_read(&client->ctx, data, len);
}

int tls_client_close(tls_client_t* client)
{
    close(client->sock);
    client->sock = -1;

    mbedtls_ssl_free(&client->ctx);
    mbedtls_ssl_config_free(&client->conf);
    mbedtls_ctr_drbg_free(&client->ctr_drbg);
    mbedtls_entropy_free(&client->entropy);
    mbedtls_x509_crt_free(&client->cacert);
}
