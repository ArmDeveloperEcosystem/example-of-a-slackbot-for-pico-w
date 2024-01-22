//
// SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//


#include <stdio.h>
#include <string.h>

#include "ISRG_Root_X1.h"
#include "logging.h"

#include "slack_client.h"

int slack_client_init(slack_client_t* client, const char* bot_token, const char* app_token, char* buf, size_t buf_len)
{
    client->bot_token = bot_token;
    client->app_token = app_token;
    client->buf = buf,
    client->buf_len = buf_len;

    return 0;
}

int slack_client_open_app_connection(slack_client_t* client)
{
    if (https_client_init(&client->https, ISRG_Root_X1_der, sizeof(ISRG_Root_X1_der), client->buf, client->buf_len) != 0) {
        LogError(("slack_client_open_app_connection: https_client_init failed!"));
        return -1;
    }

    if (wss_client_init(&client->wss, ISRG_Root_X1_der, sizeof(ISRG_Root_X1_der), client->buf, client->buf_len) != 0) {
        LogError(("slack_client_open_app_connection: wss_client_init failed!"));
        return -1;
    }

    char auth_header[128];

    snprintf(
        auth_header,
        sizeof(auth_header),
        "Bearer %s",
        client->app_token
    );

    enum HTTPStatus status = https_client_post(
        &client->https,
        "slack.com",
        "/api/apps.connections.open",
        (const char*[]){
            "Authorization", auth_header
        },
        1,
        NULL,
        0
    );

    if (status != HTTPSuccess) {
        LogError(("slack_client_open_app_connection: status != HTTPSuccess"));
        return -1;
    }

    if (client->https.response.statusCode != 200) {
        LogError(("slack_client_open_app_connection: client->https.response.statusCode = %u", client->https.response.statusCode));
        return -1;
    }

    cJSON* json = cJSON_ParseWithLength(client->https.response.pBody, client->https.response.bodyLen);

    if (json == NULL) {
        LogError(("slack_client_open_app_connection: cJSON_ParseWithLength failed!"));
        return -1;
    }

    cJSON* ok_json = cJSON_GetObjectItemCaseSensitive(json, "ok");
    if (ok_json == NULL) {
        LogError(("slack_client_open_app_connection: no 'ok' field in response!"));
        cJSON_Delete(json);
        return -1;
    }

    if (cJSON_IsFalse(ok_json)) {
        LogError(("slack_client_open_app_connection: 'ok' response value is false!"));
        cJSON_Delete(json);
        return -1;
    }

    cJSON* url_json = cJSON_GetObjectItemCaseSensitive(json, "url");
    if (url_json == NULL) {
        LogError(("slack_client_open_app_connection: no 'url' field in response!"));
        cJSON_Delete(json);
        return -1;
    }

    const char* url = cJSON_GetStringValue(url_json);
    if (url == NULL) {
        LogError(("slack_client_open_app_connection: 'url' field in response is not a string!"));
        cJSON_Delete(json);
        return -1;
    }

    LogDebug(("slack_client_open_app_connection: url = %s", url));

    if (strstr(url, "wss://") != url) {
        LogError(("slack_client_open_app_connection: 'url' field in response is not a string!"));
        cJSON_Delete(json);
        return -1;
    }

    char wss_host[64];
    char wss_path[256];

    const char* host_start = url + 6;
    const char* path_start = strchr(host_start, '/');

    memset(wss_host, 0x00, sizeof(wss_host));
    memset(wss_path, 0x00, sizeof(wss_path));

    strncpy(wss_host, host_start, (path_start - host_start));
    strncpy(wss_path, path_start, sizeof(wss_path));

    // append debug_reconnects to URL to shorten connection time
    strncat(wss_path, "&debug_reconnects=true", sizeof(wss_path) - 1);

    cJSON_Delete(json);

    status = ws_client_open(&client->wss, wss_host, wss_path);

    if (status != HTTPSuccess) {
        LogError(("slack_client_open_app_connection: ws_client_open failed!"));
        return -1;
    }

    return 0;
}

