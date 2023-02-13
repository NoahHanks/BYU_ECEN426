import socket
import threading
import random
import logging
from struct import *
import array

# import buffer

string0 = b"test string"
# "string".encode("ascii")
# ' '.join(format(ord(x), "b") for x in string)
s = bytearray(1000000)
# pack("s", 1)
string2 = pack("hhl", 1, 2, 3)
string5 = b"\x20\x00\x00\x13\x54\x68\x65\x20\x4C\x41\x4E\x20\x42\x65\x66\x6F\x72\x65\x20\x54\x69\x6D\x65".decode(
    "utf-8"
)
string3 = calcsize("hhl")
string4 = pack("p", string0)
t = memoryview(s)


s = "ABCD"
b = bytearray()
b.extend(map(ord, s))

print(t)
# print(s)
print(string0)
print(string2)
print(string3)
print(string4)
print(string5)
print(b)
