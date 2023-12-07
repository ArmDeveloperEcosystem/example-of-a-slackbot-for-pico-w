#
# SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: MIT
#


import random

from ubinascii import b2a_base64
import usocket
import ussl

TYPE_TEXT = const(0x1)
TYPE_BINARY = const(0x2)
TYPE_CONNECTION_CLOSE = const(0x8)
TYPE_PING = const(0x9)
TYPE_PONG = const(0xA)


class WebSocket:
    def __init__(self):
        self._stream = None

    def connect(self, url):
        try:
            proto, _, host, path = url.split("/", 3)
        except ValueError:
            proto, _, host = url.split("/", 2)
            path = ""

        if proto == "ws:":
            port = 80
        elif proto == "wss:":
            port = 443
        else:
            raise Exception("Unsupported URL!")

        if ":" in host:
            host, port = host.split(":", 1)

        sockaddr = usocket.getaddrinfo(host, int(port), 0, usocket.SOCK_STREAM)[0][-1]

        s = usocket.socket()
        try:
            s.connect(sockaddr)
            if proto == "wss:":
                s = ussl.wrap_socket(s, server_hostname=host)

            key = bytearray(16)
            for i in range(len(key)):
                key[i] = random.randint(0, 255)

            s.write(f"GET /{path} HTTP/1.1\r\n")
            s.write(f"Host: {host}\r\n")
            s.write("Upgrade: websocket\r\n")
            s.write("Connection: Upgrade\r\n")
            s.write(f"Sec-WebSocket-Key: {b2a_base64(key)[:-1]}\r\n")
            s.write("Sec-WebSocket-Version: 13\r\n")
            s.write("\r\n")

            l = s.readline()
            l = l.split(b" ", 2)
            status_code = int(l[1])
            while True:
                l = s.readline()
                if not l or l == b"\r\n":
                    break
        except OSError as ose:
            s.close()
            raise ose

        if status_code != 101:
            raise Exception(f"Received HTTP {status_code} on connection upgrade!")

        self._stream = s

    def recv(self):
        self._stream.setblocking(False)
        header = self._stream.read(2)

        if header is None:
            return None, None

        if len(header) == 0:
            self._stream = None
            return None, None

        msgtype = header[0] & 0x7F
        length = header[1]

        if length < 126:
            pass
        elif length == 126:
            length = self._stream.read(2)
            length = (length[0] << 8) | length[1]
        else:
            raise Exception(f"Unsupported length {length}!")

        self._stream.setblocking(True)
        return self._stream.read(length), msgtype

    def send(self, data, msgtype=0x01):
        data_len = len(data)
        if data_len < 126:
            header = bytearray(2 + 4)
            header[0] = 0x80 | msgtype
            header[1] = 0x80 | data_len
        elif data_len < 0xFFFF:
            header = bytearray(4 + 4)
            header[0] = 0x80 | msgtype
            header[1] = 0x80 | 126
            header[2] = (data_len >> 8) & 0xFF
            header[3] = (data_len >> 0) & 0xFF
        else:
            raise Exception("Unsupported data length {data_len}!")

        self._stream.write(header)
        self._stream.write(data)

    def close(self):
        self._stream.close()
        self._stream = None

    def connected(self):
        return self._stream is not None
