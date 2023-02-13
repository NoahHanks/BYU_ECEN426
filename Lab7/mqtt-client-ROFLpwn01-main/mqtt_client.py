import logging
import argparse
import paho.mqtt.client as mqtt
import time

host = "localhost"
port = 1883
data_buf = 1024
actionChoices = ["uppercase", "lowercase", "reverse", "shuffle", "random"]


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
parser.add_argument(
    "-v", "--verbose", action="store_true", help="Turn on debugging output."
)
parser.add_argument("--host", type=str, default=host, help="Defaults to localhost")
parser.add_argument("-p", "--port", type=int, default=port, help="Port to bind to")

# Required arguments
parser.add_argument(
    "netid",
    type=str,
)
parser.add_argument(
    "action",
    type=str,
)
parser.add_argument(
    "message",
    type=str,
)

args = parser.parse_args()

# Sets config for logger
logger = logging.getLogger("")
ch = logging.StreamHandler()
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
ch.setFormatter(formatter)
logger.addHandler(ch)

host = args.host
netid = args.netid
action = args.action
message = args.message

# If verbose flag is enabled
if args.verbose:
    logger.setLevel(logging.DEBUG)

logger.info(f"netID: {netid}")
if action in actionChoices:
    logger.info(f"Action: {action}")
else:
    logger.error("Invalid action.")
logger.info(f"Message: {message}")


# Method global variables
gotMessage = False
subscribed = False


def on_connect(mqttc, obj, flags, rc):
    if rc == 0:
        connected = "True"
    else:
        connected = "False"
    logging.info(f"Connected: {connected}")


def on_message(mqttc, obj, msg):
    global gotMessage
    gotMessage = True
    messageReceived = msg.payload.decode("utf-8")
    print(f"{messageReceived}")


def on_publish(mqttc, obj, mid):
    logging.info(f"mid(Message ID): {str(mid)}")


def on_subscribe(mqttc, obj, mid, granted_qos):
    global subscribed
    subscribed = True
    logging.info("Subscription successful.")


def main(netid, action, message, host, port):

    # Create client and its functions
    mqttc = mqtt.Client()
    mqttc.on_message = on_message
    mqttc.on_connect = on_connect
    mqttc.on_publish = on_publish
    mqttc.on_subscribe = on_subscribe

    # Try to connect
    try:
        mqttc.connect(host, port, 60)
    except:
        logging.error(f"Could not connect to broker.")
        return 1
    mqttc.loop_start()

    # Subscribtion
    topicRequest = netid + "/" + action + "/response"
    mqttc.subscribe(topicRequest, 1)
    while not subscribed:
        time.sleep(0.05)

    # Publish
    topicResponse = netid + "/" + action + "/request"
    infot = mqttc.publish(topicResponse, message, 1)
    infot.wait_for_publish()

    # Waiting for message
    while not gotMessage:
        time.sleep(0.05)


if __name__ == "__main__":
    main(netid, action, message, host, port)
