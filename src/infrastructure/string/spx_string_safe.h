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

#ifndef SPX_STRING_SAFE_H_DEFINED
#define SPX_STRING_SAFE_H_DEFINED

#include "../spx_error.h"
#include <stdarg.h>
#include <stddef.h>

/**
 * Safe string operations
 *
 * These functions provide bounds-checked string operations that
 * always null-terminate and prevent buffer overflows.
 */

/* Safe string copy (always null-terminates) */
size_t spx_string_copy_safe(char *dst, size_t dst_size, const char *src, spx_error_t *error);

/* Safe string concatenation */
size_t spx_string_concat_safe(char *dst, size_t dst_size, const char *src, spx_error_t *error);

/* Safe string formatting */
int spx_string_format_safe(char *dst, size_t dst_size, spx_error_t *error, const char *format, ...);

/* Safe string formatting with va_list */
int spx_string_vformat_safe(char *dst, size_t dst_size, spx_error_t *error, const char *format,
                            va_list args);

/* String utility functions */
int spx_string_starts_with(const char *str, const char *prefix);
int spx_string_ends_with(const char *str, const char *suffix);

/* Safe string length with maximum */
size_t spx_string_length_safe(const char *str, size_t max_len);

/* String sanitization */
void spx_string_sanitize_json(char *dst, size_t dst_size, const char *src);

void spx_string_sanitize_path(char *dst, size_t dst_size, const char *src);

#endif /* SPX_STRING_SAFE_H_DEFINED */
