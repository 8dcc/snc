
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>    /* sleep, write, close */
#include <arpa/inet.h> /* htonl, htons */
#include <sys/socket.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

enum modes {
    ERR     = 0,
    LISTEN  = 1,
    CONNECT = 2,
};

#define MSG "Ping"

int arg_check(int argc, char** argv) {
    if (argc < 2)
        return ERR;

    if (!strcmp(argv[1], "l")) {
        return LISTEN;
    } else if (!strcmp(argv[1], "c")) {
        if (argc < 3) {
            fprintf(stderr, "Not enough arguments for option \"c\".\n");
            return ERR;
        }

        return CONNECT;
    } else {
        fprintf(stderr, "Unknown option \"%s\".\n", argv[1]);
        return ERR;
    }
}

/**
 * @brief Entry point of the program
 * @param argc Number of arguments
 * @param argv Vector of string arguments
 * @return Exit code
 */
int main(int argc, char** argv) {
    int mode = arg_check(argc, argv);
    if (mode == ERR) {
        fprintf(stderr,
                "Usage:\n"
                "    %s l       - Start in listen mode\n"
                "    %s c <IP>  - Connect to specified IP address\n",
                argv[0], argv[0]);
        return 1;
    }

    /* FIXME */
    if (mode == CONNECT) {
        fprintf(stderr, "WIP.\n");
        return 1;
    }

    /*
     * Create the socket descriptor for listening.
     *
     * domain:   AF_INET     (IPv4 address)
     * type:     SOCK_STREAM (TCP, etc.)
     * protocol: 0           (Let the kernel decide, usually TPC)
     */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Declare and clear struct */
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));

    /* Fill the struct we just declared.
     * See: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* htonl -> uint32_t */
    server_addr.sin_port        = htons(1337);       /* htons -> uint16_t */

    /* Bind socket file descriptor to the struct we just filled (but casted) */
    bind(listen_fd, (sockaddr*)&server_addr, sizeof(sockaddr));

    /* 10 is the max number of connections to queue */
    listen(listen_fd, 10);

    int conn_fd = 0;
    for (;;) {
        conn_fd = accept(listen_fd, (sockaddr*)NULL, NULL);

        write(conn_fd, MSG, strlen(MSG));

        close(conn_fd);
        sleep(1);
    }

    return 0;
}
