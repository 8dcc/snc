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

#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h> /* close() */
#include <netdb.h>  /* getaddrinfo(), etc. */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), etc. */

#include "include/util.h"
#include "include/receive.h"

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
 * If defined, the transmission progress will be printed with `print_progress',
 * defined in "util.c".
 */
#define SNC_PRINT_PROGRESS

/*
 * Maximum number of connections that the receiver can wait for. See the second
 * parameter of listen(2).
 */
#define SNC_LISTEN_QUEUE_SZ 10

/*
 * Size of the buffer used when receiving and writing data. Independent of the
 * buffer size for reading/sending in "transmit.c".
 */
#define BUF_SZ 1000

/*----------------------------------------------------------------------------*/

void snc_receive(const char* src_port, FILE* dst_fp) {
    int status;

#ifdef SNC_LIST_INTERFACES
    list_interfaces(stderr);
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
    status = getaddrinfo(NULL, src_port, &hints, &self_info);
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
     * conveniently, `getaddrinfo' filled that information inside the
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
    fprintf(stderr, "Incoming connection from: ");
    print_sockaddr(stderr, &peer_addr);
    fprintf(stderr, "\n---------------------------\n");
#endif

#ifdef SNC_PRINT_PROGRESS
    size_t total_received = 0;
#endif

    /*
     * Receive the data from the connection. Note how we use the connection
     * socket descriptor (returned by `accept'), not the socket descriptor used
     * for listening for new connections (returned by `socket').
     */
    char buf[BUF_SZ];
    for (;;) {
        const ssize_t received = recv(sockfd_connection, buf, sizeof(buf), 0);
        if (received < 0)
            DIE("Receive error: %s", strerror(errno));
        if (received == 0)
            break;

#ifdef SNC_PRINT_PROGRESS
        total_received += received;
        print_partial_progress("Received", total_received);
#endif

        fwrite(buf, received, sizeof(char), dst_fp);
    }

#ifdef SNC_PRINT_PROGRESS
    /*
     * After we are done, we want to print the exact progress unconditionally.
     */
    print_progress("Transmitted", total_received);
    fputc('\n', stderr);
#endif

    close(sockfd_connection);
    close(sockfd_listen);
    freeaddrinfo(self_info);
}
