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

#ifndef SPX_EXECUTION_CONTEXT_H
#define SPX_EXECUTION_CONTEXT_H

#include "spx_session.h"
#include <stddef.h>

/**
 * spx_execution_context - Manages PHP execution environment context
 *
 * This module encapsulates execution-related state separate from profiling session state.
 * It tracks whether we're in CLI mode, manages call stack tracking, and holds a reference
 * to the current profiling session.
 *
 * Benefits:
 * - Separates execution concerns from profiling concerns
 * - Makes the execution handler lifecycle explicit
 * - Reduces coupling between PHP extension lifecycle and profiling
 *
 * Usage:
 *   spx_execution_context_t *ctx = spx_execution_context_create(is_cli);
 *   spx_execution_context_set_session(ctx, session);
 *   // ... execution happens ...
 *   spx_execution_context_destroy(ctx);
 */

typedef struct spx_execution_context_t spx_execution_context_t;

/* Function pointer type for execution handlers */
typedef void (*spx_execution_handler_fn)(void);

typedef struct {
    spx_execution_handler_fn init;
    spx_execution_handler_fn shutdown;
} spx_execution_handler_t;

/**
 * Create a new execution context.
 *
 * @param is_cli Whether this is CLI SAPI (1) or web SAPI (0)
 * @return New context instance, or NULL on failure
 */
spx_execution_context_t *spx_execution_context_create(int is_cli);

/**
 * Destroy an execution context and free all resources.
 *
 * @param ctx Context to destroy
 */
void spx_execution_context_destroy(spx_execution_context_t *ctx);

/**
 * Check if this is a CLI SAPI context.
 *
 * @param ctx Context
 * @return 1 if CLI, 0 if web SAPI
 */
int spx_execution_context_is_cli(const spx_execution_context_t *ctx);

/**
 * Set the profiling session for this context.
 *
 * @param ctx Context
 * @param session Session to associate (can be NULL to clear)
 */
void spx_execution_context_set_session(spx_execution_context_t *ctx, spx_session_t *session);

/**
 * Get the profiling session for this context.
 *
 * @param ctx Context
 * @return Current session, or NULL if none
 */
spx_session_t *spx_execution_context_get_session(spx_execution_context_t *ctx);

/**
 * Set the execution handler for this context.
 *
 * @param ctx Context
 * @param handler Execution handler to use
 */
void spx_execution_context_set_handler(spx_execution_context_t *ctx,
                                       const spx_execution_handler_t *handler);

/**
 * Get the execution handler for this context.
 *
 * @param ctx Context
 * @return Current handler, or NULL if none
 */
const spx_execution_handler_t *spx_execution_context_get_handler(
    const spx_execution_context_t *ctx);

#endif /* SPX_EXECUTION_CONTEXT_H */
