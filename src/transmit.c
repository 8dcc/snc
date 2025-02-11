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

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h> /* close() */
#include <netdb.h>  /* getaddrinfo(), etc. */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), etc. */

#include "include/util.h"
#include "include/main.h"
#include "include/transmit.h"

/*----------------------------------------------------------------------------*/

/*
 * Size of the buffer used when reading and sending data. Independent of the
 * buffer size for receiving/writing in "receive.c".
 */
#define BUF_SZ 1000

/*----------------------------------------------------------------------------*/

static bool send_data(int sockfd, void* data, size_t data_sz) {
    size_t total_sent = 0;

    while (data_sz > 0) {
        const ssize_t sent =
          send(sockfd, &((char*)data)[total_sent], data_sz, 0);
        if (sent < 0)
            return false;

        total_sent += sent;
        data_sz -= sent;
    }

    return true;
}

void snc_transmit(FILE* src_fp, const char* dst_ip, const char* dst_port) {
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
    status = getaddrinfo(dst_ip, dst_port, &hints, &server_info);
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
     */
    int c;
    char buf[BUF_SZ];
    size_t buf_pos           = 0;
    size_t total_transmitted = 0;
    while ((c = fgetc(src_fp)) != EOF && !g_signaled_quit) {
        buf[buf_pos++] = c;

        /*
         * If the buffer is full, send the data in the buffer, and reset the
         * buffer position.
         */
        if (buf_pos >= BUF_SZ) {
            if (!send_data(sockfd, buf, buf_pos))
                DIE("Send error: %s", strerror(errno));

            if (g_opt_print_progress) {
                total_transmitted += buf_pos;
                print_partial_progress("Transmitted", total_transmitted);
            }

            buf_pos = 0;
        }
    }

    /*
     * Once we reach this poitn, `fgetc' returned EOF. Send the remaining data
     * in the buffer (if any).
     */
    if (!send_data(sockfd, buf, buf_pos))
        DIE("Send error: %s", strerror(errno));

    /*
     * After we are done, we want to print the exact progress. Notice how we
     * call 'print_progress' instead of 'print_partial_progress'.
     */
    if (g_opt_print_progress) {
        total_transmitted += buf_pos;
        print_progress("Transmitted", total_transmitted);
        fputc('\n', stderr);
    }

    /*
     * Close the socket descriptor, and free the linked list of `addrinfo'
     * structures that `getaddrinfo' allocated.
     */
    close(sockfd);
    freeaddrinfo(server_info);
}
