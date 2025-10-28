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

#ifndef SPX_CALL_STACK_H
#define SPX_CALL_STACK_H

#include "spx_php.h"
#include <stddef.h>

/**
 * spx_call_stack - Function call stack management
 *
 * This module encapsulates call stack tracking logic, providing a clean
 * interface for push/pop operations and depth tracking. It replaces the
 * fixed array in php_spx.c with a configurable, reusable component.
 *
 * Usage:
 *   spx_call_stack_t *stack = spx_call_stack_create(2048);
 *
 *   spx_php_function_t func;
 *   spx_php_current_function(&func);
 *
 *   if (spx_call_stack_push(stack, &func) != 0) {
 *       // Handle overflow
 *   }
 *
 *   // ... later ...
 *
 *   spx_php_function_t popped;
 *   if (spx_call_stack_pop(stack, &popped) != 0) {
 *       // Handle underflow
 *   }
 *
 *   spx_call_stack_destroy(stack);
 */

typedef struct spx_call_stack_t spx_call_stack_t;

/**
 * Create a new call stack with the specified capacity.
 *
 * @param capacity Maximum number of stack frames
 * @return New stack instance, or NULL on failure
 */
spx_call_stack_t *spx_call_stack_create(size_t capacity);

/**
 * Destroy a call stack and free resources.
 *
 * @param stack Stack to destroy
 */
void spx_call_stack_destroy(spx_call_stack_t *stack);

/**
 * Push a function onto the stack.
 *
 * @param stack Call stack
 * @param function Function to push (copied internally)
 * @return 0 on success, -1 if stack is full
 */
int spx_call_stack_push(spx_call_stack_t *stack, const spx_php_function_t *function);

/**
 * Pop a function from the stack.
 *
 * @param stack Call stack
 * @param function Output: popped function (if not NULL)
 * @return 0 on success, -1 if stack is empty
 */
int spx_call_stack_pop(spx_call_stack_t *stack, spx_php_function_t *function);

/**
 * Peek at the top function without popping.
 *
 * @param stack Call stack
 * @param function Output: top function (if not NULL)
 * @return 0 on success, -1 if stack is empty
 */
int spx_call_stack_peek(const spx_call_stack_t *stack, spx_php_function_t *function);

/**
 * Get the current stack depth.
 *
 * @param stack Call stack
 * @return Current depth (number of frames on stack)
 */
size_t spx_call_stack_depth(const spx_call_stack_t *stack);

/**
 * Get the stack capacity.
 *
 * @param stack Call stack
 * @return Maximum capacity
 */
size_t spx_call_stack_capacity(const spx_call_stack_t *stack);

/**
 * Check if the stack is empty.
 *
 * @param stack Call stack
 * @return 1 if empty, 0 otherwise
 */
int spx_call_stack_is_empty(const spx_call_stack_t *stack);

/**
 * Check if the stack is full.
 *
 * @param stack Call stack
 * @return 1 if full, 0 otherwise
 */
int spx_call_stack_is_full(const spx_call_stack_t *stack);

/**
 * Clear the stack (reset depth to 0).
 *
 * @param stack Call stack
 */
void spx_call_stack_clear(spx_call_stack_t *stack);

/**
 * Get a frame at a specific index (0 = bottom of stack).
 *
 * @param stack Call stack
 * @param index Index of frame to retrieve (0-based)
 * @param function Output: frame at the given index (if not NULL)
 * @return 0 on success, -1 if index out of bounds
 */
int spx_call_stack_get_frame_at(const spx_call_stack_t *stack, size_t index,
                                spx_php_function_t *function);

#endif /* SPX_CALL_STACK_H */
