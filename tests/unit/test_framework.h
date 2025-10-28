/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Simple C unit test framework for SPX internal components
 */

#ifndef SPX_TEST_FRAMEWORK_H
#define SPX_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test result tracking */
static struct {
    int total;
    int passed;
    int failed;
} test_stats = {0, 0, 0};

/* Color codes for output */
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RESET   "\x1b[0m"

/* Test macros */
#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        test_stats.total++; \
        printf("  Running: %s ... ", #name); \
        fflush(stdout); \
        test_##name(); \
        test_stats.passed++; \
        printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
    } \
    static void test_##name(void)

#define RUN_TEST(name) run_test_##name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            fprintf(stderr, "    Assertion failed: %s\n", #expr); \
            fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
            test_stats.failed++; \
            test_stats.passed--; \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            fprintf(stderr, "    Assertion failed: %s == %s\n", #a, #b); \
            fprintf(stderr, "    Expected: %ld, Got: %ld\n", (long)(b), (long)(a)); \
            fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
            test_stats.failed++; \
            test_stats.passed--; \
            return; \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            fprintf(stderr, "    Assertion failed: %s != %s\n", #a, #b); \
            fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
            test_stats.failed++; \
            test_stats.passed--; \
            return; \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            fprintf(stderr, "    Assertion failed: %s == %s\n", #a, #b); \
            fprintf(stderr, "    Expected: \"%s\", Got: \"%s\"\n", (b), (a)); \
            fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
            test_stats.failed++; \
            test_stats.passed--; \
            return; \
        } \
    } while (0)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)

#define ASSERT_DOUBLE_EQ(a, b, epsilon) \
    do { \
        double _diff = (a) - (b); \
        if (_diff < 0) _diff = -_diff; \
        if (_diff > (epsilon)) { \
            printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
            fprintf(stderr, "    Assertion failed: %s ~= %s (epsilon: %e)\n", #a, #b, (double)(epsilon)); \
            fprintf(stderr, "    Expected: %f, Got: %f, Diff: %e\n", (double)(b), (double)(a), _diff); \
            fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
            test_stats.failed++; \
            test_stats.passed--; \
            return; \
        } \
    } while (0)

/* Test suite runner */
#define BEGIN_TEST_SUITE(name) \
    int main(void) { \
        printf("\n" COLOR_YELLOW "=== Test Suite: %s ===" COLOR_RESET "\n\n", #name); \
        test_stats.total = 0; \
        test_stats.passed = 0; \
        test_stats.failed = 0;

#define END_TEST_SUITE() \
        printf("\n" COLOR_YELLOW "=== Results ===" COLOR_RESET "\n"); \
        printf("Total:  %d\n", test_stats.total); \
        printf("Passed: " COLOR_GREEN "%d" COLOR_RESET "\n", test_stats.passed); \
        printf("Failed: " COLOR_RED "%d" COLOR_RESET "\n\n", test_stats.failed); \
        return test_stats.failed == 0 ? 0 : 1; \
    }

#endif /* SPX_TEST_FRAMEWORK_H */
