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

#include "spx_execution_context.h"
#include "spx_error.h"

#include <stdlib.h>
#include <string.h>

struct spx_execution_context_t {
    int is_cli;
    spx_session_t *session;
    spx_execution_handler_t handler;
    int has_handler;
};

spx_execution_context_t *spx_execution_context_create(int is_cli)
{
    spx_execution_context_t *ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        spx_error_set(SPX_ERR_OUT_OF_MEMORY, "Failed to allocate execution context");
        return NULL;
    }

    ctx->is_cli = is_cli;
    ctx->session = NULL;
    ctx->has_handler = 0;
    memset(&ctx->handler, 0, sizeof(ctx->handler));

    return ctx;
}

void spx_execution_context_destroy(spx_execution_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    /* Note: We don't own the session, caller manages its lifecycle */
    ctx->session = NULL;

    free(ctx);
}

int spx_execution_context_is_cli(const spx_execution_context_t *ctx)
{
    return ctx ? ctx->is_cli : 0;
}

void spx_execution_context_set_session(spx_execution_context_t *ctx, spx_session_t *session)
{
    if (ctx) {
        ctx->session = session;
    }
}

spx_session_t *spx_execution_context_get_session(spx_execution_context_t *ctx)
{
    return ctx ? ctx->session : NULL;
}

void spx_execution_context_set_handler(spx_execution_context_t *ctx,
                                       const spx_execution_handler_t *handler)
{
    if (!ctx) {
        return;
    }

    if (handler) {
        ctx->handler = *handler;
        ctx->has_handler = 1;
    } else {
        memset(&ctx->handler, 0, sizeof(ctx->handler));
        ctx->has_handler = 0;
    }
}

const spx_execution_handler_t *spx_execution_context_get_handler(const spx_execution_context_t *ctx)
{
    if (!ctx || !ctx->has_handler) {
        return NULL;
    }

    return &ctx->handler;
}
