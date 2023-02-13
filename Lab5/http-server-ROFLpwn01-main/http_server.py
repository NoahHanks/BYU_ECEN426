import socket
import logging
import argparse
import time

host = ""
port = 8084
data_buf = 1024
delay = False
folder = "."

# Creates the help menu and various argument options
parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--help", action="help", default=argparse.SUPPRESS, help="")
parser.add_argument("-v", "--verbose", action="store_true")
parser.add_argument("-d", "--delay", action="store_true")
parser.add_argument("--port", "-p", type=int, default=port)
parser.add_argument("--folder", "-f", type=str, default=folder)

args = parser.parse_args()

# Sets config for logger
logger = logging.getLogger("")
ch = logging.StreamHandler()
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
ch.setFormatter(formatter)
logger.addHandler(ch)

# If verbose flag is enabled
if args.verbose:
    logger.setLevel(logging.DEBUG)

# If delay flag is enabled
if args.delay:
    delay = True

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
        connected = True

        while connected:
            logger.info("Receiving header")
            request = conn.recv(data_buf).decode()

            # request_raw = conn.recv(data_buf)
            # if not request_raw:
            #     logger.info("Client disconnected...")
            #     break

            # request = request_raw.decode()

            # Splitting the request into strings
            input = request.split()
            method = request.partition(" ")[0]
            logger.debug("Request: " + request)

            # Finding the file and directory
            file_name = request.partition(" ")[2]
            file_and_path = args.folder + str(file_name.partition(" ")[0])
            logger.info("File Name: " + file_name.partition(" ")[0][1::])
            logger.info("File with path: " + file_and_path)

            # Sending header
            logger.info("Sending HTTP header to client...")
            header = "HTTP/1.0 200 OK\n\n"
            conn.send(header.encode())

            # Opening the file from the request
            logger.info("Trying to open file...")
            try:
                if method == "GET":
                    file = open(file_and_path, "rb")
                else:
                    file = open("www/405.html", "rb")
            except FileNotFoundError:
                logger.error("File not Found! Sending 404.")
                file = open("www/404.html", "rb")
            data = file.read(data_buf)

            # For delay flag
            if delay:
                logger.info("Delay is on, waiting 5 seconds...")
                time.sleep(5)

            # Taking care of the file
            logger.info("Sending file...")
            while data:
                conn.send(data)
                data = file.read(data_buf)
            logger.info("Closing file...")
            file.close()

            if not request:
                logger.info("Client disconnected...")
                connected = False
                break

            break

        conn.close()

except KeyboardInterrupt:
    print("Ctrl-c")
    exit()
