/*
 * Copyright 2024 8dcc
 *
 * This file is part of snc (Simple NetCat).
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>    /* read(), write(), close() */
#include <netdb.h>     /* getaddrinfo(), etc. */
#include <arpa/inet.h> /* htonl(), htons() */
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h> /* getifaddrs(), etc. */
#include <net/if.h>  /* IFF_LOOPBACK */

/*----------------------------------------------------------------------------*/
/* Macros */

/* Comment to disable interface listing on "listen" mode. */
#define LIST_INTERFACES

#define PORT    1337
#define MAX_BUF 256 /* Max size of message/response */

#define LENGTH(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define ERR(...)                                                               \
    do {                                                                       \
        fprintf(stderr, "snc: ");                                              \
        fprintf(stderr, __VA_ARGS__);                                          \
        fputc('\n', stderr);                                                   \
    } while (0)

#define DIE(...)                                                               \
    do {                                                                       \
        ERR(__VA_ARGS__);                                                      \
        exit(1);                                                               \
    } while (0)

/*----------------------------------------------------------------------------*/
/* Internal structures and enums */

enum EMode {
    MODE_ERR,
    MODE_RECEIVE,
    MODE_TRANSMIT,
};

/*----------------------------------------------------------------------------*/
/* Util functions */

/*
 * Get the program mode (`EMode') from the command line arguments.
 */
static enum EMode get_mode(int argc, char** argv) {
    if (argc < 2)
        return MODE_ERR;

    for (int i = 0; argv[1][i] != '\0'; i++) {
        switch (argv[1][i]) {
            /* Receive */
            case 'r':
                return MODE_RECEIVE;

            /* Transmit */
            case 't':
                if (argc < 3) {
                    ERR("Not enough arguments for option \"c\".");
                    return MODE_ERR;
                }

                return MODE_TRANSMIT;

            /* Help */
            case 'h':
                return MODE_ERR;

            /* "snc -h" -> "snc h" */
            case '-':
                break;

            default:
                ERR("Unknown option \"%s\".", argv[1]);
                return MODE_ERR;
        }
    }

    ERR("Not enough arguments.");
    return MODE_ERR;
}

/*
 * List the available interfaces to `stderr'.
 */
static inline void list_interfaces(void) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        ERR("Failed to list interfaces.");
        return;
    }

    fprintf(stderr,
            "---------------------------\n"
            "Listening on any interface:\n");

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        /* Ignore non-INET address families and loopback interfaces */
        if (ifa->ifa_addr->sa_family != AF_INET ||
            (ifa->ifa_flags & IFF_LOOPBACK) != 0)
            continue;

        char host[NI_MAXHOST];
        int code = getnameinfo(ifa->ifa_addr,
                               sizeof(struct sockaddr_in),
                               host,
                               NI_MAXHOST,
                               NULL,
                               0,
                               NI_NUMERICHOST);
        if (code != 0)
            continue;

        fprintf(stderr, "%s: %s\n", ifa->ifa_name, host);
    }

    fprintf(stderr, "---------------------------\n");

    freeifaddrs(ifaddr);
}

/*----------------------------------------------------------------------------*/
/* Main modes */

/*
 * Main function for the "receive" mode.
 *
 * TODO: Improve.
 * TODO: Change argument type to match `snc_transmit'.
 */
static void snc_receive(int port) {
    /*
     * Create the socket descriptor for listening.
     *
     * domain:   AF_INET     (IPv4 address)
     * type:     SOCK_STREAM (TCP, etc.)
     * protocol: 0           (Let the kernel decide, usually TPC)
     */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!listen_fd) {
        ERR("Failed to create socket.");
        exit(1);
    }

#ifdef LIST_INTERFACES
    list_interfaces();
#endif

    /* Declare and clear struct */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    /*
     * Fill the struct we just declared.
     * See: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
     */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* htonl -> uint32_t */
    server_addr.sin_port        = htons(port);       /* htons -> uint16_t */

    /* Bind socket file descriptor to the struct we just filled (but casted) */
    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));

    /* 10 is the max number of connections to queue */
    listen(listen_fd, 10);

    int conn_fd = accept(listen_fd, NULL, NULL);

    int read_code;
    char c = 0;
    while ((read_code = read(conn_fd, &c, 1)) > 0)
        putchar(c);

    if (read_code < 0)
        ERR("Read error: %s", strerror(errno));

    close(conn_fd);
    close(listen_fd);
}

/*
 * Main function for the "transmit" mode.
 *
 * Note that the format of the `ip' and `port' arguments should match any valid
 * inputs for `getaddrinfo'. Note that if the port is not numeric, it should
 * appear in the "/etc/services" file.
 */
static void snc_transmit(const char* ip, const char* port) {
    int status;

    /*
     * Initialize the `addrinfo' structure with the hints for `getaddrinfo'.
     *
     * First, the family: IPv4 (AF_INET).
     * Second, the socket type: TCP (SOCK_STREAM).
     */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /*
     * Obtain the address information from the specified hints.
     */
    struct addrinfo* server_info;
    status = getaddrinfo(ip, port, &hints, &server_info);
    if (status != 0)
        DIE("Could not obtaining address info: %s", gai_strerror(status));

    /*
     * We obtain the socket descriptor using the values from the `addrinfo'
     * structure that `getaddrinfo' filled.
     */
    const int sockfd = socket(server_info->ai_family,
                              server_info->ai_socktype,
                              server_info->ai_protocol);
    if (sockfd < 0)
        DIE("Could not create socket: %s", strerror(errno));

    /*
     * Connect to the actual server. Again, using the values from the `addrinfo'
     * structure.
     */
    status = connect(sockfd, server_info->ai_addr, server_info->ai_addrlen);
    if (status < 0)
        DIE("Connection error: %s", strerror(errno));

    /*
     * Characters from `stdin', and write them to `sockfd'.
     */
    int c;
    while ((c = getchar()) != EOF) {
        const char byte   = (char)c;
        const int written = write(sockfd, &byte, sizeof(byte));
        if (written < 0)
            DIE("Write error: %s", strerror(errno));
    }

    /*
     * Close the socket descriptor, and free the linked list of `addrinfo'
     * structures that `getaddrinfo' allocated.
     */
    close(sockfd);
    freeaddrinfo(server_info);
}

/*----------------------------------------------------------------------------*/

int main(int argc, char** argv) {
    const enum EMode mode = get_mode(argc, argv);
    if (mode == MODE_ERR) {
        fprintf(stderr,
                "Usage:\n"
                "    %s h       - Show this help.\n"
                "    %s r       - Start in \"receive\" mode.\n"
                "    %s t <IP>  - Transmit to specified IP address.\n",
                argv[0],
                argv[0],
                argv[0]);
        return 1;
    }

    switch (mode) {
        case MODE_RECEIVE:
            snc_receive(PORT);
            break;

        case MODE_TRANSMIT:
            /*
             * FIXME: Use `PORT' macro when it becomes a string literal.
             */
            snc_transmit(argv[2], "1337");
            break;

        default:
            ERR("Fatal: Unhandled mode.");
            return 1;
    }

    return 0;
}
