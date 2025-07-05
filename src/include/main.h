/*
 * Copyright 2025 8dcc
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

#ifndef MAIN_H_
#define MAIN_H_ 1

#include <stdbool.h>
#include <stddef.h>

/*
 * Globals for program arguments.
 */
extern bool g_opt_print_interfaces;
extern bool g_opt_print_peer_info;
extern bool g_opt_print_progress;

#ifndef FIXED_BLOCK_SIZE
extern size_t g_opt_block_size;
#endif /* FIXED_BLOCK_SIZE */

/*
 * Signal-handling globals.
 */
extern bool g_signaled_quit;

#endif /* MAIN_H_ */
