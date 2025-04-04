
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include <argp.h>

#include "include/args.h"

/*----------------------------------------------------------------------------*/

#ifndef COL_BEFORE
#define COL_BEFORE "\x1B[7m"
#endif /* COL_BEFORE */

#ifndef COL_AFTER
#define COL_AFTER "\x1B[0m"
#endif /* COL_AFTER */

#ifndef VERSION
#define VERSION NULL
#endif /* VERSION */

/*
 * Globals from 'argp.h'.
 */
const char* argp_program_version     = VERSION;
const char* argp_program_bug_address = "<8dcc.git@gmail.com>";

/*
 * IDs for command-line arguments that don't have a short version
 * (e.g. '--print-interfaces', etc.).
 */
enum ELongOptionIds {
    LONGOPT_BLOCK_SIZE = 256,
    LONGOPT_PRINT_INTERFACES,
    LONGOPT_PRINT_PEER_INFO,
    LONGOPT_PRINT_PROGRESS,
};

/*
 * Command-line options. See:
 * https://www.gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html
 */
static struct argp_option options[] = {
    { NULL, 0, NULL, 0, "Mode arguments", 0 },
    {
      "receive",
      'r',
      NULL,
      0,
      "Receive data from incoming transmitters.",
      1,
    },
    {
      "transmit",
      't',
      "DESTINATION",
      0,
      "Transmit data into the DESTINATION receiver.",
      1,
    },
    { NULL, 0, NULL, 0, "Optional arguments", 2 },
    {
      "port",
      'p',
      "PORT",
      0,
      "Specify the port for receiving or transferring data.",
      2,
    },
#ifndef FIXED_BLOCK_SIZE
    {
      "block-size",
      LONGOPT_BLOCK_SIZE,
      "BYTES",
      0,
      "Specify the block size used when receiving or transfering data. Used "
      "for read/write system calls.",
      2,
    },
#endif
    {
      "print-interfaces",
      LONGOPT_PRINT_INTERFACES,
      NULL,
      0,
      "When receiving data, print the list of local interfaces, along with "
      "their addresses. Useful when receiving data over a LAN.",
      3,
    },
    {
      "print-peer-info",
      LONGOPT_PRINT_PEER_INFO,
      NULL,
      0,
      "When receiving data, print the peer information whenever a connection "
      "is accepted.",
      3,
    },
    {
      "print-progress",
      LONGOPT_PRINT_PROGRESS,
      NULL,
      0,
      "Print the size of the received or transmitted data to 'stderr'.",
      3,
    },
    { NULL, 0, NULL, 0, NULL, 0 }
};

/*----------------------------------------------------------------------------*/

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    /*
     * Get the 'input' argument from 'argp_parse', which we know is a pointer to
     * our 'Args' structure.
     */
    struct Args* args = state->input;

    switch (key) {
        case 'r':
            args->mode = ARGS_MODE_RECEIVE;
            break;

        case 't':
            args->mode        = ARGS_MODE_TRANSMIT;
            args->destination = arg;
            break;

        case 'p':
            args->port = arg;
            break;

#ifndef FIXED_BLOCK_SIZE
        case LONGOPT_BLOCK_SIZE:
            if (sscanf(arg, "%zu", &args->block_size) != 1 ||
                args->block_size <= 0) {
                fprintf(state->err_stream,
                        "%s: Invalid block size.\n",
                        state->name);
                argp_usage(state);
            }
            break;
#endif

        case LONGOPT_PRINT_INTERFACES:
            args->print_interfaces = true;
            break;

        case LONGOPT_PRINT_PEER_INFO:
            args->print_peer_info = true;
            break;

        case LONGOPT_PRINT_PROGRESS:
            args->print_progress = true;
            break;

        case ARGP_KEY_END:
            /* Did the user specify one of the "mode" arguments? */
            if (args->mode == ARGS_MODE_NONE) {
                fprintf(state->err_stream,
                        "%s: Expected a mode option.\n",
                        state->name);
                argp_usage(state);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/

void args_init(struct Args* args) {
    args->mode             = ARGS_MODE_NONE;
    args->port             = "1337";
    args->print_interfaces = false;
    args->print_peer_info  = false;
    args->print_progress   = false;

#ifndef FIXED_BLOCK_SIZE
    args->block_size = 0x1000;
#endif
}

void args_parse(int argc, char** argv, struct Args* args) {
    static struct argp argp = {
        options, parse_opt, NULL, NULL, NULL, NULL, NULL,
    };

    argp_parse(&argp, argc, argv, 0, 0, args);

    assert(args->port != NULL);
    assert(args->mode != ARGS_MODE_NONE);
    assert(args->mode != ARGS_MODE_TRANSMIT || args->destination != NULL);

#ifndef FIXED_BLOCK_SIZE
    assert(args->block_size > 0);
#endif
}
