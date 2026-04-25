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

#include "spx_alloc_safe.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *spx_strdup_checked(const char *str, const char *purpose, spx_error_t *error)
{
    if (!str) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL string for %s",
                      purpose ? purpose : "unknown");
        return NULL;
    }

    size_t len = strlen(str);
    if (len > SIZE_MAX - 1) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW, "String too long for %s",
                      purpose ? purpose : "unknown");
        return NULL;
    }

    char *copy = malloc(len + 1);
    if (!copy) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_MEMORY, "Failed to allocate %zu bytes for %s", len + 1,
                      purpose ? purpose : "unknown");
        return NULL;
    }

    memcpy(copy, str, len + 1);
    return copy;
}
