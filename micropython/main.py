#
# SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: MIT
#


import network

from machine import Pin
from slack_bot import SlackBot

import config

led = Pin("LED")
led.off()

print(f"Connecting to Wi-Fi SSID: {config.WIFI_SSID}")

# initialize the Wi-Fi interface
wlan = network.WLAN(network.STA_IF)

# activate and connect to the Wi-Fi network:
wlan.active(True)
wlan.connect(config.WIFI_SSID, config.WIFI_PASSWORD)

while not wlan.isconnected():
    time.sleep(0.5)

print(f"Connected to Wi-Fi SSID: {config.WIFI_SSID}")

slack_bot = SlackBot(config.SLACK_APP_TOKEN, config.SLACK_BOT_TOKEN)

while True:
    event = slack_bot.poll()

    if event is None:
        continue

    print("event", event)

    event_type = event["type"]

    if event_type == "hello":
        print("Got hello")
    elif event_type == "events_api":
        envelope_id = event["envelope_id"]
        payload_type = event["payload"]["type"]
        ack_payload = None

        if payload_type == "event_callback":
            payload_event_type = event["payload"]["event"]["type"]
            payload_event_channel = event["payload"]["event"]["channel"]
            post_msg_text = None

            if payload_event_type == "app_mention":
                text = event["payload"]["event"]["text"]

                print("app_mention", text)

                if "led on" in text.lower():
                    led.on()
                    post_msg_text = "The LED is now on :bulb:"
                elif "led off" in text.lower():
                    led.off()
                    post_msg_text = "The LED is now off"

            slack_bot.acknowledge_event(envelope_id, ack_payload)

            if post_msg_text is not None:
                slack_bot.post_message(post_msg_text, payload_event_channel)
