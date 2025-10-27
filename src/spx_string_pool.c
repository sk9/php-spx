/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * String pool allocator implementation
 */

#include "spx_string_pool.h"
#include <stdlib.h>
#include <string.h>

#define POOL_BLOCK_SIZE 8192

typedef struct pool_block_t {
    char data[POOL_BLOCK_SIZE];
    size_t used;
    struct pool_block_t *next;
} pool_block_t;

struct spx_string_pool_t {
    pool_block_t *head;
    pool_block_t *current;
    size_t total_allocated;
    size_t total_interned;
};

spx_string_pool_t *spx_string_pool_create(void) {
    spx_string_pool_t *pool = malloc(sizeof(*pool));
    if (!pool) {
        return NULL;
    }

    pool->head = malloc(sizeof(pool_block_t));
    if (!pool->head) {
        free(pool);
        return NULL;
    }

    pool->head->used = 0;
    pool->head->next = NULL;
    pool->current = pool->head;
    pool->total_allocated = POOL_BLOCK_SIZE;
    pool->total_interned = 0;

    return pool;
}

const char *spx_string_pool_intern(spx_string_pool_t *pool, const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str) + 1; /* Include null terminator */

    /* Check if current block has space */
    if (pool->current->used + len > POOL_BLOCK_SIZE) {
        /* Allocate new block */
        pool_block_t *new_block = malloc(sizeof(pool_block_t));
        if (!new_block) {
            return NULL;
        }

        new_block->used = 0;
        new_block->next = NULL;
        pool->current->next = new_block;
        pool->current = new_block;
        pool->total_allocated += POOL_BLOCK_SIZE;
    }

    char *dest = pool->current->data + pool->current->used;
    memcpy(dest, str, len);
    pool->current->used += len;
    pool->total_interned++;

    return dest;
}

void spx_string_pool_destroy(spx_string_pool_t *pool) {
    if (!pool) {
        return;
    }

    pool_block_t *block = pool->head;
    while (block) {
        pool_block_t *next = block->next;
        free(block);
        block = next;
    }

    free(pool);
}

void spx_string_pool_stats(
    const spx_string_pool_t *pool,
    size_t *total_allocated,
    size_t *total_interned
) {
    if (total_allocated) {
        *total_allocated = pool->total_allocated;
    }
    if (total_interned) {
        *total_interned = pool->total_interned;
    }
}
