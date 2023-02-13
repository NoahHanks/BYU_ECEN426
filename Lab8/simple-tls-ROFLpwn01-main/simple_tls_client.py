import logging
import argparse
import sys
import socket
import secrets
import struct
import crypto_utils
from PIL import Image

from message import *
import paho.mqtt.client as mqtt
import time

FIELD1 = 0b11111111000000000000000000000000  # first 8 bits
FIELD2 = 0b00000000111111111111111111111111  # next 24 bits

error_type = 0x00
hello_type = 0x01
certificate_type = 0x02
encrypted_nonce_type = 0x03
hash_type = 0x04
data_type = 0x05

host = "localhost"
port = 8087
write_to_stdout = False
data_buf = 1024


# Creates the help menu and various argument options
parser = argparse.ArgumentParser(add_help=False)

# Optional arguments
parser.add_argument(
    "-h",
    "--help",
    action="help",
    default=argparse.SUPPRESS,
    help="show this help message and exit",
)
parser.add_argument("-p", "--port", type=int, default=port, help="Port to connect to.")
parser.add_argument("--host", type=str, default=host, help="Hostname to connect to.")
parser.add_argument(
    "-v", "--verbose", action="store_true", help="Turn on debugging output."
)

# Required arguments
parser.add_argument(
    "file",
    type=str,
    help="The file name to save to. It must be a PNG file extension. Use - for stdout.",
)

args = parser.parse_args()
host = args.host

# Sets config for logger
logger = logging.getLogger("")
ch = logging.StreamHandler()
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
ch.setFormatter(formatter)
logger.addHandler(ch)

file = args.file

# If file name was not specified
if file == "-":
    write_to_stdout = True

# If verbose flag is enabled
if args.verbose:
    logger.setLevel(logging.DEBUG)

nonce = secrets.token_bytes(32)


# Creates the socket and listens etc.
server_socket = socket.socket()
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(("localhost", port))
server_socket.listen()

logger.info("Config:")
logger.info(f"     port: {port}")
logger.info(f"     host: {host}")
logger.info("Creating socket...")
logger.info("Listening on socket...")


try:
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.connect((host, port))
except:
    print("Could not make a connection to the server")
    input("Press enter to quit")
    sys.exit(0)

################
#    Hello     #
################
logger.info("Sending Hello")
message1_header = hello_type.to_bytes(4, "little")

conn.send(hello_type.to_bytes(4, "little"))


################################
#    Nonce + Cerificate        #
################################
header = conn.recv(4)
unpacked = struct.unpack("!i", header)
message2_header = header
type = (unpacked[0] & FIELD1) >> 24
length = unpacked[0] & FIELD2

logger.info("Receiving Nonce + Cerificate")
data_raw = conn.recv(length)
message2_data = data_raw
data = data_raw.decode("cp437")
if len(data) < length:
    logger.error("Message not fully received")
    conn.close()
if type != certificate_type:
    logger.error("Unexpected message type.")
    conn.close()

server_nonce = data[0:32]
server_nonce_bytes = data_raw[0:32]
certificate_str = data[32:]
certificate_bytes = data_raw[32:]

logger.info("Verifying certificate")
cert = crypto_utils.load_certificate(certificate_bytes)
if cert == None:
    logger.error("Certificate is none. :(")
else:
    logger.info("Certificate is good. :)")


################################
#    Encrypted Nonce           #
################################
logger.info("Preparing and sending Encrypted Nonce")

server_public_key = cert.public_key()
encyrpted_nonce = crypto_utils.encrypt_with_public_key(server_public_key, nonce)

encrypted_nonce_header = encrypted_nonce_type.to_bytes(1, "little")
encrypted_nonce_header += len(encyrpted_nonce).to_bytes(3, "little")
message3_header = encrypted_nonce_header
message3_data = encyrpted_nonce

conn.send(encrypted_nonce_header)
conn.send(encyrpted_nonce)

four_keys = crypto_utils.generate_keys(nonce, server_nonce_bytes)

################################
#    Server Hash               #
################################
header = conn.recv(4)
unpacked = struct.unpack("!i", header)
type = (unpacked[0] & FIELD1) >> 24
length = unpacked[0] & FIELD2

logger.info("Receiving server hash.")
data_raw = conn.recv(length)
data = data_raw.decode("cp437")
if len(data) < length:
    logger.error("Message not fully received")
    conn.close()
if type != hash_type:
    logger.error("Unexpected message type.")
    conn.close()

server_hash = data_raw[0:]


################################
#    Client Hash               #
################################
logger.info("Preparing and sending client MAC")

three_messages = (
    message1_header + message2_header + message2_data + message3_header + message3_data
)
client_integrity_key = four_keys[3]
server_integrity_key = four_keys[1]

client_mac = crypto_utils.mac(
    three_messages,
    client_integrity_key,
)

server_mac = crypto_utils.mac(
    three_messages,
    server_integrity_key,
)

logger.info("Verifying server hash")
if server_hash != server_mac:
    logger.error("Server hash not equal to server MAC.")
    conn.close()

client_hash_header = hash_type.to_bytes(3, "little")
client_hash_header += b"\x20"

conn.send(client_hash_header)
conn.send(client_mac)

################################
#    Encrypted Data            #
################################
server_encryption_key = four_keys[0]

data_image = b""
sequence_counter = 0

while True:
    try:
        message = Message.from_socket(conn)
        if message == None:
            break

        decrypted_data = crypto_utils.decrypt(message.data, server_encryption_key)

        sequence_number = int.from_bytes(decrypted_data[:4], byteorder="big")
        data_chunk = decrypted_data[4:-32]
        mac_from_data = decrypted_data[-32:]

        if sequence_counter != sequence_number:
            logger.debug(
                f"Sequence numbers dont match. Actual {sequence_counter}, Expected {sequence_number}."
            )
            continue
        sequence_counter += 1

        mac_calculated_from_data_chunk = crypto_utils.mac(
            data_chunk, server_integrity_key
        )
        if mac_calculated_from_data_chunk != mac_from_data:
            logger.debug("MACs dont match.")
            continue

        data_image += data_chunk

    except AttributeError:
        break


logger.info(f"Last sequence number: {sequence_counter}")

if args.file == "-":
    logger.info("Writing image data to stdout.")
    sys.stdout.buffer.write(data_image)
else:
    logger.info("Writing image data to file.")
    with open(args.file + ".png", "wb") as file:
        file.write(data_image)
