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
#include <stddef.h>

/*
 *  Bounds-checked string copy. Always null-terminates dst. Returns the
 *  number of bytes written (excluding the terminator). Sets *error if
 *  truncation occurred.
 */
size_t spx_string_copy_safe(char *dst, size_t dst_size, const char *src, spx_error_t *error);

/*
 *  Bounds-checked snprintf. Returns the number of bytes written
 *  (excluding the terminator) on success, or -1 on error or truncation.
 */
int spx_string_format_safe(char *dst, size_t dst_size, spx_error_t *error, const char *format, ...);

/*
 *  Returns 1 if str ends with suffix, 0 otherwise. Tolerates NULL inputs.
 */
int spx_string_ends_with(const char *str, const char *suffix);

/*
 *  strnlen-equivalent: stops at max_len. Tolerates NULL.
 */
size_t spx_string_length_safe(const char *str, size_t max_len);

#endif /* SPX_STRING_SAFE_H_DEFINED */
