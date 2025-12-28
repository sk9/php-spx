/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Unit tests for spx_common module
 */

#include "test_framework.h"
#include "../../src/infrastructure/spx_common.h"

/*============================================================================
 * Numeric Utilities Tests
 *============================================================================*/

TEST(min_max)
{
    ASSERT_EQ(5, SPX_MIN(5, 10));
    ASSERT_EQ(5, SPX_MIN(10, 5));
    ASSERT_EQ(10, SPX_MAX(5, 10));
    ASSERT_EQ(10, SPX_MAX(10, 5));
}

TEST(clamp)
{
    ASSERT_EQ(5, SPX_CLAMP(5, 0, 10));
    ASSERT_EQ(0, SPX_CLAMP(-5, 0, 10));
    ASSERT_EQ(10, SPX_CLAMP(15, 0, 10));
}

TEST(round_up_pow2)
{
    ASSERT_EQ(1, spx_round_up_pow2(1));
    ASSERT_EQ(2, spx_round_up_pow2(2));
    ASSERT_EQ(4, spx_round_up_pow2(3));
    ASSERT_EQ(4, spx_round_up_pow2(4));
    ASSERT_EQ(8, spx_round_up_pow2(5));
    ASSERT_EQ(16, spx_round_up_pow2(9));
    ASSERT_EQ(1024, spx_round_up_pow2(1000));
}

TEST(is_pow2)
{
    ASSERT_TRUE(spx_is_pow2(1));
    ASSERT_TRUE(spx_is_pow2(2));
    ASSERT_TRUE(spx_is_pow2(4));
    ASSERT_TRUE(spx_is_pow2(8));
    ASSERT_TRUE(spx_is_pow2(1024));

    ASSERT_FALSE(spx_is_pow2(0));
    ASSERT_FALSE(spx_is_pow2(3));
    ASSERT_FALSE(spx_is_pow2(5));
    ASSERT_FALSE(spx_is_pow2(1000));
}

TEST(safe_mul_size)
{
    size_t result;

    /* Normal multiplication */
    ASSERT_EQ(0, spx_safe_mul_size(10, 20, &result));
    ASSERT_EQ(200, result);

    /* Zero multiplication */
    ASSERT_EQ(0, spx_safe_mul_size(0, 100, &result));
    ASSERT_EQ(0, result);

    ASSERT_EQ(0, spx_safe_mul_size(100, 0, &result));
    ASSERT_EQ(0, result);

    /* Overflow detection */
    ASSERT_EQ(-1, spx_safe_mul_size(SIZE_MAX, 2, &result));
    ASSERT_EQ(-1, spx_safe_mul_size(SIZE_MAX / 2 + 1, 2, &result));
}

TEST(safe_add_size)
{
    size_t result;

    /* Normal addition */
    ASSERT_EQ(0, spx_safe_add_size(10, 20, &result));
    ASSERT_EQ(30, result);

    /* Zero addition */
    ASSERT_EQ(0, spx_safe_add_size(0, 100, &result));
    ASSERT_EQ(100, result);

    /* Overflow detection */
    ASSERT_EQ(-1, spx_safe_add_size(SIZE_MAX, 1, &result));
    ASSERT_EQ(-1, spx_safe_add_size(SIZE_MAX - 10, 20, &result));
}

/*============================================================================
 * String Utilities Tests
 *============================================================================*/

TEST(strlen_safe)
{
    ASSERT_EQ(0, spx_strlen_safe(NULL));
    ASSERT_EQ(0, spx_strlen_safe(""));
    ASSERT_EQ(5, spx_strlen_safe("hello"));
}

TEST(str_empty)
{
    ASSERT_TRUE(spx_str_empty(NULL));
    ASSERT_TRUE(spx_str_empty(""));
    ASSERT_FALSE(spx_str_empty("hello"));
    ASSERT_FALSE(spx_str_empty(" "));
}

TEST(strcmp_safe)
{
    /* NULL handling */
    ASSERT_EQ(0, spx_strcmp_safe(NULL, NULL));
    ASSERT_TRUE(spx_strcmp_safe(NULL, "a") < 0);
    ASSERT_TRUE(spx_strcmp_safe("a", NULL) > 0);

    /* Normal comparison */
    ASSERT_EQ(0, spx_strcmp_safe("hello", "hello"));
    ASSERT_TRUE(spx_strcmp_safe("a", "b") < 0);
    ASSERT_TRUE(spx_strcmp_safe("b", "a") > 0);
    ASSERT_TRUE(spx_strcmp_safe("abc", "abd") < 0);
}

/*============================================================================
 * Bit Manipulation Tests
 *============================================================================*/

