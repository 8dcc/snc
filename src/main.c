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

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifndef NO_SIGNAL_HANDLING
#include <signal.h>
#endif

#include "include/util.h"
#include "include/args.h"
#include "include/receive.h"
#include "include/transmit.h"

/*----------------------------------------------------------------------------*/

/*
 * Globals set depending on command-line arguments.
 */
bool g_opt_print_interfaces = false;
bool g_opt_print_peer_info  = false;
bool g_opt_print_progress   = false;

/*
 * True if the user signaled that he wants to quit.
 */
bool g_signaled_quit = false;

/*----------------------------------------------------------------------------*/

#ifndef NO_SIGNAL_HANDLING
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

/*
 * Setup our "quit" handler for the specified signal, so that it also interrupts
 * system calls like `accept', `recv' and `send'.
 */
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
#endif

/*----------------------------------------------------------------------------*/

int main(int argc, char** argv) {
    struct Args args;
    args_init(&args);
    args_parse(argc, argv, &args);

    g_opt_print_interfaces = args.print_interfaces;
    g_opt_print_peer_info  = args.print_peer_info;
    g_opt_print_progress   = args.print_progress;

#ifndef NO_SIGNAL_HANDLING
    setup_quit_signal_handler(SIGINT);
    setup_quit_signal_handler(SIGQUIT);
#endif

    switch (args.mode) {
        case ARGS_MODE_RECEIVE:
            snc_receive(args.port, stdout);
            break;

        case ARGS_MODE_TRANSMIT:
            snc_transmit(stdin, args.destination, args.port);
            break;

        case ARGS_MODE_NONE:
            abort(); /* Should have been validated in 'args_parse' */
    }

    return 0;
}
