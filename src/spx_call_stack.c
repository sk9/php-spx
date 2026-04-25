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

#include "spx_call_stack.h"

#include <stdlib.h>
#include <string.h>

struct spx_call_stack_t {
    spx_php_function_t *frames;
    size_t capacity;
    size_t depth;
};

spx_call_stack_t *spx_call_stack_create(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }

    spx_call_stack_t *stack = malloc(sizeof(*stack));
    if (!stack) {
        return NULL;
    }

    stack->frames = malloc(capacity * sizeof(spx_php_function_t));
    if (!stack->frames) {
        free(stack);
        return NULL;
    }

    stack->capacity = capacity;
    stack->depth = 0;

    return stack;
}

void spx_call_stack_destroy(spx_call_stack_t *stack)
{
    if (!stack) {
        return;
    }

    free(stack->frames);
    free(stack);
}

int spx_call_stack_push(spx_call_stack_t *stack, const spx_php_function_t *function)
{
    if (!stack || !function) {
        return -1;
    }

    if (stack->depth >= stack->capacity) {
        return -1;
    }

    stack->frames[stack->depth] = *function;
    stack->depth++;

    return 0;
}

int spx_call_stack_pop(spx_call_stack_t *stack, spx_php_function_t *function)
{
    if (!stack) {
        return -1;
    }

    if (stack->depth == 0) {
        return -1;
    }

    stack->depth--;

    if (function) {
        *function = stack->frames[stack->depth];
    }

    return 0;
}

int spx_call_stack_peek(const spx_call_stack_t *stack, spx_php_function_t *function)
{
    if (!stack) {
        return -1;
    }

    if (stack->depth == 0) {
        return -1;
    }

    if (function) {
        *function = stack->frames[stack->depth - 1];
    }

    return 0;
}

size_t spx_call_stack_depth(const spx_call_stack_t *stack)
{
    return stack ? stack->depth : 0;
}

size_t spx_call_stack_capacity(const spx_call_stack_t *stack)
{
    return stack ? stack->capacity : 0;
}

int spx_call_stack_is_empty(const spx_call_stack_t *stack)
{
    return stack ? (stack->depth == 0) : 1;
}

int spx_call_stack_is_full(const spx_call_stack_t *stack)
{
    return stack ? (stack->depth >= stack->capacity) : 1;
}

void spx_call_stack_clear(spx_call_stack_t *stack)
{
    if (stack) {
        stack->depth = 0;
    }
}

int spx_call_stack_get_frame_at(const spx_call_stack_t *stack, size_t index,
                                spx_php_function_t *function)
{
    if (!stack) {
        return -1;
    }

    if (index >= stack->depth) {
        return -1;
    }

    if (function) {
        *function = stack->frames[index];
    }

    return 0;
}
