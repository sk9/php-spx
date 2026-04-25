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

#ifndef SPX_SECURITY_VALIDATION_H_DEFINED
#define SPX_SECURITY_VALIDATION_H_DEFINED

#include "../spx_error.h"
#include <stddef.h>

typedef enum {
    SPX_VALIDATE_SUCCESS = 0,
    SPX_VALIDATE_NULL_INPUT,
    SPX_VALIDATE_TOO_LONG,
    SPX_VALIDATE_INVALID_FORMAT
} spx_validate_result_t;

typedef enum {
    SPX_PARSE_SUCCESS = 0,
    SPX_PARSE_NULL_INPUT,
    SPX_PARSE_EMPTY_STRING,
    SPX_PARSE_INVALID_FORMAT,
    SPX_PARSE_OVERFLOW,
    SPX_PARSE_UNDERFLOW,
    SPX_PARSE_OUT_OF_RANGE
} spx_parse_result_t;

/*
 *  Strict strtol wrapper. Verifies the entire input was consumed, ranges
 *  are respected, no overflow, and reports errors via spx_error_t.
 */
spx_parse_result_t spx_parse_long(const char *str, long *out_value, long min_value, long max_value,
                                  spx_error_t *error);

typedef enum {
    SPX_PATH_ALLOW_RELATIVE = 1 << 0,
    SPX_PATH_ALLOW_SYMLINKS = 1 << 1,
    SPX_PATH_MUST_EXIST = 1 << 2,
    SPX_PATH_MUST_BE_DIR = 1 << 3,
    SPX_PATH_MUST_BE_FILE = 1 << 4
} spx_path_flags_t;

/*
 *  Resolve and confine `path` (interpreted relative to `base_dir`) to a path
 *  that is fully contained inside `base_dir`. Rejects symlinks unless
 *  SPX_PATH_ALLOW_SYMLINKS is set. Writes the canonicalized path into
 *  resolved_path (which must be at least PATH_MAX bytes).
 */
spx_validate_result_t spx_validate_path(const char *path, const char *base_dir,
                                        spx_path_flags_t flags, char *resolved_path,
                                        size_t resolved_path_size, spx_error_t *error);

#endif /* SPX_SECURITY_VALIDATION_H_DEFINED */
