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
#include <stdio.h> /* fprintf(), fputc(), etc. */

#include <ifaddrs.h> /* getifaddrs(), etc. */
#include <net/if.h>  /* IFF_LOOPBACK */
#include <netdb.h>   /* NI_MAXHOST */
#include <sys/types.h>
#include <sys/socket.h> /* sockaddr */
#include <arpa/inet.h>  /* inet_ntop() */

#include "include/util.h"

void list_interfaces(FILE* fp) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        ERR("Failed to list interfaces.");
        return;
    }

    fprintf(fp,
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

        fprintf(fp, "%s: %s\n", ifa->ifa_name, host);
    }

    fprintf(fp, "---------------------------\n");

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
