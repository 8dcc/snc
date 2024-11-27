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

#ifndef RECEIVE_H_
#define RECEIVE_H_ 1

/*
 * Main function for the "receive" mode.
 *
 * Listens and receives all possible data from the local `src_port' to the
 * destination file `dst_fp'.
 *
 * Just like in `snc_transmit', the format of the `src_port' argument should
 * match any valid input for `getaddrinfo'. If it's not numeric, it should
 * appear in the "/etc/services" file.
 */
void snc_receive(const char* src_port, FILE* dst_fp);

#endif /* RECEIVE_H_ */
