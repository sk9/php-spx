/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Standardized error handling implementation
 */

#include "spx_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    spx_error_t code;
    char message[256];
} spx_error_context_t;

static SPX_THREAD_TLS spx_error_context_t error_context = {SPX_ERR_NONE, ""};

void spx_error_set(spx_error_t code, const char *message)
{
    error_context.code = code;
    if (message) {
        snprintf(error_context.message, sizeof(error_context.message), "%s", message);
    } else {
        error_context.message[0] = '\0';
    }
}

spx_error_t spx_error_get_last(void)
{
    return error_context.code;
}

const char *spx_error_get_message(void)
{
    return error_context.message;
}

void spx_error_clear(void)
{
    error_context.code = SPX_ERR_NONE;
    error_context.message[0] = '\0';
}

const char *spx_error_string(spx_error_t code)
{
    switch (code) {
    case SPX_ERR_NONE:
        return "No error";
    case SPX_ERR_OUT_OF_MEMORY:
        return "Out of memory";
    case SPX_ERR_CAPACITY_EXCEEDED:
        return "Capacity exceeded";
    case SPX_ERR_IO_FAILURE:
        return "I/O failure";
    case SPX_ERR_INVALID_CONFIG:
        return "Invalid configuration";
    case SPX_ERR_INTERNAL:
        return "Internal error";
    default:
        return "Unknown error";
    }
}

void spx_fatal_error(const char *message)
{
    fprintf(stderr, "SPX FATAL ERROR: %s\n", message ? message : "Unknown error");
    exit(EXIT_FAILURE);
}
