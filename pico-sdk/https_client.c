//
// SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//


#include <string.h>

#include "logging.h"

#include "https_client.h"

int https_client_init(
    https_client_t* client,
    const unsigned char* root_ca,
    size_t root_ca_len,
    char* buf, size_t buf_len
)
{
    if (tls_client_init(&client->tls, root_ca, root_ca_len) != 0) {
        return -1;
    }

    client->request_headers.pBuffer = buf;
    client->request_headers.bufferLen = buf_len;
    client->request_headers.headersLen = 0;

    client->transport_inferface.recv = (TransportRecv_t)tls_client_read;
    client->transport_inferface.send = (TransportSend_t)tls_client_write;
    client->transport_inferface.pNetworkContext = (NetworkContext_t*)&client->tls;

    client->response.pBuffer = buf;
    client->response.bufferLen = buf_len;
    client->response.getTime = NULL;

    return 0;
}

enum HTTPStatus https_client_request(
    https_client_t* client,
    const char* method,
    const char* host,
    const char* path,
    const char* headers[],
    size_t num_headers,
    const char* body,
    size_t body_len
)
{
    HTTPStatus_t status;

    client->request_info.pMethod = method;
    client->request_info.methodLen = strlen(method);
    client->request_info.pPath = path;
    client->request_info.pathLen = strlen(path);
    client->request_info.pHost = host;
    client->request_info.hostLen = strlen(host);
    client->request_info.reqFlags = 0;

    status = HTTPClient_InitializeRequestHeaders(&client->request_headers, &client->request_info);
    if (status != HTTPSuccess) {
        return status;
    }

    for (int i = 0; i < num_headers; i++) {
        status = HTTPClient_AddHeader(
            &client->request_headers,
            headers[i * 2],
            strlen(headers[i * 2]),
            headers[i * 2 + 1],
            strlen(headers[i * 2 + 1])
        );

        if (status != HTTPSuccess) {
        LogError(("https_client_request: HTTPClient_AddHeader failed!"));
          return status;
        }
    }

    if (tls_client_connect(&client->tls, host, "443") != 0) {
        LogError(("https_client_request: tls_client_connect failed!"));
        return HTTPNetworkError;
    }

    status = HTTPClient_Send(
        &client->transport_inferface,
        &client->request_headers,
        body,
        body_len,
        &client->response,
        0
    );

    if (status == HTTPParserInternalError && client->response.statusCode == 101) {
        status = HTTPSuccess;
    } else {
        tls_client_close(&client->tls);
    }

    return status;  
}

// TODO: create common function for POST and GET
enum HTTPStatus https_client_post(
    https_client_t* client,
    const char* host,
    const char* path,
    const char* headers[],
    size_t num_headers,
    const char* body,
    size_t body_len
)
{
    return https_client_request(
        client,
        HTTP_METHOD_POST,
        host,
        path,
        headers,
        num_headers,
        body,
        body_len
    );
}

enum HTTPStatus https_client_get(
    https_client_t* client,
    const char* host,
    const char* path,
    const char* headers[],
    size_t num_headers
)
{
    return https_client_request(
        client,
        HTTP_METHOD_GET,
        host,
        path,
        headers,
        num_headers,
        NULL,
        0
    );
}
