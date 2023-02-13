import socket
import threading
import random
import logging
import argparse
import methods
import struct


host = ""
port = 8083
data_buf = 1024
format_string = "!i"

FIELD1 = 0b11111000000000000000000000000000  # first 5 bits
FIELD2 = 0b00000111111111111111111111111111  # next 27 bits


# Creates the help menu and various argument options
parser = argparse.ArgumentParser()
parser.add_argument(
    "-v", "--verbose", help="increase output verbosity", action="store_true"
)
parser.add_argument("--port", "-p", type=int, default=port)
args = parser.parse_args()

# Sets config for logger
logger = logging.getLogger("")
ch = logging.StreamHandler()
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
ch.setFormatter(formatter)
logger.addHandler(ch)

# If verbose flag is enabled
if args.verbose:
    logger.setLevel(logging.INFO)

# Creates the socket and listens etc.
server_socket = socket.socket()
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind((host, port))
server_socket.listen()

logger.info("Config:")
logger.info(f"     port: {port}")
logger.info("Creating socket...")
logger.info("Listening on socket...")

# General Server connection
try:
    while True:

        logger.info("Waiting for clients...")

        conn, address = server_socket.accept()
        logger.info("Got a new client!")
        logger.info(f"Connection from: {address}")
        receiving_header = True

        while True:
            logger.info("Receiving header")
            data = conn.recv(4)
            if not data:
                logger.info("Client disconnected...")
                break
            unpacked = struct.unpack("!i", data)
            action = (unpacked[0] & FIELD1) >> 27
            message_length = unpacked[0] & FIELD2

            logger.info("Receiving message data")
            msg = conn.recv(message_length).decode("cp437")
            if len(msg) < message_length:
                logger.error("Message not fully received")
                conn.close()
                break

            match action:
                case methods.Vars.UPPERCASE:
                    response_msg = msg.upper()
                    logger.info("Request:")
                    logger.info("    Action: uppercase")
                    logger.info(f"    Length: {message_length}")
                    logger.info(f"    Message: {msg}")
                case methods.Vars.LOWERCASE:
                    response_msg = msg.lower()
                    logger.info("Request:")
                    logger.info("    Action: lowercase")
                    logger.info(f"    Length: {message_length}")
                    logger.info(f"    Message: {msg}")
                case methods.Vars.REVERSE:
                    response_msg = msg[::-1]
                    logger.info("Request:")
                    logger.info("    Action: reverse")
                    logger.info(f"    Length: {message_length}")
                    logger.info(f"    Message: {msg}")
                case methods.Vars.SHUFFLE:
                    response_msg = "".join(random.sample(msg, len(msg)))
                    logger.info("Request:")
                    logger.info("    Action: shuffle")
                    logger.info(f"    Length: {message_length}")
                    logger.info(f"    Message: {msg}")
                case methods.Vars.RANDOM:
                    response_msg = methods.randomize_text(msg)
                    logger.info("Request:")
                    logger.info("    Action: random")
                    logger.info(f"    Length: {message_length}")
                    logger.info(f"    Message: {msg}")
                case _:
                    response_msg = msg
                    logger.error("Invalid action code used")
                    print("error")
                    conn.close()
                    break

            logger.info("Response:")
            logger.info(f"    Message: {response_msg}\n")

            if not data:
                logger.info("Client disconnected...")
                break

            conn.send(struct.pack("!i", len(response_msg)))
            conn.send(response_msg.encode())

        conn.close()

except KeyboardInterrupt:
    print("Ctrl-c")
    exit()