cJSON* slack_client_poll(slack_client_t* client)
{
    if (!ws_client_connected(&client->wss)) {
        wss_client_close(&client->wss);

        LogDebug(("slack_client_poll: opening app connection"));

        if (slack_client_open_app_connection(client) != 0) {
            LogError(("slack_client_poll: Failed to open app connection!"));
            return NULL;
        }

        LogDebug(("slack_client_poll: app connection opened"));
    }

    uint8_t type;
    int result = wss_client_read(&client->wss, &type, client->buf, client->buf_len);

    if (result <= 0) {
        return NULL;
    }

    if (type == WEBSOCKET_OPCODE_PING) {
        LogDebug(("slack_client_poll: got ping, sending pong ..."));
        // ping, send pong
        wss_client_write(&client->wss, WEBSOCKET_OPCODE_PONG, client->buf, result);

        return NULL;
    } else if (type == WEBSOCKET_OPCODE_CONNECTION_CLOSE) {
        LogDebug(("slack_client_poll: got connection close"));

        wss_client_close(&client->wss);

        return NULL;
    }

    cJSON* json = cJSON_ParseWithLength(client->buf, result);

    return json;
}

int slack_client_acknowledge_event(slack_client_t* client, const char* envelope_id, cJSON* payload)
{
    cJSON* json = cJSON_CreateObject();
    if (json == NULL) {
        return -1;
    }

    cJSON* envelope_id_json = cJSON_CreateString(envelope_id);
    if (envelope_id_json == NULL) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "envelope_id", envelope_id_json);


    if (payload != NULL) {
        cJSON_AddItemToObject(json, "payload", payload);
    }

    char* body = cJSON_PrintUnformatted(json);
    if (body == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    LogDebug(("slack_client_acknowledge_event: body = %s", body));

    int result = wss_client_write(&client->wss, WEBSOCKET_OPCODE_TEXT, body, strlen(body));

    cJSON_Delete(json);

    return 0;
}

int slack_client_post_message(slack_client_t* client, const char* text, const char* channel)
{
    if (https_client_init(&client->https, ISRG_Root_X1_der, sizeof(ISRG_Root_X1_der), client->buf, client->buf_len) != 0) {
        return -1;
    }

    char auth_header[128];

    snprintf(
        auth_header,
        sizeof(auth_header),
        "Bearer %s",
        client->bot_token
    );

    cJSON* json = cJSON_CreateObject();
    if (json == NULL) {
        return -1;
    }

    cJSON* text_json = cJSON_CreateString(text);
    if (text_json == NULL) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "text", text_json);

    cJSON* channel_json = cJSON_CreateString(channel);
    if (channel_json == NULL) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON_AddItemToObject(json, "channel", channel_json);

    char* body = cJSON_PrintUnformatted(json);
    if (body == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    enum HTTPStatus status = https_client_post(
        &client->https,
        "slack.com",
        "/api/chat.postMessage",
        (const char*[]){
            "Authorization", auth_header,
            "Content-Type", "application/json;charset=utf8"
        },
        2,
        body,
        strlen(body)
    );

    if (status != HTTPSuccess) {
        LogError(("slack_client_post_message: status != HTTPSuccess, %d", status));
        cJSON_Delete(json);
        return -1;
    }

    if (client->https.response.statusCode != 200) {
        LogError(("slack_client_post_message: client->https.response.statusCode = %d", client->https.response.statusCode));
        cJSON_Delete(json);
        return -1;
    }

    cJSON_Delete(json);

    json = cJSON_ParseWithLength(client->https.response.pBody, client->https.response.bodyLen);
    if (json == NULL) {
        LogError(("slack_client_post_message: cJSON_ParseWithLength failed!"));
        return -1;
    }

    cJSON* ok_json = cJSON_GetObjectItemCaseSensitive(json, "ok");
    if (ok_json == NULL) {
        LogError(("slack_client_post_message: no 'ok' field in response!"));
        cJSON_Delete(json);
        return -1;
    }

    if (cJSON_IsFalse(ok_json)) {
        LogError(("slack_client_post_message: 'ok' response value is false!"));
        cJSON_Delete(json);
        return -1;
    }

    cJSON_Delete(json);

    return 0;
}
