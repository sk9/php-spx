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

#ifndef SPX_ALLOC_SAFE_H_DEFINED
#define SPX_ALLOC_SAFE_H_DEFINED

#include <stddef.h>
#include "../spx_error.h"

/**
 * Safe memory allocation with overflow checking
 *
 * These functions provide memory allocation with:
 * - NULL pointer checking
 * - Overflow detection
 * - Error reporting
 * - Automatic cleanup via GCC cleanup attribute
 */

/* Allocation with error handling */
void *spx_malloc_checked(
    size_t size,
    const char *purpose,
    spx_error_t *error
);

void *spx_calloc_checked(
    size_t nmemb,
    size_t size,
    const char *purpose,
    spx_error_t *error
);

void *spx_realloc_checked(
    void *ptr,
    size_t size,
    const char *purpose,
    spx_error_t *error
);

/* Safe deallocation (sets pointer to NULL) */
void spx_free_safe(void **ptr);

/* Memory cleanup helper (for use with cleanup attribute) */
void spx_auto_free(void *ptr);

#define SPX_AUTO_FREE __attribute__((cleanup(spx_auto_free)))

/* Allocator with overflow protection */
void *spx_malloc_array(
    size_t nmemb,
    size_t size,
    const char *purpose,
    spx_error_t *error
);

/* Check for multiplication overflow */
int spx_check_mul_overflow(size_t a, size_t b, size_t *result);

/* Check for addition overflow */
int spx_check_add_overflow(size_t a, size_t b, size_t *result);

/* Allocate and copy string */
char *spx_strdup_checked(
    const char *str,
    const char *purpose,
    spx_error_t *error
);

/* Allocate and copy string with length limit */
char *spx_strndup_checked(
    const char *str,
    size_t max_len,
    const char *purpose,
    spx_error_t *error
);

#endif /* SPX_ALLOC_SAFE_H_DEFINED */
