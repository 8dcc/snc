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
#include "include/transmit.h"

/*
 * If defined, the transmission progress will be printed with `print_progress',
 * defined in "util.c".
 */
#define SNC_PRINT_PROGRESS

void snc_transmit(const char* ip, const char* port) {
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
     * Read characters from `stdin', and write them to `sockfd'.
     *
     * TODO: We could probably improve this by buffering characters from
     * `stdin', and sending the whole buffer to `send', rather than sending just
     * one character at a time.
     */
    int c;
    size_t total_transmitted = 0;
    while ((c = getchar()) != EOF) {
        const char byte       = (char)c;
        const ssize_t written = send(sockfd, &byte, sizeof(byte), 0);
        if (written < 0)
            DIE("Write error: %s", strerror(errno));
        if (written == 0)
            ERR("Warning: No bytes were sent. Ignoring...");

#ifdef SNC_PRINT_PROGRESS
        print_progress("Transmitted", total_transmitted);
#endif

        total_transmitted += written;
    }

#ifdef SNC_PRINT_PROGRESS
    fputc('\n', stderr);
#endif

    /*
     * Close the socket descriptor, and free the linked list of `addrinfo'
     * structures that `getaddrinfo' allocated.
     */
    close(sockfd);
    freeaddrinfo(server_info);
}
