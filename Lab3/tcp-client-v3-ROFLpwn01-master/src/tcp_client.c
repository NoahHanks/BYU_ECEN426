#include "tcp_client.h"
#include "log.h"
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>

#define ARG_ERROR 1
#define MAX_PORT_NUMBER 65535
#define ARGUMENTS 2
#define LINE_ASCII_LIMIT 1024
#define BUFFER_SIZE 500
#define ACTION_LENGTH_BYTES 4

#define UPPERCASE 0x01
#define LOWERCASE 0X02
#define REVERSE 0x04
#define SHUFFLE 0x08
#define RANDOM 0x10

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

    int opt;
    bool portSet = 0;
    bool hostSet = 0;
    log_info("Parsing arguments");
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {{"help", no_argument, 0, '1'},
                                               {"port", required_argument, 0, 'p'},
                                               {"host", required_argument, 0, 'h'},
                                               {"verbose", no_argument, 0, 'v'},
                                               {0, 0, 0, 0}};

        opt = getopt_long(argc, argv, ":vp:h:", long_options, &option_index);
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
            config->host = optarg;
            hostSet = 1;
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
            portSet = 1;
            break;

        case ':': // Missing option argument
            log_error("Missing option argument\n\n");
            printInfoMenu();
            return 1;
            break;
        case '?': // Invalid option
            log_error("Invalid option\n\n");
            printInfoMenu();

            return 1;
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

    // Sets default configuration
    if (portSet == 0)
        config->port = TCP_CLIENT_DEFAULT_PORT;
    if (hostSet == 0)
        config->host = TCP_CLIENT_DEFAULT_HOST;

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
        return 1;
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
    freeaddrinfo(res);

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

    uint32_t binaryMessage;

    if (strcmp("uppercase", action) == 0) {
        binaryMessage = UPPERCASE;
    } else if (strcmp("lowercase", action) == 0) {
        binaryMessage = LOWERCASE;
    } else if (strcmp("reverse", action) == 0) {
        binaryMessage = REVERSE;
    } else if (strcmp("shuffle", action) == 0) {
        binaryMessage = SHUFFLE;
    } else if (strcmp("random", action) == 0) {
        binaryMessage = RANDOM;
    } else {
        log_error("Invalid action received.\n");
    }
    binaryMessage = binaryMessage << 27;
    binaryMessage = binaryMessage | ((uint32_t)strlen(message));

    uint32_t network_binaryMessage = htonl(binaryMessage);

    if (send(sockfd, &network_binaryMessage, 4, 0) < 4) {
        log_error("Failed to send header");
        return 1;
    }
    if (send(sockfd, message, strlen(message), 0) < (ssize_t)strlen(message)) {
        log_error("Error with sending.");
        return 1;
    }

    return 0;
}

/*
Description:
    Returns the total message length from the server if successfully received.
Arguments:
    char *message: Message received from the server.
    int totalBytes: Total bytes in message.
Return value:
    Returns the message length if found or 0 if no message was received.
*/
int getMessageLength(char *message, uint32_t totalBytes) {
    if (totalBytes >= 4) {
        log_info("Checking message length...");
        uint32_t messageLength = 0;
        messageLength = (int)(message[0] << 24 | message[1] << 16 | message[2] << 8 | message[3]);
        if (totalBytes >= (ACTION_LENGTH_BYTES + messageLength)) {
            return messageLength + ACTION_LENGTH_BYTES;
        }
        return 0;
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

    int totalBytesRecevied = 0;
    int bytesReceived = 0;
    char *mainBuf;
    mainBuf = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *tempBuf;
    tempBuf = (char *)malloc(BUFFER_SIZE * sizeof(char));
    int movePointer;

    log_info("Trying to receive message");
    while (true) {
        if ((bytesReceived = recv(sockfd, tempBuf, BUFFER_SIZE - totalBytesRecevied, 0)) == -1) {
            log_error("Error receiving data");
            return 1;
        }
        memcpy(mainBuf + totalBytesRecevied, tempBuf, bytesReceived);
        totalBytesRecevied += bytesReceived;
        mainBuf[totalBytesRecevied] = '\0';
        // Updates the main buffer
        if (strlen(mainBuf) > (size_t)(BUFFER_SIZE / 2)) {
            mainBuf = (char *)realloc(mainBuf, BUFFER_SIZE * 5 * sizeof(char));
        }
        log_info("Checking for message");
        while ((movePointer = getMessageLength(mainBuf, totalBytesRecevied)) > 0) {
            log_info("Receiving message");
            char *handleString;
            handleString = (char *)malloc((movePointer - 3) * sizeof(char));
            int headerLength = ACTION_LENGTH_BYTES;
            int messageLength = movePointer - headerLength;
            memcpy(handleString, mainBuf + headerLength, messageLength);
            handleString[messageLength] = '\0';

            log_info("Freeing memory from buffers");
            if (handle_response(handleString) == true) {
                free(handleString);
                free(mainBuf);
                free(tempBuf);
                return 0;
            }

            // Moves buffer in memory
            memmove(mainBuf, mainBuf + movePointer, totalBytesRecevied - movePointer);
            totalBytesRecevied -= movePointer;
            free(handleString);
        }
    }
    log_error("No message received");
    return 1;
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
        free(*action);
        free(*message);
        free(stringLine);

        return -1;
    }
    stringLine[charCount - 1] = '\0';
    log_trace("String read from the file is: %s", stringLine);
    int read = sscanf(stringLine, "%s %[^\n]", *action, *message);
    free(stringLine);

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
