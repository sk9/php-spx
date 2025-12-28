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

/**
 * @file spx_common.h
 * @brief Common utilities and macros for SPX
 *
 * This header provides common macros, type definitions, and utility functions
 * used throughout the SPX codebase. It follows the DRY principle by centralizing
 * commonly used patterns.
 *
 * Categories:
 * - Compiler attributes and hints
 * - Memory management utilities
 * - Array and container helpers
 * - Debug and assertion macros
 * - Common type definitions
 */

#ifndef SPX_COMMON_H_DEFINED
#define SPX_COMMON_H_DEFINED

#include <stddef.h>
#include <stdint.h>

/*============================================================================
 * Compiler Detection and Attributes
 *============================================================================*/

/* GCC/Clang attributes */
#ifdef __GNUC__
#define SPX_LIKELY(x)       __builtin_expect(!!(x), 1)
#define SPX_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#define SPX_UNUSED          __attribute__((unused))
#define SPX_NORETURN        __attribute__((noreturn))
#define SPX_PURE            __attribute__((pure))
#define SPX_CONST           __attribute__((const))
#define SPX_MALLOC          __attribute__((malloc))
#define SPX_HOT             __attribute__((hot))
#define SPX_COLD            __attribute__((cold))
#define SPX_NOINLINE        __attribute__((noinline))
#define SPX_ALWAYS_INLINE   __attribute__((always_inline)) inline
#define SPX_PACKED          __attribute__((packed))
#define SPX_ALIGNED(n)      __attribute__((aligned(n)))
#define SPX_DEPRECATED(msg) __attribute__((deprecated(msg)))
#define SPX_WARN_UNUSED     __attribute__((warn_unused_result))
#define SPX_PRINTF(f, a)    __attribute__((format(printf, f, a)))
#else
#define SPX_LIKELY(x)       (x)
#define SPX_UNLIKELY(x)     (x)
#define SPX_UNUSED
#define SPX_NORETURN
#define SPX_PURE
#define SPX_CONST
#define SPX_MALLOC
#define SPX_HOT
#define SPX_COLD
#define SPX_NOINLINE
#define SPX_ALWAYS_INLINE   inline
#define SPX_PACKED
#define SPX_ALIGNED(n)
#define SPX_DEPRECATED(msg)
#define SPX_WARN_UNUSED
#define SPX_PRINTF(f, a)
#endif

/*============================================================================
 * Thread-Local Storage
 *============================================================================*/

#ifdef ZTS
#define SPX_THREAD_TLS __thread
#else
#define SPX_THREAD_TLS
#endif

/*============================================================================
 * Array and Container Utilities
 *============================================================================*/

/**
 * @brief Get array element count at compile time
 * @param arr Array (not pointer)
 */
#define SPX_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Get offset of member within struct
 * @param type Struct type
 * @param member Member name
 */
#define SPX_OFFSET_OF(type, member) ((size_t)&((type *)0)->member)

/**
 * @brief Get container from member pointer
 * @param ptr Pointer to member
 * @param type Container type
 * @param member Member name
 */
#define SPX_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - SPX_OFFSET_OF(type, member)))

/*============================================================================
 * Numeric Utilities
 *============================================================================*/

/**
 * @brief Get minimum of two values
 */
#define SPX_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Get maximum of two values
 */
#define SPX_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Clamp value to range
 */
#define SPX_CLAMP(val, min, max) SPX_MIN(SPX_MAX(val, min), max)

/**
 * @brief Round up to power of 2
 */
static inline size_t spx_round_up_pow2(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    n |= n >> 32;
#endif
    n++;
    return n;
}

/**
 * @brief Check if value is power of 2
 */
static inline int spx_is_pow2(size_t n)
{
    return n && !(n & (n - 1));
}

/**
 * @brief Check for size_t multiplication overflow
 * @param a First operand
 * @param b Second operand
 * @param result Output result
 * @return 0 if no overflow, -1 if overflow
 */
static inline int spx_safe_mul_size(size_t a, size_t b, size_t *result)
{
    if (a != 0 && b > SIZE_MAX / a) {
        return -1;
    }
    *result = a * b;
    return 0;
}

/**
 * @brief Check for size_t addition overflow
 * @param a First operand
 * @param b Second operand
 * @param result Output result
 * @return 0 if no overflow, -1 if overflow
 */
static inline int spx_safe_add_size(size_t a, size_t b, size_t *result)
{
    if (a > SIZE_MAX - b) {
        return -1;
    }
    *result = a + b;
    return 0;
}

/*============================================================================
 * String Utilities
 *============================================================================*/

/**
 * @brief Safe string length (returns 0 for NULL)
 */
