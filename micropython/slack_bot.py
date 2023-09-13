import json

import websocket_client

from urequests import request


class SlackBot:
    def __init__(self, app_token, bot_token):
        self._app_token = app_token
        self._bot_token = bot_token
        self._ws = None

    def post_message(self, text, channel):
        response = request(
            "POST",
            "https://slack.com/api/chat.postMessage",
            headers={
                "Authorization": f"Bearer {self._bot_token}",
                "Content-Type": "application/json;charset=utf8",
            },
            data=json.dumps({"channel": channel, "text": text}),
        )

        response_json = response.json()
        response.close()

        print(response_json)

        if not response_json["ok"]:
            raise Exception(response_json["error"])

    def poll(self):
        if self._ws is None or not self._ws.connected():
            response = request(
                "POST",
                "https://slack.com/api/apps.connections.open",
                headers={"Authorization": f"Bearer {self._app_token}"},
            )

            open_json = response.json()
            response.close()

            ws_url = open_json["url"]
            ws_url += "&debug_reconnects=true"

            self._ws = websocket_client.WebSocket()
            self._ws.connect(ws_url)

        rx, rx_type = self._ws.recv()
        if rx is None:
            return None

        if rx_type == websocket_client.TYPE_PING:
            # received ping, response with pong
            self._ws.send(rx, websocket_client.TYPE_PONG)
            return None

        event = json.loads(rx)
        event_type = event["type"]

        return event

    def acknowledge_event(self, envelope_id, payload=None):
        msg = {"envelope_id": envelope_id}

        if payload is not None:
            msg["payload"] = payload

        self._ws.send(json.dumps(msg), websocket_client.TYPE_TEXT)
