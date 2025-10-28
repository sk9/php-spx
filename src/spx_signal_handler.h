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

#ifndef SPX_SIGNAL_HANDLER_H
#define SPX_SIGNAL_HANDLER_H

#if defined(USE_SIGNAL)

/**
 * spx_signal_handler - Signal handling for CLI profiling
 *
 * This module encapsulates signal handling logic for graceful shutdown
 * during CLI profiling. It handles SIGINT and SIGTERM, allowing the profiler
 * to complete report generation before exiting.
 *
 * Usage:
 *   spx_signal_handler_t *handler = spx_signal_handler_create();
 *   spx_signal_handler_install(handler);
 *
 *   // In profiling hooks:
 *   spx_signal_handler_set_probing(handler, 1);
 *   // ... do profiling work ...
 *   spx_signal_handler_set_probing(handler, 0);
 *
 *   if (spx_signal_handler_should_stop(handler)) {
 *       // Clean shutdown
 *   }
 *
 *   spx_signal_handler_uninstall(handler);
 *   spx_signal_handler_destroy(handler);
 */

typedef struct spx_signal_handler_t spx_signal_handler_t;

/**
 * Callback function type for signal termination.
 * Called when a signal is received and it's safe to terminate.
 */
typedef void (*spx_signal_terminate_fn)(void);

/**
 * Create a new signal handler.
 *
 * @return New handler instance, or NULL on failure
 */
spx_signal_handler_t *spx_signal_handler_create(void);

/**
 * Destroy a signal handler and free resources.
 *
 * @param handler Handler to destroy
 */
void spx_signal_handler_destroy(spx_signal_handler_t *handler);

/**
 * Install signal handlers for SIGINT and SIGTERM.
 * Saves previous handlers for restoration.
 *
 * @param handler Signal handler instance
 * @return 0 on success, -1 on failure
 */
int spx_signal_handler_install(spx_signal_handler_t *handler);

/**
 * Uninstall signal handlers and restore previous handlers.
 *
 * @param handler Signal handler instance
 */
void spx_signal_handler_uninstall(spx_signal_handler_t *handler);

/**
 * Check if a signal has been received and profiling should stop.
 *
 * @param handler Signal handler instance
 * @return 1 if should stop, 0 otherwise
 */
int spx_signal_handler_should_stop(const spx_signal_handler_t *handler);

/**
 * Set probing state. When probing, signals are deferred until
 * probing completes to avoid interrupting critical sections.
 *
 * @param handler Signal handler instance
 * @param probing 1 if entering critical section, 0 if leaving
 */
void spx_signal_handler_set_probing(spx_signal_handler_t *handler, int probing);

/**
 * Get the signal number that was received.
 *
 * @param handler Signal handler instance
 * @return Signal number, or -1 if no signal received
 */
int spx_signal_handler_get_signo(const spx_signal_handler_t *handler);

/**
 * Set terminate callback. This function will be called when
 * it's safe to terminate after receiving a signal.
 *
 * @param handler Signal handler instance
 * @param callback Callback function
 */
void spx_signal_handler_set_terminate_callback(spx_signal_handler_t *handler,
                                               spx_signal_terminate_fn callback);

#endif /* defined(USE_SIGNAL) */

#endif /* SPX_SIGNAL_HANDLER_H */