static inline size_t spx_strlen_safe(const char *s)
{
    if (!s) return 0;
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/**
 * @brief Check if string is empty or NULL
 */
static inline int spx_str_empty(const char *s)
{
    return !s || s[0] == '\0';
}

/**
 * @brief Safe string comparison (handles NULL)
 * @return 0 if equal, non-zero otherwise
 */
static inline int spx_strcmp_safe(const char *a, const char *b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/*============================================================================
 * Bit Manipulation
 *============================================================================*/

/**
 * @brief Set bit in bitmap
 */
#define SPX_BIT_SET(map, bit) ((map) |= (1UL << (bit)))

/**
 * @brief Clear bit in bitmap
 */
#define SPX_BIT_CLEAR(map, bit) ((map) &= ~(1UL << (bit)))

/**
 * @brief Toggle bit in bitmap
 */
#define SPX_BIT_TOGGLE(map, bit) ((map) ^= (1UL << (bit)))

/**
 * @brief Test bit in bitmap
 */
#define SPX_BIT_TEST(map, bit) (((map) >> (bit)) & 1UL)

/*============================================================================
 * Memory Alignment
 *============================================================================*/

/**
 * @brief Align size up to boundary
 * @param size Size to align
 * @param align Alignment boundary (must be power of 2)
 */
static inline size_t spx_align_up(size_t size, size_t align)
{
    return (size + align - 1) & ~(align - 1);
}

/**
 * @brief Align size down to boundary
 * @param size Size to align
 * @param align Alignment boundary (must be power of 2)
 */
static inline size_t spx_align_down(size_t size, size_t align)
{
    return size & ~(align - 1);
}

/**
 * @brief Check if pointer is aligned
 * @param ptr Pointer to check
 * @param align Alignment boundary
 */
static inline int spx_is_aligned(const void *ptr, size_t align)
{
    return ((uintptr_t)ptr & (align - 1)) == 0;
}

/*============================================================================
 * Debug and Assertion
 *============================================================================*/

#ifdef SPX_DEBUG
#include <stdio.h>
#include <stdlib.h>

#define SPX_ASSERT(cond) do { \
    if (SPX_UNLIKELY(!(cond))) { \
        fprintf(stderr, "SPX assertion failed: %s at %s:%d in %s\n", \
                #cond, __FILE__, __LINE__, __func__); \
        abort(); \
    } \
} while (0)

#define SPX_DEBUG_LOG(fmt, ...) \
    fprintf(stderr, "[SPX DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#else
#define SPX_ASSERT(cond) ((void)0)
#define SPX_DEBUG_LOG(fmt, ...) ((void)0)
#endif

/*============================================================================
 * Result Types (for modern error handling)
 *============================================================================*/

/**
 * @brief Generic result type for functions that can fail
 *
 * Usage:
 * @code
 * SPX_RESULT_DEFINE(my_result, int)
 *
 * my_result_t my_function(void) {
 *     if (error) {
 *         return (my_result_t){ .ok = 0, .error = "Something failed" };
 *     }
 *     return (my_result_t){ .ok = 1, .value = 42 };
 * }
 * @endcode
 */
#define SPX_RESULT_DEFINE(name, type) \
    typedef struct { \
        int ok; \
        union { \
            type value; \
            const char *error; \
        }; \
    } name##_t

/*============================================================================
 * Optional Types (for explicit null handling)
 *============================================================================*/

/**
 * @brief Optional type macro
 *
 * Usage:
 * @code
 * SPX_OPTIONAL_DEFINE(my_optional, int)
 *
 * my_optional_t get_value(void) {
 *     if (not_found) {
 *         return (my_optional_t){ .has_value = 0 };
 *     }
 *     return (my_optional_t){ .has_value = 1, .value = 42 };
 * }
 * @endcode
 */
#define SPX_OPTIONAL_DEFINE(name, type) \
    typedef struct { \
        int has_value; \
        type value; \
    } name##_t

/*============================================================================
 * Loop Helpers
 *============================================================================*/

/**
 * @brief Iterate over array elements
 */
#define SPX_FOREACH(arr, len, type, var, body) do { \
    size_t _i; \
    for (_i = 0; _i < (len); _i++) { \
        type var = (arr)[_i]; \
        body \
    } \
} while (0)

/**
 * @brief Iterate with index
 */
#define SPX_FOREACH_IDX(arr, len, type, var, idx, body) do { \
    size_t idx; \
    for (idx = 0; idx < (len); idx++) { \
        type var = (arr)[idx]; \
        body \
    } \
} while (0)

/*============================================================================
 * Cleanup Helpers (RAII-style for C)
 *============================================================================*/

#ifdef __GNUC__
/**
 * @brief Auto-cleanup helper for automatic resource release
 *
 * Usage:
 * @code
 * static void cleanup_ptr(void **ptr) { free(*ptr); }
 * SPX_AUTO_CLEANUP(cleanup_ptr) void *buf = malloc(100);
 * // buf is automatically freed when scope exits
 * @endcode
 */
#define SPX_AUTO_CLEANUP(func) __attribute__((cleanup(func)))

/**
 * @brief Auto-free helper
 */
static inline void spx_auto_free_ptr(void *ptr)
{
    void **p = ptr;
    if (*p) {
        free(*p);
        *p = NULL;
    }
}

#define SPX_AUTO_FREE SPX_AUTO_CLEANUP(spx_auto_free_ptr)

#else
#define SPX_AUTO_CLEANUP(func)
#define SPX_AUTO_FREE
#endif

/*============================================================================
 * Version and Feature Macros
 *============================================================================*/

#define SPX_VERSION_MAJOR 0
#define SPX_VERSION_MINOR 6
#define SPX_VERSION_PATCH 0

#define SPX_VERSION_STRING "0.6.0"

#define SPX_MAKE_VERSION(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

#define SPX_VERSION \
    SPX_MAKE_VERSION(SPX_VERSION_MAJOR, SPX_VERSION_MINOR, SPX_VERSION_PATCH)

#endif /* SPX_COMMON_H_DEFINED */
