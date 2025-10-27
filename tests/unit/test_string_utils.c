/* SPX - A simple profiler for PHP
 * Unit tests for string utilities
 */

#define _POSIX_C_SOURCE 200809L

#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

/* String duplication utility */
static inline char *spx_strdup_safe(const char *str) {
    return str ? strdup(str) : NULL;
}

static inline char *spx_strdup_or_default(const char *str, const char *default_val) {
    if (!str || !*str) {
        return strdup(default_val);
    }
    return strdup(str);
}

TEST(strdup_safe_handles_null) {
    char *result = spx_strdup_safe(NULL);
    ASSERT_NULL(result);
}

TEST(strdup_safe_duplicates_string) {
    const char *original = "test string";
    char *result = spx_strdup_safe(original);

    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, original);
    ASSERT_NE((long)result, (long)original); /* Different pointers */

    free(result);
}

TEST(strdup_safe_handles_empty_string) {
    const char *original = "";
    char *result = spx_strdup_safe(original);

    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "");

    free(result);
}

TEST(strdup_or_default_uses_default_for_null) {
    char *result = spx_strdup_or_default(NULL, "default");

    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "default");

    free(result);
}

TEST(strdup_or_default_uses_default_for_empty) {
    char *result = spx_strdup_or_default("", "n/a");

    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "n/a");

    free(result);
}

TEST(strdup_or_default_uses_original_when_valid) {
    char *result = spx_strdup_or_default("valid", "default");

    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "valid");

    free(result);
}

BEGIN_TEST_SUITE(SPX_String_Utils)
    RUN_TEST(strdup_safe_handles_null);
    RUN_TEST(strdup_safe_duplicates_string);
    RUN_TEST(strdup_safe_handles_empty_string);
    RUN_TEST(strdup_or_default_uses_default_for_null);
    RUN_TEST(strdup_or_default_uses_default_for_empty);
    RUN_TEST(strdup_or_default_uses_original_when_valid);
END_TEST_SUITE()
