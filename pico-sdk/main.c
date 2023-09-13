#include <string.h>
#include <unistd.h>

#include <FreeRTOS.h>
#include <task.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "logging.h"
#include "slack_client.h"

extern char *strcasestr(const char *haystack, const char *needle);

void main_task(void*);
void handle_event(cJSON* event_json);

char buf[2048];
slack_client_t slack_client;

int main(void)
{
    stdio_init_all();
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    LogInfo(("Starting FreeRTOS on core 0"));

    xTaskCreate(main_task, "MainTask", 2048, NULL, (tskIDLE_PRIORITY + 1UL), NULL);
    vTaskStartScheduler();

    return 0;
}

void main_task(void*)
{
    if (cyw43_arch_init()) {
        LogError(("Failed to initialize Wi-Fi!"));
        while (true) { vTaskDelay(100); }
    }
    cyw43_arch_enable_sta_mode();

    LogInfo(("Connecting to Wi-Fi SSID '%s'", WIFI_SSID));
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        LogError(("Failed to connect to Wi-Fi SSID '%s' !", WIFI_SSID));
        while (true) { vTaskDelay(100); }
    } else {
        LogInfo(("Connected to Wi-Fi SSID '%s'", WIFI_SSID));
    }

    if (slack_client_init(&slack_client, SLACK_BOT_TOKEN, SLACK_APP_TOKEN, buf, sizeof(buf)) != 0) {
        LogError(("Failed to initialize Slack client!"));
        while(true) { vTaskDelay(100); }    
    }

    slack_client_post_message(&slack_client, "Hello Slack!", "pico-w-app-test");

    while (1) {
        cJSON* event_json = slack_client_poll(&slack_client);
        if (event_json == NULL) {
            vTaskDelay(0);
            continue;
        }
        
        handle_event(event_json);

        cJSON_Delete(event_json);
    }

    cyw43_arch_deinit();
}

void handle_event(cJSON* event_json)
{
    LogDebug(("Received event:\n%s", cJSON_Print(event_json)));

    const char* event_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(event_json, "type"));

    LogInfo(("event_type: = %s", event_type));

    if (strcmp(event_type, "hello") == 0) {
        LogInfo(("\tGot hello"));
    } else if (strcmp(event_type, "disconnect") == 0) {
        const char* reason = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(event_json, "reason"));

        LogInfo(("\tGot disconnect, reason = %s", reason));
    } else if (strcmp(event_type, "events_api") == 0) {
        cJSON* payload_json = cJSON_GetObjectItemCaseSensitive(event_json, "payload");

        const char* envelope_id = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(event_json, "envelope_id"));
        const char* payload_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(payload_json, "type"));

        LogInfo(("\tenvelope_id = %s", envelope_id));
        LogInfo(("\tpayload_type: = %s", payload_type));
        
        if (strcmp(payload_type, "event_callback") == 0) {
            cJSON* payload_event_json = cJSON_GetObjectItemCaseSensitive(payload_json, "event");

            const char* payload_event_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(payload_event_json, "type"));
            const char* payload_event_channel = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(payload_event_json, "channel"));
            const char* post_message_text = NULL;
            
            LogInfo(("\t\tpayload_event_type = %s", payload_event_type));
            LogInfo(("\t\tpayload_event_channel = %s", payload_event_channel));

            if (strcmp(payload_event_type, "app_mention") == 0) {
                const char* text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(payload_event_json, "text"));

                LogInfo(("\t\t\tapp_mention: %s", text));

                if (strcasestr(text, "led on") != NULL) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

                    post_message_text = "LED is now on :bulb:";
                } else if (strcasestr(text, "led off") != NULL) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

                    post_message_text = "LED is now off";
                }
            }

            slack_client_acknowledge_event(&slack_client, envelope_id, NULL);

            if (post_message_text != NULL) {
                LogInfo(("Posting message '%s' to channel = '%s'", post_message_text, payload_event_channel));
                slack_client_post_message(&slack_client, post_message_text, payload_event_channel);
            }
        }
    }
}
