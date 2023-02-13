#include "tcp_client.h"
#include "log.h"
#include <ctype.h>

#define ARG_ERROR 1
#define MAX_PORT_NUMBER 65535
#define ARGUMENTS 2

/*
Description:
    Prints the info menu.
Arguments:
    None.
Return value:
    None.
*/
void printInfoMenu() {
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

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    int argc: the amount of arguments provided to the program (provided by the main function)
    char *argv[]: the array of arguments provided to the program (provided by the main function)
    Config *config: An empty Config struct that will be filled in by this function.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_parse_arguments(int argc, char *argv[], Config *config) {
    int optionIndex = 0;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            log_set_level(LOG_DEBUG);
            printInfoMenu();
            return ARG_ERROR;
        }
    }

    static struct option longOptions[] = {{"verbose", no_argument, NULL, 'v'},
                                          {"host", required_argument, NULL, 'h'},
                                          {"port", required_argument, NULL, 'p'},
                                          {0, 0, 0, 0}};

    int opt;

    // Loops through until a valid combination of aguments is made
    while (1) {
        opt = getopt_long(argc, argv, "vh:p:", longOptions, &optionIndex);

        if (opt == -1)
            break;

        switch (opt) {
        case 1:
            printInfoMenu();
            break;
        case 'v':
            log_set_level(LOG_DEBUG);
            log_debug("Verbose mode enabled");
            break;
        case 'h':
            log_debug("Host: %s", optarg);
            config->host = optarg;
            break;
        case 'p':
            for (int i = 0; optarg[i] != 0; i++) {
                if (!isdigit(optarg[i])) {
                    log_error("Incorrect port number");
                    printInfoMenu();
                    return ARG_ERROR;
                }
            }
            int port = atoi(optarg);
            if (port > MAX_PORT_NUMBER) {
                log_error("Port number out of available range");
                printInfoMenu();
                return ARG_ERROR;
            }
            log_debug("Port: %s", optarg);
            config->port = optarg;
            break;
        case ':': // Missing option argument
            log_error("Missing option argument\n\n");
            printInfoMenu();
            return ARG_ERROR;
            break;
        case '?':
            log_error("Invalid option\n\n");
            printInfoMenu();
            return ARG_ERROR;
            break;
        default:
            log_error("getopt error code 0%o", opt);
            printInfoMenu();
            return ARG_ERROR;
            break;
        }
    }

    // Must have action and message
    if ((argc - optind) != ARGUMENTS) {
        log_error("\"action\" and \"message\" arguments are required.");
        printInfoMenu();
        return ARG_ERROR;
    } else {
        if (strcmp(argv[optind], "uppercase") && strcmp(argv[optind], "lowercase") &&
            strcmp(argv[optind], "reverse") && strcmp(argv[optind], "shuffle") &&
            strcmp(argv[optind], "random")) {
            log_error("Unknown argument provided");
            printInfoMenu();
            return ARG_ERROR;
        }

        // Set the arguments in config
        config->action = argv[optind];
        config->message = argv[++optind];
        log_debug("\tAction: %s", config->action);
        log_debug("\tMessage: %s", config->message);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////
/////////////////////// SOCKET RELATED FUNCTIONS //////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Creates a TCP socket and connects it to the specified host and port.
Arguments:
    Config config: A config struct with the necessary information.
Return value:
    Returns the socket file descriptor or -1 if an error occurs.
*/
int tcp_client_connect(Config config) {

    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Gets IP

    int returnValue;
    if ((returnValue = getaddrinfo(config.host, config.port, &hints, &res)) != 0) {
        log_error("getaddrinfo failed. %s\n", gai_strerror(returnValue));
        return -1;
    }

    log_info("Creating socket...");
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
        TCP_CLIENT_BAD_SOCKET) {
        log_error("client: socket failed to create");
        return TCP_CLIENT_BAD_SOCKET;
    }

    log_info("Connecting socket...");
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == TCP_CLIENT_BAD_SOCKET) {
        close(sockfd);
        log_error("client: failed to connect");
        return TCP_CLIENT_BAD_SOCKET;
    }

    if (res == NULL) {
        log_error("client: failed to connect");
        return TCP_CLIENT_BAD_SOCKET;
    }

    log_info("Returning sockfd...");
    return sockfd;
}

/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    Config config: A config struct with the necessary information.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, Config config) {
    int messageLength, bytesSent, sendLength;
    char request[TCP_CLIENT_MAX_INPUT_SIZE];

    // Find message length
    messageLength = strlen(config.message);
    log_info("Configuring message...");

    // Create request
    sprintf(request, "%s %d %s", config.action, messageLength, config.message);
    sendLength = strlen(request);
    log_debug("Sending message... \"%s\"", request);

    // Send message
    bytesSent = send(sockfd, request, sendLength, 0);
    if (bytesSent < (ssize_t)strlen(request)) {
        log_error("Error with sending.");
        return 1;
    }
    log_debug("Bytes sent: %d", bytesSent);
    return 0;
}

/*
Description:
    Receives the response from the server. The caller must provide an already allocated buffer.
Arguments:
    int sockfd: Socket file descriptor
    char *buf: An already allocated buffer to receive the response in
    int buf_size: The size of the allocated buffer
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, char *buf, int buf_size) {
    int returnValue;
    log_info("Receiving response...");
    if ((returnValue = recv(sockfd, buf, buf_size, 0)) <= 0) {
        log_error("Receive failed :( Bytes read: %d", returnValue);
        return 1;
    }
    log_debug("Receive successful :D Bytes read: %d", returnValue);
    buf[returnValue] = '\0'; // Adds null terminator
    return 0;
}

/*
Description:
    Closes the given socket.
Arguments:
    int sockfd: Socket file descriptor
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close(int sockfd) {
    log_info("Closing socket...");
    int returnValue;
    if ((returnValue = close(sockfd)) != 0) {
        log_debug("Close failed. returnValue: %d", returnValue);
        return 1;
    }
    return 0;
}