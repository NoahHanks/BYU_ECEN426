import socket
import logging
import argparse
import time
import errno
import signal
import os
import sys
import queue
import threading
import multiprocessing

host = ""
port = 8085
data_buf = 1024
delay = False
folder = "."
thread = 0
process = 0

# Creates the help menu and various argument options
parser = argparse.ArgumentParser(add_help=False)

parser.add_argument(
    "-h",
    "--help",
    action="help",
    default=argparse.SUPPRESS,
    help="show this help message and exit",
)
parser.add_argument("-p", "--port", type=int, default=port, help="Port to bind to")
parser.add_argument(
    "-v", "--verbose", action="store_true", help="Turn on debugging output."
)
parser.add_argument(
    "-d", "--delay", action="store_true", help="Add a delay for debugging purposes."
)
parser.add_argument(
    "-f", "--folder", type=str, default=folder, help="Folder from where to serve from."
)
parser.add_argument(
    "-c",
    "--concurrency",
    choices=["thread", "process"],
    type=str,
    help="Concurrency methodology.",
    default="thread",
)


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

# Sets the server mode from the arugment
concurrency = args.concurrency
if concurrency == "thread":
    thread = 1
    logger.info("thread enabled")
elif concurrency == "process":
    process = 1
    logger.info("process enabled")

# Creates the socket and listens etc.
server_socket = socket.socket()
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind((host, port))
server_socket.listen()

logger.info("Config:")
logger.info(f"     port: {port}")
logger.info("Creating socket...")
logger.info("Listening on socket...")


def signal_handler(signal, frame):
    print("\nprogram exiting gracefully")
    sys.exit(0)


def handle_request(conn):

    logger.info("Receiving header")
    request = conn.recv(data_buf).decode()

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

    if thread == 0:
        conn.shutdown(socket.SHUT_WR)


# General Server connection
while True:

    try:
        signal.signal(signal.SIGINT, signal.SIG_DFL)
        logger.info("Waiting for clients...")
        logger.info("Press Ctrl-C to close server.")
        pid = os.fork()

        try:
            conn, address = server_socket.accept()
            logger.info("Got a new client!")
            logger.info(f"Connection from: {address}")

        except IOError as e:
            code, msg = e.args
            # restart 'accept' if it was interrupted
            if code == errno.EINTR:
                continue
            else:
                raise

        while True:

            if thread:
                logger.info("Starting threads...")
                signal.signal(signal.SIGINT, signal.SIG_IGN)

                t1 = threading.Thread(target=handle_request, args=(conn,))
                t1.start()
                t1.join()
                logger.info("Joined threads...")

                # conn.shutdown(socket.SHUT_WR)
                conn.close()
                break

            elif process:
                logger.info("Starting processors...")
                if pid == 0:  # child
                    signal.signal(signal.SIGINT, signal.SIG_IGN)
                    handle_request(conn)
                    conn.close()
                    break
                else:  # parent
                    signal.signal(signal.SIGINT, signal.SIG_IGN)
                    handle_request(conn)
                    conn.close()
                    break

    except KeyboardInterrupt:
        print("Ctrl-c1")
        exit()
