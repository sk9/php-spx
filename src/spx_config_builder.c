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
 * @file spx_config_builder.c
 * @brief Implementation of the configuration builder
 */

#include <stdlib.h>
#include <string.h>

#include "spx_config_builder.h"
#include "spx_utils.h"
#include "infrastructure/spx_error.h"

/*============================================================================
 * Builder Structure
 *============================================================================*/

struct spx_config_builder_t {
    /* Internal configuration state */
    spx_config_t config;

    /* Validation state */
    int has_error;
    char error_message[256];

    /* Ownership tracking for copied strings */
    char *owned_key;
    char *owned_trace_file;
    char *owned_ui_uri;
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void set_builder_error(spx_config_builder_t *builder, const char *message)
{
    if (!builder) {
        return;
    }
    builder->has_error = 1;
    strncpy(builder->error_message, message, sizeof(builder->error_message) - 1);
    builder->error_message[sizeof(builder->error_message) - 1] = '\0';
}

static char *duplicate_string(const char *src)
{
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src);
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, src, len + 1);
    }
    return dup;
}

/*============================================================================
 * Builder Lifecycle
 *============================================================================*/

spx_config_builder_t *spx_config_builder_create(void)
{
    spx_config_builder_t *builder = malloc(sizeof(*builder));
    if (!builder) {
        return NULL;
    }

    memset(builder, 0, sizeof(*builder));
    spx_config_builder_reset(builder);

    return builder;
}

void spx_config_builder_destroy(spx_config_builder_t *builder)
{
    if (!builder) {
        return;
    }

    free(builder->owned_key);
    free(builder->owned_trace_file);
    free(builder->owned_ui_uri);
    free(builder);
}

spx_config_builder_t *spx_config_builder_reset(spx_config_builder_t *builder)
{
    if (!builder) {
        return NULL;
    }

    /* Free owned strings */
    free(builder->owned_key);
    free(builder->owned_trace_file);
    free(builder->owned_ui_uri);
    builder->owned_key = NULL;
    builder->owned_trace_file = NULL;
    builder->owned_ui_uri = NULL;

    /* Reset error state */
    builder->has_error = 0;
    builder->error_message[0] = '\0';

    /* Initialize config with defaults */
    memset(&builder->config, 0, sizeof(builder->config));

    builder->config.enabled = 0;
    builder->config.key = NULL;
    builder->config.ui_uri = NULL;
    builder->config.auto_start = 1;
    builder->config.sampling_period = 0;
    builder->config.builtins = 0;
    builder->config.max_depth = 0;

    /* Default metrics */
    SPX_METRIC_FOREACH(i, {
        builder->config.enabled_metrics[i] = 0;
    });
    builder->config.enabled_metrics[SPX_METRIC_WALL_TIME] = 1;
    builder->config.enabled_metrics[SPX_METRIC_ZE_MEMORY_USAGE] = 1;

    /* Default report settings */
    builder->config.report = SPX_CONFIG_REPORT_FLAT_PROFILE;

    /* Flat profile defaults */
    builder->config.fp_focus = SPX_METRIC_WALL_TIME;
    builder->config.fp_inc = 0;
    builder->config.fp_rel = 0;
    builder->config.fp_limit = 10;
    builder->config.fp_live = 0;
    builder->config.fp_color = 1;

    /* Trace defaults */
    builder->config.trace_file = NULL;
    builder->config.trace_safe = 0;

    return builder;
}

/*============================================================================
 * Core Configuration Setters
 *============================================================================*/

