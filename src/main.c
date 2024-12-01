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
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "include/util.h"
#include "include/receive.h"
#include "include/transmit.h"

/*----------------------------------------------------------------------------*/

/* Globals used for program arguments */
bool g_opt_help        = false;
bool g_opt_receive     = false;
bool g_opt_transmit    = false;
char* g_param_transmit = NULL;
char* g_param_port     = "1337";

struct ProgramOption {
    /*
     * If an element of `argv' matches the `opt_short' (e.g. "-r") or `opt_long'
     * members (e.g. "--transmit"), then the `opt_ptr' member is set to `true'
     * (as long as it's not NULL).
     *
     * The `opt_short' and `opt_ptr' members can be NULL, but the `opt_long'
     * member is mandatory.
     */
    bool* opt_ptr;
    const char* opt_short;
    const char* opt_long;

    /*
     * If the current option is found in `argv', and the `param' member is not
     * NULL, the `param_ptr' member will be set to the next element in `argv'.
     *
     * The `param' string will only be used when printing the program usage.
     */
    const char* param;
    char** param_ptr;

    /*
     * Description for the current option, used when printing the program usage.
     */
    const char* description;
};

static struct ProgramOption g_options[] = {
    {
      &g_opt_help,
      "-h",
      "--help",
      NULL,
      NULL,
      "Show the help.",
    },
    {
      &g_opt_receive,
      "-r",
      "--receive",
      NULL,
      NULL,
      "Receive data from incoming transmitters.",
    },
    {
      &g_opt_transmit,
      "-t",
      "--transmit",
      "TARGET",
      &g_param_transmit,
      "Transmit data into the TARGET receiver.",
    },
    {
      NULL,
      "-p",
      "--port",
      "PORT",
      &g_param_port,
      "Specify the port for receiving or transferring data.",
    },
};

/*----------------------------------------------------------------------------*/

/*
 * Return a pointer to the specified `option' inside `g_options'. Returns NULL
 * if not found.
 */
static inline struct ProgramOption* get_option(const char* option) {
    for (size_t i = 0; i < LENGTH(g_options); i++)
        if ((g_options[i].opt_short != NULL &&
             !strcmp(option, g_options[i].opt_short)) ||
            !strcmp(option, g_options[i].opt_long))
            return &g_options[i];

    return NULL;
}

/*
 * Parse the command-line arguments, setting the appropriate globals according
 * to the `g_options' array.
 */
static void parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        const struct ProgramOption* cur_option = get_option(argv[i]);
        if (cur_option == NULL)
            DIE("Error: Unknown option '%s'.", argv[i]);

        if (cur_option->opt_ptr != NULL)
            *(cur_option->opt_ptr) = true;

        /*
         * If this option expects an additional parameter, make sure that
         * this is not the last element of `argv', and store it.
         */
        if (cur_option->param != NULL) {
            i++;
            if (i >= argc)
                DIE("Error: Option '%s' expects a '%s' parameter.",
                    argv[i - 1],
                    cur_option->param);

            assert(cur_option->param_ptr != NULL);
            *(cur_option->param_ptr) = argv[i];
        }
    }
}

/*
 * Validate that there current option combination is valid.
 */
static inline void validate_global_opts(void) {
    if (g_opt_receive && g_opt_transmit)
        DIE("Error: Can't receive and transmit at the same time.");
}

/*
 * List the available command-line arguments.
 */
static void show_usage(const char* self) {
    printf("Usage: %s [OPTION]...\n\n"
           "List of options:",
           self);

    /*
     * For each option, print:
     *   1. The short option (if any).
     *   2. The long option with its extra parameter (if any).
     *   3. The description, in its own line.
     */
    for (size_t i = 0; i < LENGTH(g_options); i++) {
        printf("\n  ");

        if (g_options[i].opt_short != NULL)
            printf("%s, ", g_options[i].opt_short);

        printf("%s", g_options[i].opt_long);
        if (g_options[i].param != NULL)
            printf(" %s", g_options[i].param);

        printf("\n    %s\n", g_options[i].description);
    }
}

/*----------------------------------------------------------------------------*/

/*
 * TODO: Handle `SIGINT', send remaining data and close connections.
 */

int main(int argc, char** argv) {
    parse_args(argc, argv);
    validate_global_opts();

    if (g_opt_help) {
        show_usage(argc >= 1 ? argv[0] : "snc");
    } else if (g_opt_receive) {
        snc_receive(g_param_port, stdout);
    } else if (g_opt_transmit) {
        snc_transmit(stdin, g_param_transmit, g_param_port);
    } else {
        DIE("Error: Not enough arguments. Expected a mode option.");
    }

    return 0;
}
