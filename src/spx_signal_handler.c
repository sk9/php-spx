/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "spx_signal_handler.h"

#if defined(USE_SIGNAL)

#include "spx_error.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

struct spx_signal_handler_t {
    int handler_set;

    struct {
        struct sigaction sigint;
        struct sigaction sigterm;
    } prev_handler;

    volatile sig_atomic_t handler_called;
    volatile sig_atomic_t probing;
    volatile sig_atomic_t stop;
    int signo;

    spx_signal_terminate_fn terminate_callback;
};

/* Global handler for signal callback (signal handlers can't take user data) */
static spx_signal_handler_t *global_handler = NULL;

/* Forward declaration */
static void signal_callback(int signo);

spx_signal_handler_t *spx_signal_handler_create(void)
{
    spx_signal_handler_t *handler = malloc(sizeof(*handler));
    if (!handler) {
        spx_error_set(SPX_ERR_OUT_OF_MEMORY, "Failed to allocate signal handler");
        return NULL;
    }

    handler->handler_set = 0;
    handler->handler_called = 0;
    handler->probing = 0;
    handler->stop = 0;
    handler->signo = -1;
    handler->terminate_callback = NULL;

    return handler;
}

void spx_signal_handler_destroy(spx_signal_handler_t *handler)
{
    if (!handler) {
        return;
    }

    /* Uninstall if still installed */
    if (handler->handler_set) {
        spx_signal_handler_uninstall(handler);
    }

    if (global_handler == handler) {
        global_handler = NULL;
    }

    free(handler);
}

int spx_signal_handler_install(spx_signal_handler_t *handler)
{
    if (!handler) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Handler cannot be NULL");
        return -1;
    }

    if (handler->handler_set) {
        return 0; /* Already installed */
    }

    /* Set global handler for signal callback */
    global_handler = handler;

    struct sigaction act;
    act.sa_handler = signal_callback;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGINT, &act, &handler->prev_handler.sigint) != 0) {
        spx_error_set(SPX_ERR_INTERNAL, "Failed to install SIGINT handler");
        return -1;
    }

    if (sigaction(SIGTERM, &act, &handler->prev_handler.sigterm) != 0) {
        /* Restore SIGINT on failure */
        sigaction(SIGINT, &handler->prev_handler.sigint, NULL);
        spx_error_set(SPX_ERR_INTERNAL, "Failed to install SIGTERM handler");
        return -1;
    }

    handler->handler_set = 1;
    return 0;
}

void spx_signal_handler_uninstall(spx_signal_handler_t *handler)
{
    if (!handler || !handler->handler_set) {
        return;
    }

    sigaction(SIGINT, &handler->prev_handler.sigint, NULL);
    sigaction(SIGTERM, &handler->prev_handler.sigterm, NULL);

    handler->handler_set = 0;

    if (global_handler == handler) {
        global_handler = NULL;
    }
}

int spx_signal_handler_should_stop(const spx_signal_handler_t *handler)
{
    return handler ? handler->stop : 0;
}

void spx_signal_handler_set_probing(spx_signal_handler_t *handler, int probing)
{
    if (handler) {
        handler->probing = probing ? 1 : 0;
    }
}

int spx_signal_handler_get_signo(const spx_signal_handler_t *handler)
{
    return handler ? handler->signo : -1;
}

void spx_signal_handler_set_terminate_callback(spx_signal_handler_t *handler,
                                               spx_signal_terminate_fn callback)
{
    if (handler) {
        handler->terminate_callback = callback;
    }
}

/* Signal callback function */
static void signal_callback(int signo)
{
    if (!global_handler) {
        return;
    }

    global_handler->handler_called++;
    if (global_handler->handler_called > 1) {
        /* Already handling a signal */
        return;
    }

    global_handler->signo = signo;

    if (global_handler->probing) {
        /* Defer termination until probing completes */
        global_handler->stop = 1;
        return;
    }

    /* Safe to terminate now */
    if (global_handler->terminate_callback) {
        global_handler->terminate_callback();
    } else {
        /* Default: exit with appropriate code */
        _exit(signo < 0 ? EXIT_SUCCESS : 128 + signo);
    }
}

#endif /* defined(USE_SIGNAL) */
