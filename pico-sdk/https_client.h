//
// SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//


#ifndef __HTTPS_CLIENT_H__
#define __HTTPS_CLIENT_H__

#include "tls_client.h"
#include "core_http_client.h"

typedef struct {
    tls_client_t tls;

    HTTPRequestHeaders_t request_headers;
    HTTPRequestInfo_t request_info;
    TransportInterface_t transport_inferface;
    HTTPResponse_t response;
} https_client_t;

int https_client_init(
    https_client_t* client,
    const unsigned char* root_ca,
    size_t root_ca_len,
    char* buf,
    size_t buf_len
);

enum HTTPStatus https_client_post(
    https_client_t* client,
    const char* host,
    const char* path,
    const char* headers[],
    size_t num_headers,
    const char* body,
    size_t body_len
);

enum HTTPStatus https_client_get(
    https_client_t* client,
    const char* host,
    const char* path,
    const char* headers[],
    size_t num_headers
);

#endif
