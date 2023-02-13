#!/usr/bin/env python3

import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(("localhost", 8083))

sending = b"\x30\x00\x00\x13The LAN Before Time"

client.sendall(sending)

received = client.recv(1024)
print(received)

client.close()
