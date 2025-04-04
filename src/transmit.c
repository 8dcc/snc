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
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h> /* close() */
#include <netdb.h>  /* getaddrinfo(), etc. */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), etc. */

#include "include/util.h"
#include "include/main.h"
#include "include/transmit.h"

#define CLEANUP_AND_DIE(...)                                                   \
    do {                                                                       \
        ERR(__VA_ARGS__);                                                      \
        fatal_error = true;                                                    \
        goto cleanup;                                                          \
    } while (0)

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
    struct addrinfo* server_info = NULL;
    int sockfd                   = -1;

#ifdef FIXED_BLOCK_SIZE
    static char buf[FIXED_BLOCK_SIZE];
#else  /* not FIXED_BLOCK_SIZE */
    char* buf           = NULL;
#endif /* not FIXED_BLOCK_SIZE */

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
    status = getaddrinfo(dst_ip, dst_port, &hints, &server_info);
    if (status != 0)
        CLEANUP_AND_DIE("Could not obtaining address info: %s",
                        gai_strerror(status));

    /*
     * We obtain the socket descriptor using the values from the `addrinfo'
     * structure that `getaddrinfo' filled.
     */
    sockfd = socket(server_info->ai_family,
                    server_info->ai_socktype,
                    server_info->ai_protocol);
    if (sockfd < 0)
        CLEANUP_AND_DIE("Could not create socket: %s", strerror(errno));

    /*
     * Connect to the actual server. Again, using the values from the `addrinfo'
     * structure.
     */
    status = connect(sockfd, server_info->ai_addr, server_info->ai_addrlen);
    if (status != 0)
        CLEANUP_AND_DIE("Connection error: %s", strerror(errno));

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
     * Read characters from `stdin', and write them to `sockfd'.
     */
    int c;
    size_t buf_pos           = 0;
    size_t total_transmitted = 0;
    while ((c = fgetc(src_fp)) != EOF && !g_signaled_quit) {
        buf[buf_pos++] = c;

        /*
         * If the buffer is full, send the data in the buffer, and reset the
         * buffer position.
         */
        if (buf_pos >= buf_sz) {
            if (!send_data(sockfd, buf, buf_pos))
                CLEANUP_AND_DIE("Send error: %s", strerror(errno));

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
        CLEANUP_AND_DIE("Send error: %s", strerror(errno));

    /*
     * After we are done, we want to print the exact progress. Notice how we
     * call 'print_progress' instead of 'print_partial_progress'.
     */
    if (g_opt_print_progress) {
        total_transmitted += buf_pos;
        print_progress("Transmitted", total_transmitted);
        fputc('\n', stderr);
    }

cleanup:
#ifndef FIXED_BLOCK_SIZE
    if (buf != NULL)
        free(buf);
#endif /* not FIXED_BLOCK_SIZE */

    /* Opened by 'socket' */
    if (sockfd > -1)
        close(sockfd);

    /* Allocated by 'getaddrinfo' */
    if (server_info != NULL)
        freeaddrinfo(server_info);

    if (fatal_error)
        exit(1);
}
