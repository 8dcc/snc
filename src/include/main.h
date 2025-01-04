
#ifndef MAIN_H_
#define MAIN_H_ 1

#include <stdbool.h>

/*
 * Globals for program arguments.
 */
extern bool g_opt_print_interfaces;
extern bool g_opt_print_peer_info;
extern bool g_opt_print_progress;

/*
 * Signal-handling globals.
 */
extern bool g_signaled_quit;

#endif /* MAIN_H_ */
