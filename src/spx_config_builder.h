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
 * @file spx_config_builder.h
 * @brief Configuration builder with fluent API for SPX configuration
 *
 * This module implements the Builder Pattern for constructing SPX configuration
 * objects. It provides:
 *
 * - Fluent API for clear, readable configuration
 * - Type-safe configuration setting
 * - Validation at build time
 * - Default values for all options
 *
 * Usage:
 * @code
 * spx_config_t config;
 * spx_error_t error = SPX_ERROR_INIT();
 *
 * spx_config_builder_t *builder = spx_config_builder_create();
 * spx_config_builder_enabled(builder, 1)
 *     ->auto_start(builder, 1)
 *     ->builtins(builder, 1)
 *     ->report(builder, SPX_CONFIG_REPORT_FULL)
 *     ->build(builder, &config, &error);
 *
 * spx_config_builder_destroy(builder);
 * @endcode
 */

#ifndef SPX_CONFIG_BUILDER_H_DEFINED
#define SPX_CONFIG_BUILDER_H_DEFINED

#include "spx_config.h"
#include "spx_metric.h"
#include "infrastructure/spx_error.h"

/*============================================================================
 * Configuration Builder Type
 *============================================================================*/

typedef struct spx_config_builder_t spx_config_builder_t;

/*============================================================================
 * Builder Lifecycle
 *============================================================================*/

/**
 * @brief Create a new configuration builder
 * @return New builder instance, or NULL on allocation failure
 */
spx_config_builder_t *spx_config_builder_create(void);

/**
 * @brief Destroy a configuration builder
 * @param builder Builder to destroy
 */
void spx_config_builder_destroy(spx_config_builder_t *builder);

/**
 * @brief Reset builder to default values
 * @param builder Builder to reset
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_reset(spx_config_builder_t *builder);

/*============================================================================
 * Core Configuration Setters
 *============================================================================*/

/**
 * @brief Set whether profiling is enabled
 * @param builder Configuration builder
 * @param enabled 1 for enabled, 0 for disabled
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_enabled(spx_config_builder_t *builder, int enabled);

/**
 * @brief Set authentication key
 * @param builder Configuration builder
 * @param key Authentication key (copied internally)
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_key(spx_config_builder_t *builder, const char *key);

/**
 * @brief Set auto-start behavior
 * @param builder Configuration builder
 * @param auto_start 1 to auto-start, 0 for manual start
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_auto_start(spx_config_builder_t *builder, int auto_start);

/*============================================================================
 * Profiling Configuration Setters
 *============================================================================*/

/**
 * @brief Set sampling period in microseconds
 * @param builder Configuration builder
 * @param period Sampling period (0 for no sampling)
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_sampling_period(spx_config_builder_t *builder,
                                                         size_t period);

/**
 * @brief Enable/disable built-in function tracing
 * @param builder Configuration builder
 * @param builtins 1 to trace builtins, 0 to skip
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_builtins(spx_config_builder_t *builder, int builtins);

/**
 * @brief Set maximum call depth to trace
 * @param builder Configuration builder
 * @param max_depth Maximum depth (0 for unlimited)
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_max_depth(spx_config_builder_t *builder, size_t max_depth);

/*============================================================================
 * Metric Configuration
 *============================================================================*/

/**
 * @brief Enable a specific metric
 * @param builder Configuration builder
 * @param metric Metric to enable
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_enable_metric(spx_config_builder_t *builder,
                                                       spx_metric_t metric);

/**
 * @brief Disable a specific metric
 * @param builder Configuration builder
 * @param metric Metric to disable
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_disable_metric(spx_config_builder_t *builder,
                                                        spx_metric_t metric);

/**
 * @brief Enable metrics from comma-separated string
 * @param builder Configuration builder
 * @param metrics Comma-separated metric keys (e.g., "wt,zm,zo")
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_metrics_from_string(spx_config_builder_t *builder,
                                                             const char *metrics);

/**
 * @brief Enable all available metrics
 * @param builder Configuration builder
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_enable_all_metrics(spx_config_builder_t *builder);

/**
 * @brief Disable all metrics
 * @param builder Configuration builder
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_disable_all_metrics(spx_config_builder_t *builder);

/*============================================================================
 * Report Configuration
 *============================================================================*/

