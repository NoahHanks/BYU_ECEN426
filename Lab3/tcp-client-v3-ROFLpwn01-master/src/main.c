#include <stdio.h>

#include "log.h"
#include "tcp_client.h"

static size_t messagesSent = 0;
static size_t messagesReceived = 0;

void printInfoMenuMain() {
    fprintf(stderr, "\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\n"
                    "Arguments:\n"
                    "ACTION   Must be uppercase, lowercase, rreverse,\n"
                    "\t   shuffle, or random.\n"
                    "MESSAGE  Message to send to the server\n\n"
                    "Options:\n"
                    "  --help\n"
                    "  -v, --verbose\n"
                    "  --host HOSTNAME, -h HOSTNAME\n"
                    "  --port PORT, -p PORT\n");
}

int handle_response(char *response) {
    printf("%s\n", response);
    messagesReceived++;
    if (messagesSent > messagesReceived)
        return 0;
    else
        return 1;
}

int main(int argc, char *argv[]) {

    Config defaultValues = {TCP_CLIENT_DEFAULT_PORT, TCP_CLIENT_DEFAULT_HOST, ""};
    int socket;

    log_set_level(LOG_ERROR);

    int result = tcp_client_parse_arguments(argc, argv, &defaultValues);
    if (result != 0) {
        log_warn("Incorrect arguments provided");
        printInfoMenuMain();
        exit(EXIT_FAILURE);
    }

    log_info("host: %s, port: %s", defaultValues.host, defaultValues.port);

    socket = tcp_client_connect(defaultValues);
    if (socket == -1) {
        log_warn("Unable to connect to socket");
        exit(EXIT_FAILURE);
    } else {
        log_trace("Connected to socket.");
    }

    FILE *file = tcp_client_open_file(defaultValues.file);
    log_info("Contents of file descriptor %d.", file);

    if (file != NULL) {
        log_trace("File was successfully opened.");
    } else {
        log_error("There was an error trying to open the file.");
    }

    char *action;
    char *message;
    ssize_t c;

    // Sends data to server while there is still data
    while ((c = tcp_client_get_line(file, &action, &message)) != -1) {

        log_trace("Attempting to send a new send message with action: %s, and message: %s.", action,
                  message);
        if (tcp_client_send_request(socket, action, message)) {
            log_warn("Message was not sent successfully to the server");
            exit(EXIT_FAILURE);
        } else {
            messagesSent++;
        }
        free(action);
        free(message);
    }
    if ((c == -1) && (messagesSent == 0)) {
        log_warn("No messages were sent.");
    }

    log_info("Messages sent: %d, messages received: %d.", messagesSent, messagesReceived);
    tcp_client_receive_response(socket, handle_response);

    if (tcp_client_close_file(file))
        log_error("Error closing file");
    else
        log_trace("File closed");

    if (tcp_client_close(socket)) {
        log_warn("Unable to disconnect");
        exit(EXIT_FAILURE);
    }

    log_info("Program executed successfully");
    exit(EXIT_SUCCESS);
}
