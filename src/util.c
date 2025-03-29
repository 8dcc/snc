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

/*
 * TODO: Even when defining '_POSIX_C_SOURCE' or '_XOPEN_SOURCE', 'IFF_LOOPBACK'
 * and 'NI_MAXHOST' are not recognized in C99, although I don't think this is
 * documented.
 */
#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h> /* fprintf(), fputc(), etc. */

#include <ifaddrs.h> /* getifaddrs(), etc. */
#include <net/if.h>  /* IFF_LOOPBACK */
#include <netdb.h>   /* NI_MAXHOST */
#include <sys/types.h>
#include <sys/socket.h> /* sockaddr */
#include <arpa/inet.h>  /* inet_ntop() */

#include "include/util.h"

void print_indentated(FILE* fp, int indent, const char* str) {
    for (int i = 0; i < indent; i++)
        fputc(' ', fp);

    for (; *str != '\0'; str++) {
        fputc(*str, fp);
        if (*str == '\n')
            for (int i = 0; i < indent; i++)
                fputc(' ', fp);
    }
    fputc('\n', fp);
}

void print_interface_list(FILE* fp) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        ERR("Failed to list interfaces.");
        return;
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        /* Ignore non-INET address families and loopback interfaces */
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET ||
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

        fprintf(fp, "%-6s - %s\n", ifa->ifa_name, host);
    }

    freeifaddrs(ifaddr);
}

void print_sockaddr(FILE* fp, struct sockaddr_storage* info) {
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

void print_progress(const char* verb, size_t progress) {
    /*
     * List of units for printing the `progress'. Each unit should be 1024 bytes
     * appart from each other. You can safely add or remove units to this array
     * if you want more or less precision.
     */
    static const char* unit_names[] = {
        "bytes",
        "KiB",
        "MiB",
        "GiB",
    };

    /*
     * Convert progress (originally in bytes) to the appropriate unit.
     */
    size_t unit_name_idx   = 0;
    double pretty_progress = progress;
    while (pretty_progress >= 1024 && unit_name_idx + 1 < LENGTH(unit_names)) {
        pretty_progress /= 1024.0;
        unit_name_idx++;
    }

    const int printed_len = (unit_name_idx == 0)
                              ? fprintf(stderr,
                                        "\r%s %zu %s.",
                                        verb,
                                        progress,
                                        unit_names[unit_name_idx])
                              : fprintf(stderr,
                                        "\r%s %.2f %s.",
                                        verb,
                                        pretty_progress,
                                        unit_names[unit_name_idx]);
    if (printed_len < 0)
        return;

    /*
     * If the last printed text was longer, remove trailing characters.
     */
    static int last_printed_len = 0;
    for (int i = printed_len; i < last_printed_len; i++)
        fputc(' ', stderr);

    /*
     * Store the number of characters we have written, to make sure that future
     * calls can clear the trailing characters, if necessary.
     */
    last_printed_len = printed_len;
}

void print_partial_progress(const char* verb, size_t progress) {
    /*
     * The `progress' will be printed if it advanced at least `PROGRESS_STEP'
     * times since the last printed value.
     */
    static const double PROGRESS_STEP = 1.25;
    static size_t last_progress       = 0;

    /*
     * If the current progress changed enough, print it. Then, save the current
     * progress in a `last_progress' static variable for future calls.
     */
    if (progress < last_progress * PROGRESS_STEP)
        return;

    print_progress(verb, progress);
    last_progress = progress;
}
