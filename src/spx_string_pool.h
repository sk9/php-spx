/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * String pool allocator for efficient string interning
 *
 * This module provides a memory-efficient way to intern strings without
 * individual malloc() calls. Useful for function/class names that persist
 * for the duration of profiling.
 */

#ifndef SPX_STRING_POOL_H_DEFINED
#define SPX_STRING_POOL_H_DEFINED

#include <stddef.h>

typedef struct spx_string_pool_t spx_string_pool_t;

/**
 * Create a new string pool.
 *
 * @return New string pool or NULL on allocation failure
 */
spx_string_pool_t *spx_string_pool_create(void);

/**
 * Intern a string in the pool.
 *
 * The returned pointer remains valid until the pool is destroyed.
 * Multiple interns of the same string will return different pointers
 * (no deduplication - use for unique strings like function names).
 *
 * @param pool String pool
 * @param str String to intern (may be NULL)
 * @return Interned string pointer or NULL if str was NULL or allocation failed
 */
const char *spx_string_pool_intern(spx_string_pool_t *pool, const char *str);

/**
 * Destroy the string pool and free all memory.
 *
 * All pointers returned by spx_string_pool_intern() become invalid.
 *
 * @param pool String pool (may be NULL)
 */
void spx_string_pool_destroy(spx_string_pool_t *pool);

/**
 * Get statistics about pool usage (for debugging/monitoring).
 *
 * @param pool String pool
 * @param total_allocated Output: total bytes allocated
 * @param total_interned Output: number of strings interned
 */
void spx_string_pool_stats(
    const spx_string_pool_t *pool,
    size_t *total_allocated,
    size_t *total_interned
);

#endif /* SPX_STRING_POOL_H_DEFINED */
