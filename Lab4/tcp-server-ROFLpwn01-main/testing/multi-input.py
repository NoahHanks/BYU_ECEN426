#!/usr/bin/env python3

import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(("localhost", 8083))

sending = [
    b"\x20\x00\x00\x13The LAN Before Time",
    b"\x08\x00\x00\x16Pretty Fly for a Wi-Fi",
    b"\x40\x00\x00\x10The Promised LAN",
    b"\x10\x00\x00\x18WI-FIght the inevitable?",
]
sending = b"".join(sending)

client.sendall(sending)

received = b""
while len(received) < len(sending):
    received += client.recv(1024)
print(received)

client.close()
