/*
 * Copyright 2025 8dcc
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

#ifndef ARGS_H_
#define ARGS_H_ 1

#include <stdbool.h>
#include <stddef.h>

/*
 * Available program modes, used in 'Args.mode'.
 */
enum EArgsMode {
    ARGS_MODE_NONE,
    ARGS_MODE_RECEIVE,
    ARGS_MODE_TRANSMIT,
};

/*
 * Structure filled by 'args_parse' to indicate the program's command-line
 * arguments.
 */
struct Args {
    /* Mandatory arguments */
    enum EArgsMode mode;

    /* Optional arguments */
    const char* port;
    bool print_interfaces, print_peer_info, print_progress;

#ifndef FIXED_BLOCK_SIZE
    size_t block_size;
#endif

    /* Only set if 'mode' is 'ARGS_MODE_TRANSMIT' */
    const char* destination;
};

/*----------------------------------------------------------------------------*/

/*
 * Initialize an 'Args' structure.
 */
void args_init(struct Args* args);

/*
 * Parse the arguments in the 'argv' array, of length 'argc', storing the
 * results in the 'Args' structure pointed to by 'args'.
 */
void args_parse(int argc, char** argv, struct Args* args);

#endif /* ARGS_H_ */
