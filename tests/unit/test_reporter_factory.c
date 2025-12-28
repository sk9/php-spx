/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Unit tests for spx_reporter_factory module
 */

#include "test_framework.h"
#include "../../src/spx_reporter_factory.h"

/*============================================================================
 * Reporter Info Tests
 *============================================================================*/

TEST(reporter_get_info)
{
    const spx_reporter_info_t *info;

    info = spx_reporter_get_info(SPX_REPORTER_FULL);
    ASSERT_NOT_NULL(info);
    ASSERT_STR_EQ("full", info->name);
    ASSERT_EQ(SPX_REPORTER_FULL, info->type);
    ASSERT_TRUE(info->requires_file);

    info = spx_reporter_get_info(SPX_REPORTER_FLAT_PROFILE);
    ASSERT_NOT_NULL(info);
    ASSERT_STR_EQ("fp", info->name);
    ASSERT_TRUE(info->requires_terminal);
    ASSERT_TRUE(info->supports_live);

    info = spx_reporter_get_info(SPX_REPORTER_TRACE);
    ASSERT_NOT_NULL(info);
    ASSERT_STR_EQ("trace", info->name);

    info = spx_reporter_get_info(SPX_REPORTER_NULL);
    ASSERT_NOT_NULL(info);
    ASSERT_STR_EQ("null", info->name);

    /* Invalid type */
    info = spx_reporter_get_info(SPX_REPORTER_COUNT);
    ASSERT_NULL(info);
}

TEST(reporter_type_from_string)
{
    ASSERT_EQ(SPX_REPORTER_FULL, spx_reporter_type_from_string("full"));
    ASSERT_EQ(SPX_REPORTER_FLAT_PROFILE, spx_reporter_type_from_string("fp"));
    ASSERT_EQ(SPX_REPORTER_TRACE, spx_reporter_type_from_string("trace"));
    ASSERT_EQ(SPX_REPORTER_NULL, spx_reporter_type_from_string("null"));

    /* Aliases */
    ASSERT_EQ(SPX_REPORTER_FLAT_PROFILE, spx_reporter_type_from_string("flat_profile"));
    ASSERT_EQ(SPX_REPORTER_FLAT_PROFILE, spx_reporter_type_from_string("flat-profile"));

    /* Invalid */
    ASSERT_EQ(SPX_REPORTER_COUNT, spx_reporter_type_from_string(NULL));
    ASSERT_EQ(SPX_REPORTER_COUNT, spx_reporter_type_from_string("invalid"));
}

TEST(reporter_type_to_string)
{
    ASSERT_STR_EQ("full", spx_reporter_type_to_string(SPX_REPORTER_FULL));
    ASSERT_STR_EQ("fp", spx_reporter_type_to_string(SPX_REPORTER_FLAT_PROFILE));
    ASSERT_STR_EQ("trace", spx_reporter_type_to_string(SPX_REPORTER_TRACE));
    ASSERT_STR_EQ("null", spx_reporter_type_to_string(SPX_REPORTER_NULL));
    ASSERT_STR_EQ("unknown", spx_reporter_type_to_string(SPX_REPORTER_COUNT));
}

/*============================================================================
 * Configuration Tests
 *============================================================================*/

TEST(config_init_full)
{
    spx_reporter_config_t config;

    spx_reporter_config_init(&config, SPX_REPORTER_FULL);
    ASSERT_EQ(SPX_REPORTER_FULL, config.type);
    ASSERT_NOT_NULL(config.full.data_dir);
}

TEST(config_init_fp)
{
    spx_reporter_config_t config;

    spx_reporter_config_init(&config, SPX_REPORTER_FLAT_PROFILE);
    ASSERT_EQ(SPX_REPORTER_FLAT_PROFILE, config.type);
    ASSERT_EQ(SPX_METRIC_WALL_TIME, config.fp.focus);
    ASSERT_EQ(0, config.fp.show_inclusive);
    ASSERT_EQ(0, config.fp.show_relative);
    ASSERT_EQ(20, config.fp.limit);
    ASSERT_EQ(0, config.fp.live_mode);
    ASSERT_EQ(1, config.fp.use_color);
}

TEST(config_init_trace)
{
    spx_reporter_config_t config;

    spx_reporter_config_init(&config, SPX_REPORTER_TRACE);
    ASSERT_EQ(SPX_REPORTER_TRACE, config.type);
    ASSERT_NULL(config.trace.file_name);
    ASSERT_EQ(0, config.trace.safe_mode);
}

