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

/*
 * If defined, interfaces will be listed on "receive" mode.
 */
#define SNC_LIST_INTERFACES

/*
 * If defined, the peer info will be displayed when a connection is accepted on
 * "receive" mode.
 */
#define SNC_PRINT_PEER_INFO

/*
 * Maximum number of connections that the receiver can wait for. See the second
 * parameter of listen(2).
 */
#define SNC_LISTEN_QUEUE_SZ 10

/*
 * Port used to communicate between a transmitter and a receiver.
 *
 * TODO: Allow the user to overwrite it through argv?
 */
#define SNC_PORT "1337"

/*----------------------------------------------------------------------------*/

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

/*
 * Main modes for the program.
 */
enum EMode {
    MODE_ERR,
    MODE_RECEIVE,
    MODE_TRANSMIT,
};

/*----------------------------------------------------------------------------*/

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
static void list_interfaces(void) {
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

static void print_sockaddr(FILE* fp, struct sockaddr_storage* info) {
    void* addr;
    in_port_t port;

    switch (info->ss_family) {
        case AF_INET: {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)info;

            addr = &(ipv4->sin_addr);
            port = ipv4->sin_port;
        } break;

        case AF_INET6: {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)info;

            addr = &(ipv6->sin6_addr);
            port = ipv6->sin6_port;
        } break;

        default:
            ERR("Unknown address family.");
            return;
    }

    char dst[INET6_ADDRSTRLEN];
    inet_ntop(info->ss_family, addr, dst, sizeof(dst));
    fprintf(fp, "%s, %d", dst, port);
}

/*----------------------------------------------------------------------------*/
/* Main modes */

/*
 * Main function for the "receive" mode.
 */
static void snc_receive(const char* port) {
    int status;

#ifdef SNC_LIST_INTERFACES
    list_interfaces();
#endif

    /*
     * Initialize the `addrinfo' structure with the hints for `getaddrinfo'.
     *
     *   1. The family: IPv4 (AF_INET).
     *   1. The socket type: TCP (SOCK_STREAM).
     *   3. Set the `AI_PASSIVE' flag to indicate that we want to deal with
     *      our own IP address.
     */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    /*
     * Obtain the address information from the specified hints.
     *
     * Note how we use `NULL', along with the `AI_PASSIVE' flag in `hints', to
     * indicate that we want to obtain information about or own IP address.
     */
    struct addrinfo* self_info;
    status = getaddrinfo(NULL, port, &hints, &self_info);
    if (status != 0)
        DIE("Could not obtaining our address info: %s", gai_strerror(status));

    /*
     * We obtain the socket descriptor using the values from the `addrinfo'
     * structure that `getaddrinfo' filled.
     */
    const int sockfd_listen = socket(self_info->ai_family,
                                     self_info->ai_socktype,
                                     self_info->ai_protocol);
    if (sockfd_listen < 0)
        DIE("Could not create socket: %s", strerror(errno));

    /*
     * We `bind' the local port to the socket descriptor. The port (along with
     * the IP address) should be inside a `sockaddr' structure; and,
     * conviniently, `getaddrinfo' filled that information inside the
     * `self_info->ai_addr' member.
     */
    status = bind(sockfd_listen, self_info->ai_addr, self_info->ai_addrlen);
    if (status != 0)
        DIE("Could not bind port to socket descriptor: %s", strerror(errno));

    /*
     * We listen for incoming connections on the port we just bound to the
     * socket descriptor. The second argument indicates the maximum number of
     * connections that can be queued before being accepted.
     */
    status = listen(sockfd_listen, SNC_LISTEN_QUEUE_SZ);
    if (status != 0)
        DIE("Could not listen for connections: %s", strerror(errno));

    /*
     * We accept the incoming connection, and we get a new socket descriptor. It
     * will be used to read (and optionally write) data.
     *
     * The function will fill the second and third arguments with the address
     * information of the peer. We use `sockaddr_storage' since it's guaranteed
     * to be "at least as large as any other `sockaddr_*' address structure".
     * See also sockaddr(3type).
     *
     * The call to `accept' normally blocks the program until a connection is
     * received.
     */
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_sz = sizeof(peer_addr);
    const int sockfd_connection =
      accept(sockfd_listen, (struct sockaddr*)&peer_addr, &peer_addr_sz);
    if (sockfd_connection < 0)
        DIE("Could not accept incoming connection: %s", strerror(errno));

#ifdef SNC_PRINT_PEER_INFO
    fprintf(stderr, "Incomming connection from: ");
    print_sockaddr(stderr, &peer_addr);
    fprintf(stderr, "\n---------------------------\n");
#endif

    /*
     * Receive the data from the connection, one byte at a time. Note how we use
     * the connection socket descriptor (returned by `accept'), not the socket
     * descriptor used for listening for new connections (returned by `socket').
     */
    for (;;) {
        char c;
        const ssize_t num_read = recv(sockfd_connection, &c, sizeof(c), 0);
        if (num_read < 0)
            DIE("Read error: %s", strerror(errno));
        if (num_read == 0)
            break;

        putchar(c);
    }

    close(sockfd_connection);
    close(sockfd_listen);
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
     *   1. The family: IPv4 (AF_INET).
     *   2. The socket type: TCP (SOCK_STREAM).
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
    if (status != 0)
        DIE("Connection error: %s", strerror(errno));

    /*
     * Characters from `stdin', and write them to `sockfd'.
     *
     * TODO: Use `send', instead of `write'.
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
            snc_receive(SNC_PORT);
            break;

        case MODE_TRANSMIT:
            snc_transmit(argv[2], SNC_PORT);
            break;

        default:
            ERR("Fatal: Unhandled mode.");
            return 1;
    }

    return 0;
}
