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

#include "spx_string_safe.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

size_t spx_string_copy_safe(char *dst, size_t dst_size, const char *src, spx_error_t *error)
{
    if (!dst || dst_size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL or zero-size destination buffer");
        return 0;
    }

    if (!src) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL source string");
        dst[0] = '\0';
        return 0;
    }

    size_t i;
    for (i = 0; i < dst_size - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';

    if (src[i] != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW, "String truncated (buffer size: %zu)",
                      dst_size);
    }

    return i;
}

int spx_string_format_safe(char *dst, size_t dst_size, spx_error_t *error, const char *format, ...)
{
    if (!dst || dst_size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL or zero-size destination buffer");
        return -1;
    }

    if (!format) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL format string");
        dst[0] = '\0';
        return -1;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(dst, dst_size, format, args);
    va_end(args);

    if (written < 0) {
        SPX_ERROR_SET(error, SPX_ERR_INTERNAL, "vsnprintf failed");
        dst[0] = '\0';
        return -1;
    }

    if ((size_t) written >= dst_size) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW,
                      "Format string truncated (needed %d, got %zu)", written + 1, dst_size);
        return -1;
    }

    return written;
}

int spx_string_ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix) {
        return 0;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (str_len < suffix_len) {
        return 0;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

size_t spx_string_length_safe(const char *str, size_t max_len)
{
    if (!str) {
        return 0;
    }

    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }

    return len;
}