TEST(config_validate_valid)
{
    spx_reporter_config_t config;
    spx_error_t error = SPX_ERROR_INIT();

    spx_reporter_config_init(&config, SPX_REPORTER_FULL);
    ASSERT_EQ(0, spx_reporter_config_validate(&config, &error));

    spx_reporter_config_init(&config, SPX_REPORTER_FLAT_PROFILE);
    ASSERT_EQ(0, spx_reporter_config_validate(&config, &error));

    spx_reporter_config_init(&config, SPX_REPORTER_TRACE);
    ASSERT_EQ(0, spx_reporter_config_validate(&config, &error));

    spx_reporter_config_init(&config, SPX_REPORTER_NULL);
    ASSERT_EQ(0, spx_reporter_config_validate(&config, &error));
}

TEST(config_validate_invalid_full)
{
    spx_reporter_config_t config;
    spx_error_t error = SPX_ERROR_INIT();

    spx_reporter_config_init(&config, SPX_REPORTER_FULL);
    config.full.data_dir = NULL;  /* Invalid: NULL data_dir */

    ASSERT_EQ(-1, spx_reporter_config_validate(&config, &error));
    ASSERT_TRUE(spx_error_has_error(&error));
}

TEST(config_validate_invalid_fp)
{
    spx_reporter_config_t config;
    spx_error_t error = SPX_ERROR_INIT();

    spx_reporter_config_init(&config, SPX_REPORTER_FLAT_PROFILE);
    config.fp.focus = SPX_METRIC_COUNT;  /* Invalid focus metric */

    ASSERT_EQ(-1, spx_reporter_config_validate(&config, &error));

    spx_error_clear(&error);
    config.fp.focus = SPX_METRIC_WALL_TIME;
    config.fp.limit = 0;  /* Invalid: limit must be > 0 */

    ASSERT_EQ(-1, spx_reporter_config_validate(&config, &error));
}

/*============================================================================
 * Factory Creation Tests
 *============================================================================*/

TEST(create_null_reporter)
{
    spx_error_t error = SPX_ERROR_INIT();

    spx_profiler_reporter_t *reporter = spx_reporter_create_null(&error);
    ASSERT_NOT_NULL(reporter);
    ASSERT_FALSE(spx_error_has_error(&error));

    /* Verify it's functional */
    ASSERT_NOT_NULL(reporter->notify);
    ASSERT_NOT_NULL(reporter->destroy);

    /* Verify notify returns light cost */
    spx_profiler_reporter_cost_t cost = reporter->notify(reporter, NULL);
    ASSERT_EQ(SPX_PROFILER_REPORTER_COST_LIGHT, cost);

    spx_profiler_reporter_destroy(reporter);
}

TEST(create_via_config)
{
    spx_reporter_config_t config;
    spx_error_t error = SPX_ERROR_INIT();

    spx_reporter_config_init(&config, SPX_REPORTER_NULL);

    spx_profiler_reporter_t *reporter = spx_reporter_create(&config, &error);
    ASSERT_NOT_NULL(reporter);
    ASSERT_FALSE(spx_error_has_error(&error));

    spx_profiler_reporter_destroy(reporter);
}

TEST(create_with_invalid_config)
{
    spx_error_t error = SPX_ERROR_INIT();

    /* NULL config */
    spx_profiler_reporter_t *reporter = spx_reporter_create(NULL, &error);
    ASSERT_NULL(reporter);
    ASSERT_TRUE(spx_error_has_error(&error));

    /* Invalid type */
    spx_reporter_config_t config;
    spx_reporter_config_init(&config, SPX_REPORTER_FULL);
    config.full.data_dir = NULL;

    spx_error_clear(&error);
    reporter = spx_reporter_create(&config, &error);
    ASSERT_NULL(reporter);
    ASSERT_TRUE(spx_error_has_error(&error));
}

/*============================================================================
 * Test Suite Entry Point
 *============================================================================*/

BEGIN_TEST_SUITE(ReporterFactory)
    RUN_TEST(reporter_get_info);
    RUN_TEST(reporter_type_from_string);
    RUN_TEST(reporter_type_to_string);
    RUN_TEST(config_init_full);
    RUN_TEST(config_init_fp);
    RUN_TEST(config_init_trace);
    RUN_TEST(config_validate_valid);
    RUN_TEST(config_validate_invalid_full);
    RUN_TEST(config_validate_invalid_fp);
    RUN_TEST(create_null_reporter);
    RUN_TEST(create_via_config);
    RUN_TEST(create_with_invalid_config);
END_TEST_SUITE()
