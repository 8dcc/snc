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

#ifndef TRANSMIT_H_
#define TRANSMIT_H_ 1

#include <stdio.h> /* FILE */

/*
 * Main function for the "transmit" mode.
 *
 * Transmits all possible data from the `src_fp' file to the destination port
 * `dst_port' at the `dst_ip'.
 *
 * Note that the format of the `dst_ip' and `dst_port' arguments should match
 * any valid inputs for `getaddrinfo'. Note that if the port is not numeric, it
 * should appear in the "/etc/services" file.
 */
void snc_transmit(FILE* src_fp, const char* dst_ip, const char* dst_port);

#endif /* TRANSMIT_H_ */
