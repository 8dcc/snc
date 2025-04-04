
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