/**
 * @brief Set report type
 * @param builder Configuration builder
 * @param report Report type
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_report(spx_config_builder_t *builder,
                                                spx_config_report_t report);

/**
 * @brief Set report type from string
 * @param builder Configuration builder
 * @param report_str Report type string ("full", "fp", "trace")
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_report_from_string(spx_config_builder_t *builder,
                                                            const char *report_str);

/*============================================================================
 * Flat Profile Configuration
 *============================================================================*/

/**
 * @brief Set flat profile focus metric
 * @param builder Configuration builder
 * @param focus Focus metric
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_fp_focus(spx_config_builder_t *builder,
                                                  spx_metric_t focus);

/**
 * @brief Enable/disable inclusive metrics in flat profile
 * @param builder Configuration builder
 * @param inc 1 for inclusive, 0 for exclusive
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_fp_inc(spx_config_builder_t *builder, int inc);

/**
 * @brief Enable/disable relative percentages in flat profile
 * @param builder Configuration builder
 * @param rel 1 for relative, 0 for absolute
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_fp_rel(spx_config_builder_t *builder, int rel);

/**
 * @brief Set maximum functions to show in flat profile
 * @param builder Configuration builder
 * @param limit Maximum functions (0 for unlimited)
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_fp_limit(spx_config_builder_t *builder, size_t limit);

/**
 * @brief Enable/disable live mode in flat profile
 * @param builder Configuration builder
 * @param live 1 for live mode, 0 for static
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_fp_live(spx_config_builder_t *builder, int live);

/**
 * @brief Enable/disable color output in flat profile
 * @param builder Configuration builder
 * @param color 1 for color, 0 for no color
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_fp_color(spx_config_builder_t *builder, int color);

/*============================================================================
 * Trace Configuration
 *============================================================================*/

/**
 * @brief Set trace output file
 * @param builder Configuration builder
 * @param file Output file path (NULL for stdout)
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_trace_file(spx_config_builder_t *builder,
                                                    const char *file);

/**
 * @brief Enable/disable safe mode for trace output
 * @param builder Configuration builder
 * @param safe 1 for safe mode, 0 for normal
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_trace_safe(spx_config_builder_t *builder, int safe);

/*============================================================================
 * HTTP UI Configuration
 *============================================================================*/

/**
 * @brief Set UI URI for web interface
 * @param builder Configuration builder
 * @param uri UI URI path
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_ui_uri(spx_config_builder_t *builder, const char *uri);

/*============================================================================
 * Build and Validation
 *============================================================================*/

/**
 * @brief Build the configuration object
 * @param builder Configuration builder
 * @param config Output configuration structure
 * @param error Output error on validation failure
 * @return 0 on success, -1 on validation error
 *
 * This method validates the configuration and populates the output structure.
 * The builder remains valid after calling build() and can be reused.
 */
int spx_config_builder_build(spx_config_builder_t *builder, spx_config_t *config,
                             spx_error_t *error);

/**
 * @brief Validate the current configuration
 * @param builder Configuration builder
 * @param error Output error on validation failure
 * @return 0 if valid, -1 if invalid
 *
 * Validates without building the final configuration.
 */
int spx_config_builder_validate(spx_config_builder_t *builder, spx_error_t *error);

/**
 * @brief Check if builder has any validation errors
 * @param builder Configuration builder
 * @return 1 if has errors, 0 if clean
 */
int spx_config_builder_has_errors(const spx_config_builder_t *builder);

/**
 * @brief Get the last error from the builder
 * @param builder Configuration builder
 * @return Error message, or NULL if no error
 */
const char *spx_config_builder_get_error(const spx_config_builder_t *builder);

/*============================================================================
 * Preset Configurations
 *============================================================================*/

/**
 * @brief Apply CLI preset (sensible defaults for command line)
 * @param builder Configuration builder
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_preset_cli(spx_config_builder_t *builder);

/**
 * @brief Apply HTTP preset (sensible defaults for web)
 * @param builder Configuration builder
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_preset_http(spx_config_builder_t *builder);

/**
 * @brief Apply minimal preset (minimal overhead)
 * @param builder Configuration builder
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_preset_minimal(spx_config_builder_t *builder);

/**
 * @brief Apply full preset (all metrics enabled)
 * @param builder Configuration builder
 * @return Same builder for chaining
 */
spx_config_builder_t *spx_config_builder_preset_full(spx_config_builder_t *builder);

#endif /* SPX_CONFIG_BUILDER_H_DEFINED */
