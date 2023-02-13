#include "log.h"
#include "tcp_client.h"

int main(int argc, char *argv[]) {

    log_set_level(LOG_WARN);
    Config defaultValues = {TCP_CLIENT_DEFAULT_PORT, TCP_CLIENT_DEFAULT_HOST, "", ""};

    if (tcp_client_parse_arguments(argc, argv, &defaultValues))
        exit(EXIT_FAILURE);

    int sockfd = tcp_client_connect(defaultValues);
    char buf[TCP_CLIENT_MAX_INPUT_SIZE];

    if (sockfd == -1)
        exit(EXIT_FAILURE);

    if (tcp_client_send_request(sockfd, defaultValues))
        exit(EXIT_FAILURE);

    if (tcp_client_receive_response(sockfd, buf, TCP_CLIENT_MAX_INPUT_SIZE))
        exit(EXIT_FAILURE);

    printf("%s\n", buf);

    if (tcp_client_close(sockfd))
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}