spx_config_builder_t *spx_config_builder_enabled(
    spx_config_builder_t *builder,
    int enabled)
{
    if (builder) {
        builder->config.enabled = enabled ? 1 : 0;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_key(
    spx_config_builder_t *builder,
    const char *key)
{
    if (!builder) {
        return NULL;
    }

    free(builder->owned_key);
    builder->owned_key = duplicate_string(key);
    builder->config.key = builder->owned_key;

    return builder;
}

spx_config_builder_t *spx_config_builder_auto_start(
    spx_config_builder_t *builder,
    int auto_start)
{
    if (builder) {
        builder->config.auto_start = auto_start ? 1 : 0;
    }
    return builder;
}

/*============================================================================
 * Profiling Configuration Setters
 *============================================================================*/

spx_config_builder_t *spx_config_builder_sampling_period(
    spx_config_builder_t *builder,
    size_t period)
{
    if (builder) {
        if (period > 10000000) {
            set_builder_error(builder, "Sampling period exceeds maximum (10M)");
        } else {
            builder->config.sampling_period = period;
        }
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_builtins(
    spx_config_builder_t *builder,
    int builtins)
{
    if (builder) {
        builder->config.builtins = builtins ? 1 : 0;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_max_depth(
    spx_config_builder_t *builder,
    size_t max_depth)
{
    if (builder) {
        if (max_depth > 100000) {
            set_builder_error(builder, "Max depth exceeds maximum (100000)");
        } else {
            builder->config.max_depth = max_depth;
        }
    }
    return builder;
}

/*============================================================================
 * Metric Configuration
 *============================================================================*/

spx_config_builder_t *spx_config_builder_enable_metric(
    spx_config_builder_t *builder,
    spx_metric_t metric)
{
    if (builder && metric < SPX_METRIC_COUNT) {
        builder->config.enabled_metrics[metric] = 1;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_disable_metric(
    spx_config_builder_t *builder,
    spx_metric_t metric)
{
    if (builder && metric < SPX_METRIC_COUNT) {
        builder->config.enabled_metrics[metric] = 0;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_metrics_from_string(
    spx_config_builder_t *builder,
    const char *metrics)
{
    if (!builder || !metrics) {
        return builder;
    }

    /* Disable all metrics first */
    SPX_METRIC_FOREACH(i, {
        builder->config.enabled_metrics[i] = 0;
    });

    /* Parse comma-separated list */
    SPX_UTILS_TOKENIZE_STRING(metrics, ',', token, 32, {
        spx_metric_t metric = spx_metric_get_by_key(token);
        if (metric != SPX_METRIC_NONE) {
            builder->config.enabled_metrics[metric] = 1;
        }
    });

    return builder;
}

spx_config_builder_t *spx_config_builder_enable_all_metrics(
    spx_config_builder_t *builder)
{
    if (builder) {
        SPX_METRIC_FOREACH(i, {
            builder->config.enabled_metrics[i] = 1;
        });
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_disable_all_metrics(
    spx_config_builder_t *builder)
{
    if (builder) {
        SPX_METRIC_FOREACH(i, {
            builder->config.enabled_metrics[i] = 0;
        });
    }
    return builder;
}

/*============================================================================
 * Report Configuration
 *============================================================================*/

spx_config_builder_t *spx_config_builder_report(
    spx_config_builder_t *builder,
    spx_config_report_t report)
{
    if (builder) {
        builder->config.report = report;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_report_from_string(
    spx_config_builder_t *builder,
    const char *report_str)
{
    if (!builder || !report_str) {
        return builder;
    }

    if (strcmp(report_str, "full") == 0) {
        builder->config.report = SPX_CONFIG_REPORT_FULL;
    } else if (strcmp(report_str, "fp") == 0) {
        builder->config.report = SPX_CONFIG_REPORT_FLAT_PROFILE;
    } else if (strcmp(report_str, "trace") == 0) {
        builder->config.report = SPX_CONFIG_REPORT_TRACE;
    } else {
        set_builder_error(builder, "Invalid report type");
    }

    return builder;
}

/*============================================================================
 * Flat Profile Configuration
 *============================================================================*/

spx_config_builder_t *spx_config_builder_fp_focus(
    spx_config_builder_t *builder,
    spx_metric_t focus)
{
    if (builder) {
        if (focus >= SPX_METRIC_COUNT) {
            set_builder_error(builder, "Invalid focus metric");
        } else {
            builder->config.fp_focus = focus;
        }
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_fp_inc(
    spx_config_builder_t *builder,
    int inc)
{
    if (builder) {
        builder->config.fp_inc = inc ? 1 : 0;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_fp_rel(
    spx_config_builder_t *builder,
    int rel)
{
    if (builder) {
        builder->config.fp_rel = rel ? 1 : 0;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_fp_limit(
    spx_config_builder_t *builder,
    size_t limit)
{
    if (builder) {
        if (limit > 1000000) {
            set_builder_error(builder, "FP limit exceeds maximum (1M)");
        } else {
            builder->config.fp_limit = limit;
        }
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_fp_live(
    spx_config_builder_t *builder,
    int live)
{
    if (builder) {
        builder->config.fp_live = live ? 1 : 0;
    }
    return builder;
}

spx_config_builder_t *spx_config_builder_fp_color(
    spx_config_builder_t *builder,
    int color)
{
    if (builder) {
        builder->config.fp_color = color ? 1 : 0;
    }
    return builder;
}

/*============================================================================
 * Trace Configuration
 *============================================================================*/

spx_config_builder_t *spx_config_builder_trace_file(
    spx_config_builder_t *builder,
    const char *file)
{
    if (!builder) {
        return NULL;
    }

    free(builder->owned_trace_file);
    builder->owned_trace_file = duplicate_string(file);
    builder->config.trace_file = builder->owned_trace_file;

    return builder;
}

spx_config_builder_t *spx_config_builder_trace_safe(
    spx_config_builder_t *builder,
    int safe)
{
    if (builder) {
        builder->config.trace_safe = safe ? 1 : 0;
    }
    return builder;
}

/*============================================================================
 * HTTP UI Configuration
 *============================================================================*/

spx_config_builder_t *spx_config_builder_ui_uri(
    spx_config_builder_t *builder,
    const char *uri)
{
    if (!builder) {
        return NULL;
    }

    free(builder->owned_ui_uri);
    builder->owned_ui_uri = duplicate_string(uri);
    builder->config.ui_uri = builder->owned_ui_uri;

    return builder;
}

/*============================================================================
 * Build and Validation
 *============================================================================*/

int spx_config_builder_validate(
    spx_config_builder_t *builder,
    spx_error_t *error)
{
    if (!builder) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL builder");
        return -1;
    }

    if (builder->has_error) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG, "%s", builder->error_message);
        return -1;
    }

    /* Validate report-specific requirements */
    if (builder->config.report == SPX_CONFIG_REPORT_FLAT_PROFILE) {
        if (builder->config.fp_focus >= SPX_METRIC_COUNT) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG, "Invalid focus metric");
            return -1;
        }
        if (builder->config.fp_limit == 0) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG, "FP limit must be > 0");
            return -1;
        }
    }

    /* Ensure at least one metric is enabled */
    int has_metric = 0;
    SPX_METRIC_FOREACH(i, {
        if (builder->config.enabled_metrics[i]) {
            has_metric = 1;
        }
    });

    if (builder->config.enabled && !has_metric) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG, "No metrics enabled");
        return -1;
    }

    return 0;
}

int spx_config_builder_build(
    spx_config_builder_t *builder,
    spx_config_t *config,
    spx_error_t *error)
{
    if (!config) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL config output");
        return -1;
    }

    if (spx_config_builder_validate(builder, error) != 0) {
        return -1;
    }

    /* Apply fixes based on report type */
    if (builder->config.report == SPX_CONFIG_REPORT_FULL) {
        builder->config.enabled_metrics[SPX_METRIC_WALL_TIME] = 1;
        builder->config.enabled_metrics[SPX_METRIC_ZE_MEMORY_USAGE] = 1;
    }

    if (builder->config.report == SPX_CONFIG_REPORT_FLAT_PROFILE) {
        builder->config.enabled_metrics[builder->config.fp_focus] = 1;
    }

    /* Copy configuration to output */
    memcpy(config, &builder->config, sizeof(*config));

    return 0;
}

int spx_config_builder_has_errors(const spx_config_builder_t *builder)
{
    return builder ? builder->has_error : 1;
}

const char *spx_config_builder_get_error(const spx_config_builder_t *builder)
{
    if (!builder || !builder->has_error) {
        return NULL;
    }
    return builder->error_message;
}

/*============================================================================
 * Preset Configurations
 *============================================================================*/

spx_config_builder_t *spx_config_builder_preset_cli(spx_config_builder_t *builder)
{
    if (!builder) {
        return NULL;
    }

    spx_config_builder_reset(builder);

    builder->config.enabled = 1;
    builder->config.auto_start = 1;
    builder->config.report = SPX_CONFIG_REPORT_FLAT_PROFILE;
    builder->config.fp_color = 1;
    builder->config.fp_limit = 20;

    return builder;
}

spx_config_builder_t *spx_config_builder_preset_http(spx_config_builder_t *builder)
{
    if (!builder) {
        return NULL;
    }

    spx_config_builder_reset(builder);

    builder->config.enabled = 1;
    builder->config.auto_start = 1;
    builder->config.report = SPX_CONFIG_REPORT_FULL;

    return builder;
}

spx_config_builder_t *spx_config_builder_preset_minimal(spx_config_builder_t *builder)
{
    if (!builder) {
        return NULL;
    }

    spx_config_builder_reset(builder);

    builder->config.enabled = 1;
    builder->config.auto_start = 1;
    builder->config.builtins = 0;

    /* Only wall time */
    SPX_METRIC_FOREACH(i, {
        builder->config.enabled_metrics[i] = 0;
    });
    builder->config.enabled_metrics[SPX_METRIC_WALL_TIME] = 1;

    return builder;
}

spx_config_builder_t *spx_config_builder_preset_full(spx_config_builder_t *builder)
{
    if (!builder) {
        return NULL;
    }

    spx_config_builder_reset(builder);

    builder->config.enabled = 1;
    builder->config.auto_start = 1;
    builder->config.builtins = 1;
    builder->config.report = SPX_CONFIG_REPORT_FULL;

    /* Enable all metrics */
    SPX_METRIC_FOREACH(i, {
        builder->config.enabled_metrics[i] = 1;
    });

    return builder;
}
