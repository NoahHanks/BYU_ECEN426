#include "tcp_client.h"
#include "log.h"
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>

#define ARG_ERROR 1
#define MAX_PORT_NUMBER 65535
#define ARGUMENTS 2
#define LINE_ASCII_LIMIT 1024

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
    Allocates memory depending on the configuration
Arguments:
    Config *config: has the host, port and file.
Return value:
    None.
*/
void tcp_client_allocate_config(Config *config) {
    config->host = malloc(sizeof(char) * (100 + 1));
    config->port = malloc(sizeof(char) * (5 + 1));
    config->file = malloc(sizeof(char) * (100 + 1));
    log_info("Memory allocattion complete");
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
    int opt;

    tcp_client_allocate_config(config);

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

    strcpy(config->port, TCP_CLIENT_DEFAULT_PORT);
    strcpy(config->host, TCP_CLIENT_DEFAULT_HOST);
    // log_trace("enter parse args");
    while (1) {
        opt = getopt_long(argc, argv, "vh:p:", longOptions, &optionIndex);

        if (opt == -1)
            break;

        switch (opt) {
        case 0:
            printInfoMenu();
            break;
        case 'v':
            log_set_level(LOG_TRACE);
            break;
        case 'h':
            log_info("host: %s", optarg);
            strcpy(config->host, optarg);
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
    if (optind < argc) {
        config->file = argv[optind];
        log_info("File: %s", config->file);
    }

    else {
        log_debug("Incorrect number of arguments");
        printInfoMenu();
        return 1;
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
int tcp_client_send_request(int sockfd, char *action, char *message) {

    log_info("Sending data to the server");
    int messageLength, sent;
    char *request = malloc(strlen(action) + 4 + strlen(message));
    char str[5];

    sprintf(str, "%lu", strlen(message));
    log_info("Configuring message...");

    strcpy(request, action);
    strcat(request, " ");
    strcat(request, str);
    strcat(request, " ");
    strcat(request, message);
    messageLength = strlen(request);

    log_info("Sending message...");
    sent = send(sockfd, request, messageLength, 0);
    if (sent == -1) {
        log_error("Error with sending.");
        return -1;
    }
    return 0;
}

/*
Description:
    Receives the response from the server. The caller must provide a function pointer that handles
the response and returns a true value if all responses have been handled, otherwise it returns a
    false value. After the response is handled by the handle_response function pointer, the response
    data can be safely deleted. The string passed to the function pointer must be null terminated.
Arguments:
    int sockfd: Socket file descriptor
    int (*handle_response)(char *): A callback function that handles a response
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, int (*handle_response)(char *)) {
    char *index = 0;
    int lastMessage = 0;
    size_t numBytesInBuffer = 0;
    size_t bufferSize = 1024;
    char *buffer;
    int messageLength = 0;

    char *response = NULL;

    buffer = malloc(sizeof(char) * bufferSize);
    buffer[0] = '\0';

    // Continues to receive messages until the last one is received
    log_info("Beginning to receive messages.");
    while (!lastMessage) {
        // Allocate space for buffer
        if (numBytesInBuffer > bufferSize / 2) {
            bufferSize *= 2;
            buffer = realloc(buffer, bufferSize);
        }
        log_info("Continue receiving data %d", sockfd);

        int numbytes =
            recv(sockfd, buffer + numBytesInBuffer, bufferSize - numBytesInBuffer - 1, 0);
        numBytesInBuffer += numbytes;
        buffer[numBytesInBuffer] = '\0';
        log_debug("Number of bytes in the buffer: %zu", numBytesInBuffer);

        // Scans and processes the buffer and determines if there are more messages
        while ((index = strchr(buffer, ' ')) != NULL) {
            log_debug("Contents of buffer: %s", buffer);
            sscanf(buffer, "%d", &messageLength);
            log_debug("Message length is: %zu", messageLength);
            if (numBytesInBuffer - (index + 1 - buffer)) {
                response = malloc(sizeof(char) * (messageLength + 1));
                strncpy(response, index + 1, messageLength);
                response[messageLength] = '\0';
                lastMessage = handle_response(response);
                free(response);
                numBytesInBuffer -= (index - buffer) + 1 + messageLength;
                log_debug("New number of bytes in buffer: %zu", numBytesInBuffer);
                log_debug("Buffer: %s, New buffer: %s", buffer, index + 1 + messageLength);
                char *tempString = malloc(sizeof(char) * (numBytesInBuffer + 1));
                strcpy(tempString, index + 1 + messageLength);
                strcpy(buffer, tempString);
                free(tempString);
                log_info("Receive successful");
            }
        }
    }
    free(buffer);
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

/*
Description:
    Opens a file.
Arguments:
    char *file_name: The name of the file to open
Return value:
    Returns NULL on failure, a FILE pointer on success
*/
FILE *tcp_client_open_file(char *file_name) {
    log_info("File name: %s", file_name);
    if (strcmp(file_name, "-") == 0) {
        return stdin;
    }
    return fopen(file_name, "r");
}

/*
Description:
    Gets the next line of a file, filling in action and message. This function should be similar
    design to getline() (https://linux.die.net/man/3/getline). *action and message must be allocated
    by the function and freed by the caller.* When this function is called, action must point to the
    action string and the message must point to the message string.
Arguments:
    FILE *fd: The file pointer to read from
    char **action: A pointer to the action that was read in
    char **message: A pointer to the message that was read in
Return value:
    Returns -1 on failure, the number of characters read on success
*/
int tcp_client_get_line(FILE *fd, char **action, char **message) {

    *action = malloc(sizeof(char) * (LINE_ASCII_LIMIT));
    *message = malloc(sizeof(char) * (LINE_ASCII_LIMIT));
    char *stringLine = NULL;
    size_t readIn = LINE_ASCII_LIMIT;
    ssize_t charCount;

    if ((charCount = getline(&stringLine, &readIn, fd)) == -1) {
        log_info("No line was read from file or reached the end of file.");
        return -1;
    }
    stringLine[charCount - 1] = '\0';
    log_trace("String read from the file is: %s", stringLine);
    int read = sscanf(stringLine, "%s %[^\n]", *action, *message);
    return read;
}

/*
Description:
    Closes a file.
Arguments:
    FILE *fd: The file pointer to close
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close_file(FILE *fd) { return fclose(fd); }
