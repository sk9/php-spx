/* Unit tests for spx_string_pool */
#include "test_framework.h"
#include "../../src/spx_string_pool.h"
#include <string.h>

TEST(string_pool_create_destroy) {
    spx_string_pool_t *pool = spx_string_pool_create();
    ASSERT_NOT_NULL(pool);
    spx_string_pool_destroy(pool);
}

TEST(string_pool_intern_string) {
    spx_string_pool_t *pool = spx_string_pool_create();
    ASSERT_NOT_NULL(pool);

    const char *s1 = spx_string_pool_intern(pool, "hello");
    ASSERT_NOT_NULL(s1);
    ASSERT_STR_EQ(s1, "hello");

    spx_string_pool_destroy(pool);
}

TEST(string_pool_intern_null) {
    spx_string_pool_t *pool = spx_string_pool_create();
    ASSERT_NOT_NULL(pool);

    const char *s = spx_string_pool_intern(pool, NULL);
    ASSERT_NULL(s);

    spx_string_pool_destroy(pool);
}

TEST(string_pool_multiple_interns) {
    spx_string_pool_t *pool = spx_string_pool_create();
    ASSERT_NOT_NULL(pool);

    const char *s1 = spx_string_pool_intern(pool, "first");
    const char *s2 = spx_string_pool_intern(pool, "second");
    const char *s3 = spx_string_pool_intern(pool, "third");

    /* All interned strings should be valid */
    ASSERT_NOT_NULL(s1);
    ASSERT_NOT_NULL(s2);
    ASSERT_NOT_NULL(s3);
    ASSERT_STR_EQ(s1, "first");
    ASSERT_STR_EQ(s2, "second");
    ASSERT_STR_EQ(s3, "third");

    spx_string_pool_destroy(pool);
}

TEST(string_pool_large_strings) {
    spx_string_pool_t *pool = spx_string_pool_create();
    ASSERT_NOT_NULL(pool);

    char large[1000];
    memset(large, 'x', sizeof(large) - 1);
    large[sizeof(large) - 1] = '\0';

    const char *s = spx_string_pool_intern(pool, large);
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(strlen(s), strlen(large));

    spx_string_pool_destroy(pool);
}

TEST(string_pool_stats) {
    spx_string_pool_t *pool = spx_string_pool_create();
    ASSERT_NOT_NULL(pool);

    spx_string_pool_intern(pool, "test1");
    spx_string_pool_intern(pool, "test2");

    size_t total_allocated = 0, total_interned = 0;
    spx_string_pool_stats(pool, &total_allocated, &total_interned);

    ASSERT_EQ(total_interned, 2);
    ASSERT_TRUE(total_allocated > 0);

    spx_string_pool_destroy(pool);
}

TEST(string_pool_destroy_null) {
    /* Should not crash */
    spx_string_pool_destroy(NULL);
}

BEGIN_TEST_SUITE(SPX_String_Pool)
    RUN_TEST(string_pool_create_destroy);
    RUN_TEST(string_pool_intern_string);
    RUN_TEST(string_pool_intern_null);
    RUN_TEST(string_pool_multiple_interns);
    RUN_TEST(string_pool_large_strings);
    RUN_TEST(string_pool_stats);
    RUN_TEST(string_pool_destroy_null);
END_TEST_SUITE()
