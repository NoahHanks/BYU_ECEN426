#!/usr/bin/env python3

import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(("localhost", 8093))

sending = b"\x20\x00\x00\x13The LAN Before Time"

client.sendall(sending)

received = b""
while len(received) < len(sending):
    received += client.recv(1024)
print(received)

client.close()
