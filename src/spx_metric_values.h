/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Type-safe metric value operations
 *
 * This module replaces the previous macro-based operations with inline functions
 * providing better type safety, debuggability, and compiler optimization opportunities.
 */

#ifndef SPX_METRIC_VALUES_H_DEFINED
#define SPX_METRIC_VALUES_H_DEFINED

#include "spx_profiler.h"
#include "spx_metric.h"
#include <string.h>

/**
 * Zero-initialize all metric values.
 *
 * @param m Pointer to metric values structure
 */
static inline void spx_metric_values_zero(spx_profiler_metric_values_t *m) {
    memset(m->values, 0, sizeof(m->values));
}

/**
 * Add metric values element-wise: dest += src
 *
 * @param dest Destination (modified in place)
 * @param src Source values to add
 */
static inline void spx_metric_values_add(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        dest->values[i] += src->values[i];
    }
}

/**
 * Subtract metric values element-wise: dest -= src
 *
 * @param dest Destination (modified in place)
 * @param src Source values to subtract
 */
static inline void spx_metric_values_sub(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        dest->values[i] -= src->values[i];
    }
}

/**
 * Take maximum of metric values element-wise: dest = max(dest, src)
 *
 * @param dest Destination (modified in place)
 * @param src Source values to compare
 */
static inline void spx_metric_values_max(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        if (src->values[i] > dest->values[i]) {
            dest->values[i] = src->values[i];
        }
    }
}

/**
 * Copy metric values: dest = src
 *
 * @param dest Destination
 * @param src Source values
 */
static inline void spx_metric_values_copy(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    memcpy(dest->values, src->values, sizeof(dest->values));
}

/* Legacy macro compatibility layer (deprecated, will be removed) */
#ifdef SPX_METRIC_VALUES_USE_LEGACY_MACROS
#define METRIC_VALUES_ZERO(m) spx_metric_values_zero(&(m))
#define METRIC_VALUES_ADD(a, b) spx_metric_values_add(&(a), &(b))
#define METRIC_VALUES_SUB(a, b) spx_metric_values_sub(&(a), &(b))
#define METRIC_VALUES_MAX(a, b) spx_metric_values_max(&(a), &(b))
#endif

#endif /* SPX_METRIC_VALUES_H_DEFINED */
