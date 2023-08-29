#ifndef __WSS_CLIENT_H_
#define __WSS_CLIENT_H_

#include "https_client.h"

typedef struct {
    https_client_t https;
} wss_client_t;

#define WEBSOCKET_OPCODE_TEXT             0x1
#define WEBSOCKET_OPCODE_BINARY           0x2
#define WEBSOCKET_OPCODE_CONNECTION_CLOSE 0x8
#define WEBSOCKET_OPCODE_PING             0x9
#define WEBSOCKET_OPCODE_PONG             0xA

int wss_client_init(
    wss_client_t* client,
    const unsigned char* root_ca,
    size_t root_ca_len,
    char* buf,
    size_t buf_len
);

enum HTTPStatus ws_client_open(wss_client_t* client, const char* host, const char* path);

int ws_client_connected(wss_client_t* client);

int wss_client_write(wss_client_t* client, uint8_t type, const uint8_t* buf, size_t len);

int wss_client_read(wss_client_t* client, uint8_t* type, uint8_t* buf, size_t len);

int wss_client_close(wss_client_t* client);

#endif