TEST(bit_operations)
{
    unsigned long map = 0;

    SPX_BIT_SET(map, 0);
    ASSERT_EQ(1, map);

    SPX_BIT_SET(map, 3);
    ASSERT_EQ(9, map); /* 1001 in binary */

    ASSERT_TRUE(SPX_BIT_TEST(map, 0));
    ASSERT_TRUE(SPX_BIT_TEST(map, 3));
    ASSERT_FALSE(SPX_BIT_TEST(map, 1));
    ASSERT_FALSE(SPX_BIT_TEST(map, 2));

    SPX_BIT_CLEAR(map, 0);
    ASSERT_EQ(8, map); /* 1000 in binary */
    ASSERT_FALSE(SPX_BIT_TEST(map, 0));

    SPX_BIT_TOGGLE(map, 1);
    ASSERT_EQ(10, map); /* 1010 in binary */
    SPX_BIT_TOGGLE(map, 1);
    ASSERT_EQ(8, map);
}

/*============================================================================
 * Memory Alignment Tests
 *============================================================================*/

TEST(align_up)
{
    ASSERT_EQ(0, spx_align_up(0, 8));
    ASSERT_EQ(8, spx_align_up(1, 8));
    ASSERT_EQ(8, spx_align_up(7, 8));
    ASSERT_EQ(8, spx_align_up(8, 8));
    ASSERT_EQ(16, spx_align_up(9, 8));
    ASSERT_EQ(16, spx_align_up(16, 8));
}

TEST(align_down)
{
    ASSERT_EQ(0, spx_align_down(0, 8));
    ASSERT_EQ(0, spx_align_down(1, 8));
    ASSERT_EQ(0, spx_align_down(7, 8));
    ASSERT_EQ(8, spx_align_down(8, 8));
    ASSERT_EQ(8, spx_align_down(9, 8));
    ASSERT_EQ(8, spx_align_down(15, 8));
    ASSERT_EQ(16, spx_align_down(16, 8));
}

TEST(is_aligned)
{
    char buffer[32] __attribute__((aligned(16)));

    ASSERT_TRUE(spx_is_aligned(buffer, 1));
    ASSERT_TRUE(spx_is_aligned(buffer, 2));
    ASSERT_TRUE(spx_is_aligned(buffer, 4));
    ASSERT_TRUE(spx_is_aligned(buffer, 8));
    ASSERT_TRUE(spx_is_aligned(buffer, 16));

    /* Check unaligned pointer */
    char *unaligned = buffer + 1;
    ASSERT_TRUE(spx_is_aligned(unaligned, 1));
    ASSERT_FALSE(spx_is_aligned(unaligned, 2));
}

/*============================================================================
 * Array Utilities Tests
 *============================================================================*/

TEST(array_size)
{
    int arr1[5] = {0};
    ASSERT_EQ(5, SPX_ARRAY_SIZE(arr1));

    char arr2[10] = {0};
    ASSERT_EQ(10, SPX_ARRAY_SIZE(arr2));

    struct { int a; int b; } arr3[3] = {{0, 0}, {0, 0}, {0, 0}};
    ASSERT_EQ(3, SPX_ARRAY_SIZE(arr3));
}

TEST(container_of)
{
    struct container {
        int a;
        int b;
        int c;
    };

    struct container c = {1, 2, 3};
    int *ptr_b = &c.b;

    struct container *recovered = SPX_CONTAINER_OF(ptr_b, struct container, b);
    ASSERT_EQ(&c, recovered);
    ASSERT_EQ(1, recovered->a);
    ASSERT_EQ(2, recovered->b);
    ASSERT_EQ(3, recovered->c);
}

/*============================================================================
 * Version Macros Tests
 *============================================================================*/

TEST(version_macros)
{
    ASSERT_TRUE(SPX_VERSION_MAJOR >= 0);
    ASSERT_TRUE(SPX_VERSION_MINOR >= 0);
    ASSERT_TRUE(SPX_VERSION_PATCH >= 0);

    /* Check version string is not NULL */
    ASSERT_NOT_NULL(SPX_VERSION_STRING);

    /* Check make_version produces correct value */
    unsigned int version = SPX_MAKE_VERSION(1, 2, 3);
    ASSERT_EQ((1 << 16) | (2 << 8) | 3, version);
}

/*============================================================================
 * Test Suite Entry Point
 *============================================================================*/

BEGIN_TEST_SUITE(Common)
    RUN_TEST(min_max);
    RUN_TEST(clamp);
    RUN_TEST(round_up_pow2);
    RUN_TEST(is_pow2);
    RUN_TEST(safe_mul_size);
    RUN_TEST(safe_add_size);
    RUN_TEST(strlen_safe);
    RUN_TEST(str_empty);
    RUN_TEST(strcmp_safe);
    RUN_TEST(bit_operations);
    RUN_TEST(align_up);
    RUN_TEST(align_down);
    RUN_TEST(is_aligned);
    RUN_TEST(array_size);
    RUN_TEST(container_of);
    RUN_TEST(version_macros);
END_TEST_SUITE()
