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

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h> /* close() */
#include <netdb.h>  /* getaddrinfo(), etc. */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), etc. */

#include "include/util.h"
#include "include/main.h"
#include "include/receive.h"

#define CLEANUP_AND_DIE(...)                                                   \
    do {                                                                       \
        ERR(__VA_ARGS__);                                                      \
        fatal_error = true;                                                    \
        goto cleanup;                                                          \
    } while (0)

/*----------------------------------------------------------------------------*/

/*
 * Maximum number of connections that the receiver can wait for. See the second
 * parameter of listen(2).
 */
#define SNC_LISTEN_QUEUE_SZ 10

/*----------------------------------------------------------------------------*/

void snc_receive(const char* src_port, FILE* dst_fp) {
    /*
     * The 'status' variable is used to store temporary integer return values.
     *
     * If the 'fatal_error' variable is true that the end of the function, the
     * program will be aborted.
     */
    int status       = 0;
    bool fatal_error = false;

    /*
     * Variables that need to be cleaned up if their value changes.
     */
    struct addrinfo* self_info = NULL;
    int sockfd_listen          = -1;
    int sockfd_connection      = -1;

#ifdef FIXED_BLOCK_SIZE
    static char buf[FIXED_BLOCK_SIZE];
#else  /* not FIXED_BLOCK_SIZE */
    char* buf           = NULL;
#endif /* not FIXED_BLOCK_SIZE */

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
    status = getaddrinfo(NULL, src_port, &hints, &self_info);
    if (status != 0)
        CLEANUP_AND_DIE("Could not obtaining our address info: %s",
                        gai_strerror(status));

    /*
     * We obtain the socket descriptor using the values from the `addrinfo'
     * structure that `getaddrinfo' filled.
     */
    sockfd_listen = socket(self_info->ai_family,
                           self_info->ai_socktype,
                           self_info->ai_protocol);
    if (sockfd_listen < 0)
        CLEANUP_AND_DIE("Could not create socket: %s", strerror(errno));

    /*
     * We `bind' the local port to the socket descriptor. The port (along with
     * the IP address) should be inside a `sockaddr' structure; and,
     * conveniently, `getaddrinfo' filled that information inside the
     * `self_info->ai_addr' member.
     */
    status = bind(sockfd_listen, self_info->ai_addr, self_info->ai_addrlen);
    if (status != 0)
        CLEANUP_AND_DIE("Could not bind port to socket descriptor: %s",
                        strerror(errno));

    /*
     * We listen for incoming connections on the port we just bound to the
     * socket descriptor. The second argument indicates the maximum number of
     * connections that can be queued before being accepted.
     */
    status = listen(sockfd_listen, SNC_LISTEN_QUEUE_SZ);
    if (status != 0)
        CLEANUP_AND_DIE("Could not listen for connections: %s",
                        strerror(errno));

    if (g_opt_print_interfaces) {
        print_separator(stderr);
        fprintf(stderr,
                "Listening on port '%s'. Local interfaces:\n",
                src_port);
        print_interface_list(stderr);
        print_separator(stderr);
    }

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
    sockfd_connection =
      accept(sockfd_listen, (struct sockaddr*)&peer_addr, &peer_addr_sz);
    if (sockfd_connection < 0)
        CLEANUP_AND_DIE("Could not accept incoming connection: %s",
                        strerror(errno));

    if (g_opt_print_peer_info) {
        if (!g_opt_print_interfaces)
            print_separator(stderr);
        fprintf(stderr, "Incoming connection from: ");
        print_sockaddr(stderr, &peer_addr);
        fputc('\n', stderr);
        print_separator(stderr);
    }

#ifdef FIXED_BLOCK_SIZE
    const size_t buf_sz = FIXED_BLOCK_SIZE;
#else  /* not FIXED_BLOCK_SIZE */
    const size_t buf_sz = g_opt_block_size;
    buf                 = malloc(buf_sz);
    if (buf == NULL)
        CLEANUP_AND_DIE("Failed to allocate %zu bytes: %s",
                        buf_sz,
                        strerror(errno));
#endif /* not FIXED_BLOCK_SIZE */

    assert(buf_sz > 0);

    /*
     * Receive the data from the connection. Note how we use the connection
     * socket descriptor (returned by `accept'), not the socket descriptor used
     * for listening for new connections (returned by `socket').
     */
    size_t total_received = 0;
    while (!g_signaled_quit) {
        const ssize_t received = recv(sockfd_connection, buf, buf_sz, 0);
        if (received < 0)
            CLEANUP_AND_DIE("Receive error: %s", strerror(errno));
        if (received == 0)
            break;

        if (g_opt_print_progress) {
            total_received += received;
            print_partial_progress("Received", total_received);
        }

        fwrite(buf, received, sizeof(char), dst_fp);
    }

    /*
     * After we are done, we want to print the exact progress. Notice how we
     * call 'print_progress' instead of 'print_partial_progress'.
     */
    if (g_opt_print_progress) {
        print_progress("Received", total_received);
        fputc('\n', stderr);
    }

cleanup:
#ifndef FIXED_BLOCK_SIZE
    if (buf != NULL)
        free(buf);
#endif /* not FIXED_BLOCK_SIZE */

    /* Opened by 'accept' */
    if (sockfd_connection > -1)
        close(sockfd_connection);

    /* Opened by 'socket' */
    if (sockfd_listen > -1)
        close(sockfd_listen);

    /* Allocated by 'getaddrinfo' */
    if (self_info != NULL)
        freeaddrinfo(self_info);

    if (fatal_error)
        exit(1);
}
