#!/usr/bin/env python3

import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect(("localhost", 8083))

sending = b"\x08\x07\x5B\x74"

with open("testing/large.txt", "rb") as f:
    sending += f.read()
sending = sending.strip()

client.sendall(sending)

received_bytes = 0
while received_bytes < len(sending):
    received = client.recv(16_384)
    received_bytes += len(received)
    print(". ({}/{})".format(received_bytes, len(sending)))

print("Received all of the data...")
client.close()
