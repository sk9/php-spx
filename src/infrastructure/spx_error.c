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

#include "spx_error.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *error_names[] = {
    [SPX_ERR_NONE] = "SPX_ERR_NONE",
    [SPX_ERR_OUT_OF_MEMORY] = "SPX_ERR_OUT_OF_MEMORY",
    [SPX_ERR_ALLOCATION_FAILED] = "SPX_ERR_ALLOCATION_FAILED",
    [SPX_ERR_BUFFER_OVERFLOW] = "SPX_ERR_BUFFER_OVERFLOW",
    [SPX_ERR_INVALID_INPUT] = "SPX_ERR_INVALID_INPUT",
    [SPX_ERR_INVALID_CONFIG] = "SPX_ERR_INVALID_CONFIG",
    [SPX_ERR_VALIDATION_FAILED] = "SPX_ERR_VALIDATION_FAILED",
    [SPX_ERR_OUT_OF_RANGE] = "SPX_ERR_OUT_OF_RANGE",
    [SPX_ERR_PARSE_ERROR] = "SPX_ERR_PARSE_ERROR",
    [SPX_ERR_FILE_NOT_FOUND] = "SPX_ERR_FILE_NOT_FOUND",
    [SPX_ERR_FILE_ACCESS_DENIED] = "SPX_ERR_FILE_ACCESS_DENIED",
    [SPX_ERR_FILE_WRITE_FAILED] = "SPX_ERR_FILE_WRITE_FAILED",
    [SPX_ERR_FILE_READ_FAILED] = "SPX_ERR_FILE_READ_FAILED",
    [SPX_ERR_DIR_CREATE_FAILED] = "SPX_ERR_DIR_CREATE_FAILED",
    [SPX_ERR_AUTH_FAILED] = "SPX_ERR_AUTH_FAILED",
    [SPX_ERR_ACCESS_DENIED] = "SPX_ERR_ACCESS_DENIED",
    [SPX_ERR_RATE_LIMITED] = "SPX_ERR_RATE_LIMITED",
    [SPX_ERR_PATH_TRAVERSAL] = "SPX_ERR_PATH_TRAVERSAL",
    [SPX_ERR_INTERNAL] = "SPX_ERR_INTERNAL",
    [SPX_ERR_NOT_IMPLEMENTED] = "SPX_ERR_NOT_IMPLEMENTED",
    [SPX_ERR_CAPACITY_EXCEEDED] = "SPX_ERR_CAPACITY_EXCEEDED",
};

void spx_error_set(spx_error_t *error, spx_error_code_t code, const char *file, int line,
                   const char *function, const char *format, ...)
{
    if (!error) {
        return;
    }

    error->code = code;
    error->file = file;
    error->line = line;
    error->function = function;

    if (format) {
        va_list args;
        va_start(args, format);
        vsnprintf(error->message, sizeof(error->message), format, args);
        va_end(args);

        /* Ensure null termination */
        error->message[sizeof(error->message) - 1] = '\0';
    } else {
        error->message[0] = '\0';
    }
}

int spx_error_has_error(const spx_error_t *error)
{
    return error && error->code != SPX_ERR_NONE;
}

const char *spx_error_message(const spx_error_t *error)
{
    if (!error) {
        return "NULL error object";
    }

    if (error->code == SPX_ERR_NONE) {
        return "No error";
    }

    return error->message[0] ? error->message : "Unknown error";
}

const char *spx_error_code_name(spx_error_code_t code)
{
    if (code < 0 || code >= SPX_ERR_MAX) {
        return "INVALID_ERROR_CODE";
    }

    return error_names[code];
}

void spx_error_propagate(spx_error_t *dst, const spx_error_t *src)
{
    if (!dst || !src) {
        return;
    }

    if (!spx_error_has_error(src)) {
        return;
    }

    *dst = *src;
}

void spx_error_log(const spx_error_t *error)
{
    if (!error || !spx_error_has_error(error)) {
        return;
    }

    fprintf(stderr, "SPX Error [%s]: %s\n", spx_error_code_name(error->code),
            spx_error_message(error));

    if (error->file) {
        fprintf(stderr, "  at %s:%d in %s\n", error->file, error->line,
                error->function ? error->function : "unknown");
    }
}

void spx_error_clear(spx_error_t *error)
{
    if (error) {
        error->code = SPX_ERR_NONE;
        error->message[0] = '\0';
        error->file = NULL;
        error->line = 0;
        error->function = NULL;
    }
}
