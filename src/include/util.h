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

#ifndef UTIL_H_
#define UTIL_H_ 1

#include <stdio.h>  /* fprintf(), fputc(), etc. */
#include <stdlib.h> /* exit() */

#include <sys/types.h>
#include <sys/socket.h> /* sockaddr */

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
 * List the available interfaces to the specified `FILE'.
 */
void list_interfaces(FILE* fp);

/*
 * Print a generic `sockaddr' structure to the specified `FILE'. Prints both
 * IPv4 and IPv6 addresses.
 */
void print_sockaddr(FILE* fp, struct sockaddr_storage* info);

/*
 * Print the progress of a generic operation. The `verb' argument indicates the
 * action name (used for printing), and the `progress' argument indicates the
 * current progress of the operation in bytes.
 *
 * This function clears the current line, and overwrites the trailing characters
 * from the previous call.
 *
 * See the function definition for more details.
 */
void print_progress(const char* verb, size_t progress);

/*
 * Keep track of the `progress' history, and call `print_progress' when there is
 * a big enough difference with the previously printed `progress'.
 *
 * This "big enough difference" is defined inside the function itself.
 */
void print_partial_progress(const char* verb, size_t progress);

#endif /* UTIL_H_ */
