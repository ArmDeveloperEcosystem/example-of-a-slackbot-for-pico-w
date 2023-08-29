#ifndef __SLACK_CLIENT_H__
#define __SLACK_CLIENT_H__

#include <cJSON.h>

#include "https_client.h"
#include "wss_client.h"

typedef struct {
    const char* bot_token;
    const char* app_token;
    https_client_t https;
    wss_client_t wss;
    char* buf;
    size_t buf_len;
} slack_client_t;

int slack_client_init(slack_client_t* client, const char* bot_token, const char* app_token, char* buf, size_t buf_len);

cJSON* slack_client_poll(slack_client_t* client);

int slack_client_acknowledge_event(slack_client_t* client, const char* envelope_id, cJSON* payload);

int slack_client_post_message(slack_client_t* client, const char* text, const char* channel);

#endif
