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
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Safe string copy - always null-terminates */
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

    /* Check if truncation occurred */
    if (src[i] != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW, "String truncated (buffer size: %zu)",
                      dst_size);
    }

    return i;
}

/* Safe string concatenation */
size_t spx_string_concat_safe(char *dst, size_t dst_size, const char *src, spx_error_t *error)
{
    if (!dst || dst_size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL or zero-size destination buffer");
        return 0;
    }

    if (!src) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL source string");
        return strlen(dst);
    }

    /* Find current end of dst */
    size_t dst_len = 0;
    while (dst_len < dst_size && dst[dst_len] != '\0') {
        dst_len++;
    }

    if (dst_len >= dst_size) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW, "Destination buffer not null-terminated");
        return 0;
    }

    /* Copy src to end of dst */
    size_t i;
    for (i = 0; dst_len + i < dst_size - 1 && src[i] != '\0'; i++) {
        dst[dst_len + i] = src[i];
    }
    dst[dst_len + i] = '\0';

    /* Check if truncation occurred */
    if (src[i] != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW, "String truncated (buffer size: %zu)",
                      dst_size);
    }

    return dst_len + i;
}

/* Safe string formatting */
int spx_string_vformat_safe(char *dst, size_t dst_size, spx_error_t *error, const char *format,
                            va_list args)
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

    int written = vsnprintf(dst, dst_size, format, args);

    if (written < 0) {
        SPX_ERROR_SET(error, SPX_ERR_INTERNAL, "vsnprintf failed");
        dst[0] = '\0';
        return -1;
    }

    if ((size_t) written >= dst_size) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW,
                      "Format string truncated (needed %d, got %zu)", written + 1, dst_size);
        /* vsnprintf already null-terminated */
        return -1;
    }

    return written;
}

int spx_string_format_safe(char *dst, size_t dst_size, spx_error_t *error, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = spx_string_vformat_safe(dst, dst_size, error, format, args);
    va_end(args);
    return result;
}

/* String utility functions */
int spx_string_starts_with(const char *str, const char *prefix)
{
    if (!str || !prefix) {
        return 0;
    }

    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
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

/* Safe string length with maximum */
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

/* JSON sanitization */
void spx_string_sanitize_json(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) {
        return;
    }

    if (!src) {
        dst[0] = '\0';
        return;
    }

    size_t dst_idx = 0;
    size_t src_idx = 0;

    while (src[src_idx] != '\0' && dst_idx < dst_size - 2) {
        unsigned char c = src[src_idx];

        /* Characters that need escaping in JSON */
        if (c == '"' || c == '\\' || c == '/') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = c;
            }
        } else if (c == '\b') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'b';
            }
        } else if (c == '\f') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'f';
            }
        } else if (c == '\n') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'n';
            }
        } else if (c == '\r') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'r';
            }
        } else if (c == '\t') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 't';
            }
        } else if (c < 0x20) {
            /* Control characters - skip them */
            src_idx++;
            continue;
        } else {
            dst[dst_idx++] = c;
        }

        src_idx++;
    }

    dst[dst_idx] = '\0';
}

/* Path sanitization - removes potentially dangerous characters */
void spx_string_sanitize_path(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) {
        return;
    }

    if (!src) {
        dst[0] = '\0';
        return;
    }

    size_t dst_idx = 0;
    size_t src_idx = 0;

    while (src[src_idx] != '\0' && dst_idx < dst_size - 1) {
        unsigned char c = src[src_idx];

        /* Allow only safe characters for paths */
        if (isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/') {
            dst[dst_idx++] = c;
        } else {
            /* Replace unsafe characters with underscore */
            dst[dst_idx++] = '_';
        }

        src_idx++;
    }

    dst[dst_idx] = '\0';
}
