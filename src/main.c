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
#include <stdio.h>
#include <string.h>

#include "include/util.h"
#include "include/receive.h"
#include "include/transmit.h"

/*----------------------------------------------------------------------------*/

/*
 * Port used to communicate between a transmitter and a receiver.
 *
 * TODO: Allow the user to overwrite it through argv?
 */
#define SNC_PORT "1337"

/*----------------------------------------------------------------------------*/

/*
 * Main modes for the program.
 */
enum EMode {
    MODE_ERR_UNK,
    MODE_ERR_ARGC,
    MODE_HELP,
    MODE_RECEIVE,
    MODE_TRANSMIT,
};

struct {
    enum EMode mode;
    const char* arg;
    const char* extra_args;
    const char* desc;
} g_mode_args[] = {
    { MODE_HELP, "h", NULL, "Show the help." },
    { MODE_RECEIVE, "r", NULL, "Receive data from incoming transmitters." },
    { MODE_TRANSMIT, "t", "TARGET", "Transmit data into the TARGET receiver." },
};

/*----------------------------------------------------------------------------*/

/*
 * Get the program mode from the command line arguments.
 */
static enum EMode get_mode(int argc, char** argv) {
    if (argc < 2)
        return MODE_ERR_ARGC;

    const char* mode_argument = argv[1];
    for (size_t i = 0; i < LENGTH(g_mode_args); i++)
        if (!strcmp(mode_argument, g_mode_args[i].arg))
            return g_mode_args[i].mode;

    return MODE_ERR_UNK;
}

/*
 * List the avaliable command-line arguments.
 */
static void show_help(const char* self) {
    fprintf(stderr, "Usage:\n\n");
    for (size_t i = 0; i < LENGTH(g_mode_args); i++) {
        fprintf(stderr, "\t%s %s", self, g_mode_args[i].arg);
        if (g_mode_args[i].extra_args != NULL)
            fprintf(stderr, " %s", g_mode_args[i].extra_args);
        fprintf(stderr, "\n\t\t%s\n\n", g_mode_args[i].desc);
    }
}

/*----------------------------------------------------------------------------*/

/*
 * TODO: Handle `SIGINT', send remaining data and close connections.
 */

int main(int argc, char** argv) {
    const enum EMode mode = get_mode(argc, argv);

    switch (mode) {
        case MODE_RECEIVE:
            snc_receive(SNC_PORT, stdout);
            break;

        case MODE_TRANSMIT:
            if (argc < 3)
                DIE("Not enough arguments for the specified mode.");

            snc_transmit(stdin, argv[2], SNC_PORT);
            break;

        case MODE_HELP:
            show_help(argv[0]);
            return 1;

        case MODE_ERR_ARGC:
            DIE("Not enough arguments.");

        case MODE_ERR_UNK:
            DIE("Unknown mode argument.");
    }

    return 0;
}
