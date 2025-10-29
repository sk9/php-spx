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

#ifndef SPX_ERROR_H_DEFINED
#define SPX_ERROR_H_DEFINED

#include <stddef.h>

typedef enum {
    SPX_ERR_NONE = 0,

    /* Memory errors */
    SPX_ERR_OUT_OF_MEMORY,
    SPX_ERR_ALLOCATION_FAILED,
    SPX_ERR_BUFFER_OVERFLOW,

    /* Validation errors */
    SPX_ERR_INVALID_INPUT,
    SPX_ERR_INVALID_CONFIG,
    SPX_ERR_VALIDATION_FAILED,
    SPX_ERR_OUT_OF_RANGE,
    SPX_ERR_PARSE_ERROR,

    /* I/O errors */
    SPX_ERR_FILE_NOT_FOUND,
    SPX_ERR_FILE_ACCESS_DENIED,
    SPX_ERR_FILE_WRITE_FAILED,
    SPX_ERR_FILE_READ_FAILED,
    SPX_ERR_DIR_CREATE_FAILED,

    /* Security errors */
    SPX_ERR_AUTH_FAILED,
    SPX_ERR_ACCESS_DENIED,
    SPX_ERR_RATE_LIMITED,
    SPX_ERR_PATH_TRAVERSAL,

    /* Internal errors */
    SPX_ERR_INTERNAL,
    SPX_ERR_NOT_IMPLEMENTED,
    SPX_ERR_CAPACITY_EXCEEDED,

    SPX_ERR_MAX
} spx_error_code_t;

typedef struct {
    spx_error_code_t code;
    char message[512];
    const char *file;
    int line;
    const char *function;
} spx_error_t;

/* Error construction */
#define SPX_ERROR_INIT() ((spx_error_t){SPX_ERR_NONE, {0}, NULL, 0, NULL})

void spx_error_set(
    spx_error_t *error,
    spx_error_code_t code,
    const char *file,
    int line,
    const char *function,
    const char *format,
    ...
);

#define SPX_ERROR_SET(err, code, ...) \
    spx_error_set(err, code, __FILE__, __LINE__, __func__, __VA_ARGS__)

/* Error checking */
int spx_error_has_error(const spx_error_t *error);
const char *spx_error_message(const spx_error_t *error);
const char *spx_error_code_name(spx_error_code_t code);

/* Error propagation */
void spx_error_propagate(spx_error_t *dst, const spx_error_t *src);

/* Error logging */
void spx_error_log(const spx_error_t *error);

/* Clear error */
void spx_error_clear(spx_error_t *error);

#endif /* SPX_ERROR_H_DEFINED */
