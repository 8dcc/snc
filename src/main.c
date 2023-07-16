
/**
 * @todo Handle SIGTERM and close sockets
 * @todo Multi-directional transfers (l <-> c)
 * */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>    /* write, close */
#include <arpa/inet.h> /* htonl, htons */
#include <sys/socket.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

enum modes {
    ERR     = 0,
    LISTEN  = 1,
    CONNECT = 2,
};

#define PORT    1337
#define MAX_BUF 256 /* Max size of message/response */

/**
 * @brief Argument parsing
 * @param argc Number of arguments
 * @param argv Vector of string arguments
 * @return Mode or error code
 */
int arg_check(int argc, char** argv) {
    if (argc < 2)
        return ERR;

    if (!strcmp(argv[1], "l")) { /* Listen */
        return LISTEN;
    } else if (!strcmp(argv[1], "c")) { /* Connect */
        if (argc < 3) {
            fprintf(stderr, "Not enough arguments for option \"c\".\n");
            return ERR;
        }

        return CONNECT;
    } else if (!strcmp(argv[1], "h")) { /* Help */
        return ERR;
    } else {
        fprintf(stderr, "Unknown option \"%s\".\n", argv[1]);
        return ERR;
    }
}

/**
 * @brief Main function for the "listen" mode
 * @return Exit code
 */
int snc_listen(void) {
    /*
     * Create the socket descriptor for listening.
     *
     * domain:   AF_INET     (IPv4 address)
     * type:     SOCK_STREAM (TCP, etc.)
     * protocol: 0           (Let the kernel decide, usually TPC)
     */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!listen_fd) {
        fprintf(stderr, "listen: failed to create socket.\n");
        return 1;
    }

    /* Declare and clear struct */
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));

    /* Fill the struct we just declared.
     * See: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* htonl -> uint32_t */
    server_addr.sin_port        = htons(PORT);       /* htons -> uint16_t */

    /* Bind socket file descriptor to the struct we just filled (but casted) */
    bind(listen_fd, (sockaddr*)&server_addr, sizeof(sockaddr));

    /* 10 is the max number of connections to queue */
    listen(listen_fd, 10);

    int conn_fd = accept(listen_fd, NULL, NULL);

    char c = 0;
    while (read(conn_fd, &c, 1) && c != EOF)
        putchar(c);

    close(conn_fd);
    close(listen_fd);

    return 0;
}

int snc_connect(char* ip) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!socket_fd) {
        fprintf(stderr, "connect: failed to create socket.\n");
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);

    if (!inet_pton(AF_INET, ip, &server_addr.sin_addr)) {
        fprintf(stderr, "IP error.\n");
        return 1;
    }

    if (connect(socket_fd, (sockaddr*)&server_addr, sizeof(sockaddr)) < 0) {
        fprintf(stderr, "Connection error.\n");
        return 1;
    }

    char c;
    while ((c = getchar()) != EOF)
        write(socket_fd, &c, 1);

    /* Need to send EOF so it knows when to stop */
    write(socket_fd, &c, 1);
    close(socket_fd);

    return 0;
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
                "    %s h       - Show this help\n"
                "    %s l       - Start in listen mode\n"
                "    %s c <IP>  - Connect to specified IP address\n",
                argv[0], argv[0], argv[0]);
        return 1;
    }

    if (mode == LISTEN) {
        return snc_listen(); /* snc l */
    } else if (mode == CONNECT) {
        return snc_connect(argv[2]); /* snc c IP */
    } else {
        fprintf(stderr, "Unknown mode error.\n");
        return 1;
    }

    return 0;
}
