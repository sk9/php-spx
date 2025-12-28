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
 * @file spx_reporter_factory.h
 * @brief Unified reporter factory for creating profiler reporters
 *
 * This module implements the Factory Pattern to provide a single entry point
 * for creating different types of profiler reporters. It promotes:
 *
 * - Single Responsibility: Factory handles creation, reporters handle reporting
 * - Open/Closed: New reporter types can be added without modifying client code
 * - Dependency Inversion: Clients depend on abstractions, not concrete types
 *
 * Usage:
 * @code
 * // Create reporter via configuration
 * spx_reporter_config_t config = {0};
 * config.type = SPX_REPORTER_FULL;
 * config.full.data_dir = "/tmp/spx";
 * spx_profiler_reporter_t *reporter = spx_reporter_create(&config, &error);
 *
 * // Or use convenience functions
 * spx_profiler_reporter_t *reporter = spx_reporter_create_full("/tmp/spx", &error);
 * @endcode
 */

#ifndef SPX_REPORTER_FACTORY_H_DEFINED
#define SPX_REPORTER_FACTORY_H_DEFINED

#include <stddef.h>

#include "spx_metric.h"
#include "spx_profiler.h"
#include "infrastructure/spx_error.h"

/*============================================================================
 * Reporter Types
 *============================================================================*/

/**
 * @brief Enumeration of available reporter types
 */
typedef enum {
    SPX_REPORTER_FULL,         /**< Full JSON report with call graph */
    SPX_REPORTER_FLAT_PROFILE, /**< Flat profile summary */
    SPX_REPORTER_TRACE,        /**< Call trace output */
    SPX_REPORTER_NULL,         /**< No-op reporter (for testing) */
    SPX_REPORTER_COUNT         /**< Number of reporter types */
} spx_reporter_type_t;

/*============================================================================
 * Reporter-Specific Configuration
 *============================================================================*/

/**
 * @brief Configuration for full reporter
 */
typedef struct {
    const char *data_dir; /**< Directory for storing reports */
} spx_reporter_full_config_t;

/**
 * @brief Configuration for flat profile reporter
 */
typedef struct {
    spx_metric_t focus;    /**< Primary metric to focus on */
    int show_inclusive;    /**< Show inclusive metrics */
    int show_relative;     /**< Show relative percentages */
    size_t limit;          /**< Maximum functions to display */
    int live_mode;         /**< Enable live updating */
    int use_color;         /**< Enable ANSI colors */
} spx_reporter_fp_config_t;

/**
 * @brief Configuration for trace reporter
 */
typedef struct {
    const char *file_name; /**< Output file path (NULL for stdout) */
    int safe_mode;         /**< Enable buffered/safe output */
} spx_reporter_trace_config_t;

/**
 * @brief Unified reporter configuration
 */
typedef struct {
    spx_reporter_type_t type;

    union {
        spx_reporter_full_config_t full;
        spx_reporter_fp_config_t fp;
        spx_reporter_trace_config_t trace;
    };
} spx_reporter_config_t;

/*============================================================================
 * Reporter Information
 *============================================================================*/

/**
 * @brief Information about a reporter type
 */
typedef struct {
    spx_reporter_type_t type;
    const char *name;        /**< Human-readable name */
    const char *description; /**< Brief description */
    int requires_file;       /**< Requires file system access */
    int requires_terminal;   /**< Requires terminal for output */
    int supports_live;       /**< Supports live updating */
} spx_reporter_info_t;

/**
 * @brief Get information about a reporter type
 * @param type Reporter type
 * @return Pointer to reporter info, or NULL if invalid type
 */
const spx_reporter_info_t *spx_reporter_get_info(spx_reporter_type_t type);

/**
 * @brief Get reporter type from string name
 * @param name Reporter name (e.g., "full", "fp", "trace")
 * @return Reporter type, or SPX_REPORTER_COUNT if not found
 */
spx_reporter_type_t spx_reporter_type_from_string(const char *name);

/**
 * @brief Get string name for reporter type
 * @param type Reporter type
 * @return Reporter name string
 */
const char *spx_reporter_type_to_string(spx_reporter_type_t type);

/*============================================================================
 * Factory Functions
 *============================================================================*/

/**
 * @brief Create a reporter from configuration
 * @param config Reporter configuration
 * @param error Output error on failure
 * @return New reporter instance, or NULL on failure
 *
 * The returned reporter must be destroyed with spx_profiler_reporter_destroy()
 */
spx_profiler_reporter_t *spx_reporter_create(
    const spx_reporter_config_t *config,
    spx_error_t *error
);

/**
 * @brief Create a full reporter (convenience function)
 * @param data_dir Directory for report storage
 * @param error Output error on failure
 * @return New full reporter, or NULL on failure
 */
spx_profiler_reporter_t *spx_reporter_create_full(
    const char *data_dir,
    spx_error_t *error
);

/**
 * @brief Create a flat profile reporter (convenience function)
 * @param focus Primary metric
 * @param show_inclusive Show inclusive metrics
 * @param show_relative Show relative percentages
 * @param limit Maximum functions to show
 * @param live_mode Enable live updating
 * @param use_color Enable ANSI colors
 * @param error Output error on failure
 * @return New flat profile reporter, or NULL on failure
 */
spx_profiler_reporter_t *spx_reporter_create_fp(
    spx_metric_t focus,
    int show_inclusive,
    int show_relative,
    size_t limit,
    int live_mode,
    int use_color,
    spx_error_t *error
);

/**
 * @brief Create a trace reporter (convenience function)
 * @param file_name Output file (NULL for stdout)
 * @param safe_mode Enable safe/buffered output
 * @param error Output error on failure
 * @return New trace reporter, or NULL on failure
 */
spx_profiler_reporter_t *spx_reporter_create_trace(
    const char *file_name,
    int safe_mode,
    spx_error_t *error
);

/**
 * @brief Create a null reporter (for testing)
 * @param error Output error on failure (never set for null reporter)
 * @return New null reporter
 *
 * The null reporter accepts all events but produces no output.
 * Useful for benchmarking profiler overhead.
 */
spx_profiler_reporter_t *spx_reporter_create_null(spx_error_t *error);

/*============================================================================
 * Reporter Configuration Helpers
 *============================================================================*/

/**
 * @brief Initialize configuration with default values
 * @param config Configuration to initialize
 * @param type Reporter type
 */
void spx_reporter_config_init(spx_reporter_config_t *config, spx_reporter_type_t type);

/**
 * @brief Validate reporter configuration
 * @param config Configuration to validate
 * @param error Output validation error
 * @return 0 if valid, -1 if invalid
 */
int spx_reporter_config_validate(
    const spx_reporter_config_t *config,
    spx_error_t *error
);

/*============================================================================
 * Reporter Extension Interface
 *============================================================================*/

/**
 * @brief Reporter factory function type
 *
 * Used for registering custom reporter types.
 */
typedef spx_profiler_reporter_t *(*spx_reporter_factory_func_t)(
    const void *config,
    spx_error_t *error
);

/**
 * @brief Register a custom reporter factory
 * @param type Reporter type identifier (use values >= SPX_REPORTER_COUNT)
 * @param info Reporter information
 * @param factory Factory function
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 *
 * This allows extending SPX with custom reporter implementations.
 */
int spx_reporter_register(
    spx_reporter_type_t type,
    const spx_reporter_info_t *info,
    spx_reporter_factory_func_t factory,
    spx_error_t *error
);

#endif /* SPX_REPORTER_FACTORY_H_DEFINED */
