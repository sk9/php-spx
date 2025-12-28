/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Unit tests for spx_config_builder module
 */

#include "test_framework.h"
#include "../../src/spx_config_builder.h"

/*============================================================================
 * Builder Lifecycle Tests
 *============================================================================*/

TEST(builder_create_destroy)
{
    spx_config_builder_t *builder = spx_config_builder_create();
    ASSERT_NOT_NULL(builder);
    spx_config_builder_destroy(builder);
}

TEST(builder_reset)
{
    spx_config_builder_t *builder = spx_config_builder_create();
    ASSERT_NOT_NULL(builder);

    spx_config_builder_enabled(builder, 1);
    spx_config_builder_builtins(builder, 1);

    spx_config_builder_t *result = spx_config_builder_reset(builder);
    ASSERT_EQ(result, builder);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(0, config.enabled);
    ASSERT_EQ(0, config.builtins);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Core Configuration Tests
 *============================================================================*/

TEST(builder_enabled)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_enabled(builder, 1);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(1, config.enabled);

    spx_config_builder_enabled(builder, 0);
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(0, config.enabled);

    spx_config_builder_destroy(builder);
}

TEST(builder_auto_start)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_auto_start(builder, 0);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(0, config.auto_start);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Profiling Configuration Tests
 *============================================================================*/

TEST(builder_sampling_period)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_sampling_period(builder, 1000);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(1000, config.sampling_period);

    spx_config_builder_destroy(builder);
}

TEST(builder_sampling_period_overflow)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    /* Set invalid value (exceeds maximum) */
    spx_config_builder_sampling_period(builder, 20000000);

    ASSERT_TRUE(spx_config_builder_has_errors(builder));
    ASSERT_NOT_NULL(spx_config_builder_get_error(builder));

    spx_config_builder_destroy(builder);
}

TEST(builder_builtins)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_builtins(builder, 1);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(1, config.builtins);

    spx_config_builder_destroy(builder);
}

TEST(builder_max_depth)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_max_depth(builder, 500);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(500, config.max_depth);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Metric Configuration Tests
 *============================================================================*/

TEST(builder_enable_metric)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_disable_all_metrics(builder);
    spx_config_builder_enable_metric(builder, SPX_METRIC_CPU_TIME);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(1, config.enabled_metrics[SPX_METRIC_CPU_TIME]);
    ASSERT_EQ(0, config.enabled_metrics[SPX_METRIC_WALL_TIME]);

    spx_config_builder_destroy(builder);
}

TEST(builder_enable_all_metrics)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_enable_all_metrics(builder);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));

    /* Check that at least wall time and CPU time are enabled */
    ASSERT_EQ(1, config.enabled_metrics[SPX_METRIC_WALL_TIME]);
    ASSERT_EQ(1, config.enabled_metrics[SPX_METRIC_CPU_TIME]);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Report Configuration Tests
 *============================================================================*/

TEST(builder_report_type)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_report(builder, SPX_CONFIG_REPORT_TRACE);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(SPX_CONFIG_REPORT_TRACE, config.report);

    spx_config_builder_destroy(builder);
}

TEST(builder_report_from_string)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_report_from_string(builder, "full");

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));
    ASSERT_EQ(SPX_CONFIG_REPORT_FULL, config.report);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Flat Profile Configuration Tests
 *============================================================================*/

TEST(builder_fp_config)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_fp_focus(builder, SPX_METRIC_CPU_TIME);
    spx_config_builder_fp_inc(builder, 1);
    spx_config_builder_fp_rel(builder, 1);
    spx_config_builder_fp_limit(builder, 50);
    spx_config_builder_fp_live(builder, 1);
    spx_config_builder_fp_color(builder, 0);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));

    ASSERT_EQ(SPX_METRIC_CPU_TIME, config.fp_focus);
    ASSERT_EQ(1, config.fp_inc);
    ASSERT_EQ(1, config.fp_rel);
    ASSERT_EQ(50, config.fp_limit);
    ASSERT_EQ(1, config.fp_live);
    ASSERT_EQ(0, config.fp_color);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Preset Configuration Tests
 *============================================================================*/

TEST(builder_preset_cli)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_preset_cli(builder);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));

    ASSERT_EQ(1, config.enabled);
    ASSERT_EQ(1, config.auto_start);
    ASSERT_EQ(SPX_CONFIG_REPORT_FLAT_PROFILE, config.report);

    spx_config_builder_destroy(builder);
}

TEST(builder_preset_http)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_preset_http(builder);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));

    ASSERT_EQ(1, config.enabled);
    ASSERT_EQ(SPX_CONFIG_REPORT_FULL, config.report);

    spx_config_builder_destroy(builder);
}

TEST(builder_preset_minimal)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    spx_config_builder_preset_minimal(builder);

    spx_config_t config;
    spx_error_t error = SPX_ERROR_INIT();
    ASSERT_EQ(0, spx_config_builder_build(builder, &config, &error));

    ASSERT_EQ(1, config.enabled);
    ASSERT_EQ(0, config.builtins);
    ASSERT_EQ(1, config.enabled_metrics[SPX_METRIC_WALL_TIME]);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Chaining Tests
 *============================================================================*/

TEST(builder_chaining)
{
    spx_config_builder_t *builder = spx_config_builder_create();

    /* Test that all setters return the builder for chaining */
    spx_config_builder_t *result = spx_config_builder_enabled(builder, 1);
    ASSERT_EQ(result, builder);

    result = spx_config_builder_auto_start(builder, 1);
    ASSERT_EQ(result, builder);

    result = spx_config_builder_builtins(builder, 1);
    ASSERT_EQ(result, builder);

    spx_config_builder_destroy(builder);
}

/*============================================================================
 * Test Suite Entry Point
 *============================================================================*/

BEGIN_TEST_SUITE(ConfigBuilder)
    RUN_TEST(builder_create_destroy);
    RUN_TEST(builder_reset);
    RUN_TEST(builder_enabled);
    RUN_TEST(builder_auto_start);
    RUN_TEST(builder_sampling_period);
    RUN_TEST(builder_sampling_period_overflow);
    RUN_TEST(builder_builtins);
    RUN_TEST(builder_max_depth);
    RUN_TEST(builder_enable_metric);
    RUN_TEST(builder_enable_all_metrics);
    RUN_TEST(builder_report_type);
    RUN_TEST(builder_report_from_string);
    RUN_TEST(builder_fp_config);
    RUN_TEST(builder_preset_cli);
    RUN_TEST(builder_preset_http);
    RUN_TEST(builder_preset_minimal);
    RUN_TEST(builder_chaining);
END_TEST_SUITE()
