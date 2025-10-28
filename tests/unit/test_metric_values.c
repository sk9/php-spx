/* SPX - A simple profiler for PHP
 * Unit tests for metric value operations
 */

#include "test_framework.h"
#include <stddef.h>
#include <string.h>

/* Mock the minimal types needed for testing */
#define SPX_METRIC_COUNT 22

typedef struct {
    double values[SPX_METRIC_COUNT];
} spx_profiler_metric_values_t;

/* Metric indices for testing */
#define SPX_METRIC_WALL_TIME 0
#define SPX_METRIC_CPU_TIME 1
#define SPX_METRIC_ZE_MEMORY_USAGE 3
#define SPX_METRIC_ZE_ERROR_COUNT 17

/* Include the implementation to test */
static inline void spx_metric_values_zero(spx_profiler_metric_values_t *m) {
    memset(m->values, 0, sizeof(m->values));
}

static inline void spx_metric_values_add(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        dest->values[i] += src->values[i];
    }
}

static inline void spx_metric_values_sub(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        dest->values[i] -= src->values[i];
    }
}

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

static inline void spx_metric_values_copy(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    memcpy(dest->values, src->values, sizeof(dest->values));
}

/* Tests */

TEST(zero_initializes_all_values_to_zero) {
    spx_profiler_metric_values_t metrics;

    /* Initialize with non-zero values */
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        metrics.values[i] = 123.45;
    }

    spx_metric_values_zero(&metrics);

    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        ASSERT_DOUBLE_EQ(metrics.values[i], 0.0, 0.0001);
    }
}

TEST(add_performs_element_wise_addition) {
    spx_profiler_metric_values_t a, b;

    spx_metric_values_zero(&a);
    spx_metric_values_zero(&b);

    a.values[SPX_METRIC_WALL_TIME] = 100.0;
    a.values[SPX_METRIC_CPU_TIME] = 50.0;

    b.values[SPX_METRIC_WALL_TIME] = 25.0;
    b.values[SPX_METRIC_CPU_TIME] = 10.0;

    spx_metric_values_add(&a, &b);

    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_WALL_TIME], 125.0, 0.0001);
    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_CPU_TIME], 60.0, 0.0001);
}

TEST(sub_performs_element_wise_subtraction) {
    spx_profiler_metric_values_t a, b;

    spx_metric_values_zero(&a);
    spx_metric_values_zero(&b);

    a.values[SPX_METRIC_WALL_TIME] = 100.0;
    a.values[SPX_METRIC_CPU_TIME] = 50.0;

    b.values[SPX_METRIC_WALL_TIME] = 25.0;
    b.values[SPX_METRIC_CPU_TIME] = 10.0;

    spx_metric_values_sub(&a, &b);

    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_WALL_TIME], 75.0, 0.0001);
    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_CPU_TIME], 40.0, 0.0001);
}

TEST(max_takes_maximum_of_each_element) {
    spx_profiler_metric_values_t a, b;

    spx_metric_values_zero(&a);
    spx_metric_values_zero(&b);

    a.values[SPX_METRIC_WALL_TIME] = 100.0;
    a.values[SPX_METRIC_CPU_TIME] = 30.0;
    a.values[SPX_METRIC_ZE_MEMORY_USAGE] = 50.0;

    b.values[SPX_METRIC_WALL_TIME] = 75.0;
    b.values[SPX_METRIC_CPU_TIME] = 60.0;
    b.values[SPX_METRIC_ZE_MEMORY_USAGE] = 50.0;

    spx_metric_values_max(&a, &b);

    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_WALL_TIME], 100.0, 0.0001);
    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_CPU_TIME], 60.0, 0.0001);
    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_ZE_MEMORY_USAGE], 50.0, 0.0001);
}

TEST(operations_preserve_unmodified_metrics) {
    spx_profiler_metric_values_t a, b;

    spx_metric_values_zero(&a);
    spx_metric_values_zero(&b);

    /* Set specific values */
    a.values[SPX_METRIC_ZE_ERROR_COUNT] = 42.0;
    b.values[SPX_METRIC_ZE_ERROR_COUNT] = 0.0;

    /* Perform operation on different metric */
    a.values[SPX_METRIC_WALL_TIME] = 100.0;
    b.values[SPX_METRIC_WALL_TIME] = 50.0;

    spx_metric_values_add(&a, &b);

    /* Error count should be preserved */
    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_ZE_ERROR_COUNT], 42.0, 0.0001);
}

TEST(copy_duplicates_all_values) {
    spx_profiler_metric_values_t src, dest;

    spx_metric_values_zero(&src);
    spx_metric_values_zero(&dest);

    src.values[SPX_METRIC_WALL_TIME] = 123.45;
    src.values[SPX_METRIC_CPU_TIME] = 67.89;
    src.values[SPX_METRIC_ZE_MEMORY_USAGE] = 1024.0;

    spx_metric_values_copy(&dest, &src);

    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        ASSERT_DOUBLE_EQ(dest.values[i], src.values[i], 0.0001);
    }
}

TEST(operations_handle_negative_values) {
    spx_profiler_metric_values_t a, b;

    spx_metric_values_zero(&a);
    spx_metric_values_zero(&b);

    a.values[SPX_METRIC_WALL_TIME] = 50.0;
    b.values[SPX_METRIC_WALL_TIME] = 100.0;

    spx_metric_values_sub(&a, &b);

    ASSERT_DOUBLE_EQ(a.values[SPX_METRIC_WALL_TIME], -50.0, 0.0001);
}

BEGIN_TEST_SUITE(SPX_Metric_Values)
    RUN_TEST(zero_initializes_all_values_to_zero);
    RUN_TEST(add_performs_element_wise_addition);
    RUN_TEST(sub_performs_element_wise_subtraction);
    RUN_TEST(max_takes_maximum_of_each_element);
    RUN_TEST(operations_preserve_unmodified_metrics);
    RUN_TEST(copy_duplicates_all_values);
    RUN_TEST(operations_handle_negative_values);
END_TEST_SUITE()
