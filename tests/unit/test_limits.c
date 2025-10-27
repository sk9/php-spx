/* SPX - A simple profiler for PHP
 * Unit tests for SPX limits configuration
 */

#include "test_framework.h"
#include <stdlib.h>

/* We'll create spx_limits.h which will be tested here */
#include "../../src/spx_limits.h"

TEST(default_stack_depth_is_2048) {
    size_t depth = spx_get_max_stack_depth();
    ASSERT_EQ(depth, 2048);
}

TEST(default_function_table_size_is_65536) {
    size_t size = spx_get_max_function_table_size();
    ASSERT_EQ(size, 65536);
}

TEST(default_reporter_buffer_size_is_16384) {
    size_t size = spx_get_reporter_buffer_size();
    ASSERT_EQ(size, 16384);
}

TEST(can_override_stack_depth_via_env) {
    setenv("SPX_MAX_STACK_DEPTH", "4096", 1);
    spx_limits_init(); /* Re-initialize to read env */

    size_t depth = spx_get_max_stack_depth();
    ASSERT_EQ(depth, 4096);

    unsetenv("SPX_MAX_STACK_DEPTH");
    spx_limits_init(); /* Reset to default */
}

TEST(can_override_function_table_size_via_env) {
    setenv("SPX_MAX_FUNCTION_TABLE_SIZE", "32768", 1);
    spx_limits_init();

    size_t size = spx_get_max_function_table_size();
    ASSERT_EQ(size, 32768);

    unsetenv("SPX_MAX_FUNCTION_TABLE_SIZE");
    spx_limits_init();
}

TEST(invalid_env_value_uses_default) {
    setenv("SPX_MAX_STACK_DEPTH", "invalid", 1);
    spx_limits_init();

    size_t depth = spx_get_max_stack_depth();
    ASSERT_EQ(depth, 2048); /* Falls back to default */

    unsetenv("SPX_MAX_STACK_DEPTH");
    spx_limits_init();
}

TEST(zero_env_value_uses_default) {
    setenv("SPX_MAX_STACK_DEPTH", "0", 1);
    spx_limits_init();

    size_t depth = spx_get_max_stack_depth();
    ASSERT_EQ(depth, 2048); /* Falls back to default */

    unsetenv("SPX_MAX_STACK_DEPTH");
    spx_limits_init();
}

TEST(json_escape_buffer_size_is_8192) {
    size_t size = spx_get_json_escape_buffer_size();
    ASSERT_EQ(size, 8192);
}

TEST(max_key_size_is_512) {
    size_t size = spx_get_max_key_size();
    ASSERT_EQ(size, 512);
}

BEGIN_TEST_SUITE(SPX_Limits)
    RUN_TEST(default_stack_depth_is_2048);
    RUN_TEST(default_function_table_size_is_65536);
    RUN_TEST(default_reporter_buffer_size_is_16384);
    RUN_TEST(can_override_stack_depth_via_env);
    RUN_TEST(can_override_function_table_size_via_env);
    RUN_TEST(invalid_env_value_uses_default);
    RUN_TEST(zero_env_value_uses_default);
    RUN_TEST(json_escape_buffer_size_is_8192);
    RUN_TEST(max_key_size_is_512);
END_TEST_SUITE()
