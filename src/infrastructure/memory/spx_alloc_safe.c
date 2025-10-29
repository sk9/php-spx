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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Check for multiplication overflow */
int spx_check_mul_overflow(size_t a, size_t b, size_t *result)
{
    if (a == 0 || b == 0) {
        *result = 0;
        return 0;
    }

    /* Check if a * b would overflow */
    if (a > SIZE_MAX / b) {
        return 1;  /* Overflow would occur */
    }

    *result = a * b;
    return 0;
}

/* Check for addition overflow */
int spx_check_add_overflow(size_t a, size_t b, size_t *result)
{
    if (a > SIZE_MAX - b) {
        return 1;  /* Overflow would occur */
    }

    *result = a + b;
    return 0;
}

/* Safe malloc with error handling */
void *spx_malloc_checked(
    size_t size,
    const char *purpose,
    spx_error_t *error
) {
    if (size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "Attempted to allocate 0 bytes for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    void *ptr = malloc(size);
    if (!ptr) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_MEMORY,
                     "Failed to allocate %zu bytes for %s",
                     size, purpose ? purpose : "unknown");
        return NULL;
    }

    return ptr;
}

/* Safe calloc with error handling */
void *spx_calloc_checked(
    size_t nmemb,
    size_t size,
    const char *purpose,
    spx_error_t *error
) {
    if (nmemb == 0 || size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "Attempted to allocate 0 elements for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    /* Check for overflow */
    size_t total_size;
    if (spx_check_mul_overflow(nmemb, size, &total_size)) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW,
                     "Integer overflow: %zu * %zu for %s",
                     nmemb, size, purpose ? purpose : "unknown");
        return NULL;
    }

    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_MEMORY,
                     "Failed to allocate %zu bytes (%zu * %zu) for %s",
                     total_size, nmemb, size, purpose ? purpose : "unknown");
        return NULL;
    }

    return ptr;
}

/* Safe realloc with error handling */
void *spx_realloc_checked(
    void *ptr,
    size_t size,
    const char *purpose,
    spx_error_t *error
) {
    if (size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "Attempted to realloc to 0 bytes for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_MEMORY,
                     "Failed to reallocate to %zu bytes for %s",
                     size, purpose ? purpose : "unknown");
        return NULL;
    }

    return new_ptr;
}

/* Safe free that sets pointer to NULL */
void spx_free_safe(void **ptr)
{
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

/* Cleanup function for GCC cleanup attribute */
void spx_auto_free(void *ptr)
{
    void **p = (void **)ptr;
    if (p && *p) {
        free(*p);
        *p = NULL;
    }
}

/* Array allocation with overflow check */
void *spx_malloc_array(
    size_t nmemb,
    size_t size,
    const char *purpose,
    spx_error_t *error
) {
    /* Check for overflow */
    size_t total_size;
    if (spx_check_mul_overflow(nmemb, size, &total_size)) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW,
                     "Integer overflow: %zu * %zu for %s",
                     nmemb, size, purpose ? purpose : "unknown");
        return NULL;
    }

    return spx_malloc_checked(total_size, purpose, error);
}

/* Safe strdup */
char *spx_strdup_checked(
    const char *str,
    const char *purpose,
    spx_error_t *error
) {
    if (!str) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "NULL string for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    size_t len = strlen(str);
    size_t alloc_size;

    /* Check for overflow when adding 1 for null terminator */
    if (spx_check_add_overflow(len, 1, &alloc_size)) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW,
                     "String too long for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    char *copy = spx_malloc_checked(alloc_size, purpose, error);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, str, len + 1);
    return copy;
}

/* Safe strndup with length limit */
char *spx_strndup_checked(
    const char *str,
    size_t max_len,
    const char *purpose,
    spx_error_t *error
) {
    if (!str) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "NULL string for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    /* Find actual length (up to max_len) */
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }

    size_t alloc_size;
    if (spx_check_add_overflow(len, 1, &alloc_size)) {
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW,
                     "String too long for %s",
                     purpose ? purpose : "unknown");
        return NULL;
    }

    char *copy = spx_malloc_checked(alloc_size, purpose, error);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, str, len);
    copy[len] = '\0';

    return copy;
}
