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

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#include "include/util.h"
#include "include/receive.h"
#include "include/transmit.h"

/*----------------------------------------------------------------------------*/

/*
 * Globals used for program arguments. A description can be found inside each
 * entry of the 'g_options' array below.
 */
static bool g_opt_help        = false;
static bool g_opt_receive     = false;
static bool g_opt_transmit    = false;
static char* g_param_transmit = NULL;
static char* g_param_port     = "1337";
bool g_opt_print_interfaces   = false;
bool g_opt_print_peer_info    = false;
bool g_opt_print_progress     = false;

/*
 * True if the user signaled that he wants to quit.
 */
bool g_signaled_quit = false;

/*----------------------------------------------------------------------------*/

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
      "Print the help to 'stdout' and exit.",
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
    {
      &g_opt_print_interfaces,
      NULL,
      "--print-interfaces",
      NULL,
      NULL,
      "When receiving data, print the list of local interfaces, along with \n"
      "their addresses. Useful when receiving data over a LAN.",
    },
    {
      &g_opt_print_peer_info,
      NULL,
      "--print-peer-info",
      NULL,
      NULL,
      "When receiving data, print the peer information whenever a connection\n"
      "is accepted.",
    },
    {
      &g_opt_print_progress,
      NULL,
      "--print-progress",
      NULL,
      NULL,
      "Print the size of the received or transmitted data to 'stderr'.",
    },
};

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
static void print_usage(FILE* fp, const char* self) {
    fprintf(fp,
            "Usage: %s [OPTION]...\n\n"
            "List of options:",
            self);

    /*
     * For each option, print:
     *   1. The short option (if any).
     *   2. The long option with its extra parameter (if any).
     *   3. The description, in its own line.
     */
    for (size_t i = 0; i < LENGTH(g_options); i++) {
        fprintf(fp, "\n  ");

        if (g_options[i].opt_short != NULL)
            fprintf(fp, "%s, ", g_options[i].opt_short);

        fprintf(fp, "%s", g_options[i].opt_long);
        if (g_options[i].param != NULL)
            fprintf(fp, " %s", g_options[i].param);
        fputc('\n', fp);

        print_indentated(fp, 4, g_options[i].description);
    }
}

/*----------------------------------------------------------------------------*/

/*
 * Handler for quit signals (e.g. `SIGINT' or `SIGQUIT').
 *
 * We simply store that the user wants to quit, and then restore the default
 * handler of the signal we received. This is probably not even necessary, since
 * interrupted calls to `recv' and `send' return `EINTR' according to the
 * signal(7) man page.
 */
static void quit_signal_handler(int sig) {
    g_signaled_quit = true;
    signal(sig, SIG_DFL);
}

static void setup_quit_signal_handler(int sig) {
    struct sigaction act;

    if (sigaction(sig, NULL, &act) == -1)
        DIE("Failed to read signal action for '%s': %s",
            strsignal(sig),
            strerror(errno));

    /*
     * Unset the `SA_RESTART' flag, so system calls (like `accept', `recv',
     * etc.) return `EINTR' when the signal is received. See also the
     * sigaction(2) and signal(7) man pages.
     */
    act.sa_flags &= ~SA_RESTART;

    /*
     * Setup the signal handler itself. We also to make sure that the
     * `SA_SIGINFO' flag is not set, since it means that our handler takes 3
     * arguments, which is not the case.
     */
    act.sa_flags &= ~SA_SIGINFO;
    act.sa_handler = quit_signal_handler;

    if (sigaction(sig, &act, NULL) == -1)
        DIE("Failed to set signal action for '%s': %s",
            strsignal(sig),
            strerror(errno));
}

/*----------------------------------------------------------------------------*/

int main(int argc, char** argv) {
    parse_args(argc, argv);
    validate_global_opts();

    setup_quit_signal_handler(SIGINT);
    setup_quit_signal_handler(SIGQUIT);

    if (g_opt_help) {
        print_usage(stdout, argc >= 1 ? argv[0] : "snc");
    } else if (g_opt_receive) {
        snc_receive(g_param_port, stdout);
    } else if (g_opt_transmit) {
        snc_transmit(stdin, g_param_transmit, g_param_port);
    } else {
        DIE("Error: Not enough arguments. Expected a mode option.");
    }

    return 0;
}
