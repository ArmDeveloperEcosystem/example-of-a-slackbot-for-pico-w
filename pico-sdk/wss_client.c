//
// SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//


#include <lwip/sockets.h>

#include <mbedtls/base64.h>

#include "wss_client.h"

int wss_client_init(
    wss_client_t* client,
    const unsigned char* root_ca,
    size_t root_ca_len,
    char* buf, size_t
    buf_len)
{
    if (https_client_init(&client->https, root_ca, root_ca_len, buf, buf_len) != 0) {
        return -1;
    }

    return 0;
}

enum HTTPStatus ws_client_open(wss_client_t* client, const char* host, const char* path)
{
    unsigned char key[16];
    char key_base64[24 + 1];

    for (int i = 0; i < 16; i += 8) {
        uint64_t rand64 = get_rand_64();
        memcpy(&key[i], &rand64, sizeof(rand64));
    }

    size_t olen;
    mbedtls_base64_encode(key_base64, sizeof(key_base64), &olen, key, sizeof(key));

    enum HTTPStatus status = https_client_get(
        &client->https,
        host,
        path,
        (const char*[]){
            "Upgrade", "websocket",
            "Connection", "Upgrade",
            "Sec-WebSocket-Key", key_base64,
            "Sec-WebSocket-Version", "13"
        },
        4
    );

    return status;
}

int ws_client_connected(wss_client_t* client)
{
    if (client->https.tls.sock == -1) {
        return 0;
    }

    int result = tls_client_read(&client->https.tls, NULL, 0);

    if (result == 0 || result == MBEDTLS_ERR_SSL_WANT_READ) {
        return 1;
    }

    return 0;
}

int wss_client_write(wss_client_t* client, uint8_t type, const uint8_t* buf, size_t len)
{
    if (len > 0xFFFF) {
        return -1;
    }

    uint8_t header[8];
    int header_len = 0;

    header[header_len++] = 0x80 | type;
    if (len < 126) {    
        header[header_len++] = 0x80 | len;
    } else {
        header[header_len++] = 0x80 | 126;
        header[header_len++] = (len >> 8) & 0xFF;
        header[header_len++] = (len >> 0) & 0xFF;
    }

    // mask
    header[header_len++] = 0x00;
    header[header_len++] = 0x00;
    header[header_len++] = 0x00;
    header[header_len++] = 0x00;

    if (tls_client_write(&client->https.tls, header, header_len) < 0) {
        return -1;
    }

    return tls_client_write(&client->https.tls, buf, len);
}

int wss_client_read(wss_client_t* client, uint8_t* type, uint8_t* buf, size_t len)
{
    int fiobio = 1; 
    tls_client_ioctl(&client->https.tls, FIONBIO, &fiobio); 

    uint8_t header[2];
    int result = tls_client_read(&client->https.tls, header, sizeof(header));

    if (result == MBEDTLS_ERR_SSL_WANT_READ) {
        return 0;
    } else if (result <= 0) {
        return result;
    }

    fiobio = 1; 
    tls_client_ioctl(&client->https.tls, FIONBIO, &fiobio);

    *type = header[0] & 0x7F;
    int msg_length = header[1];

    if (msg_length == 126) {
        result = tls_client_read(&client->https.tls, header, sizeof(header));

        if (result <= 0) {
            return -1;
        }

        msg_length = (header[0] << 8) | header[1];
    } else if (msg_length == 127) {
        // unsupported size
        LogError(("wss_client_read: received message of length > 65535"))

        wss_client_close(client);
        return -1;
    }

    if (msg_length > len) {
        // unsupported size
        LogError(("wss_client_read: got message of length %d, which was larger than buffer size %d", msg_length, len))
        wss_client_close(client);
        return -1;
    }

    return tls_client_read(&client->https.tls, buf, msg_length);
}

int wss_client_close(wss_client_t* client)
{
    tls_client_close(&client->https.tls);

    return 0;
}
