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
 * @file spx_reporter_factory.c
 * @brief Implementation of the reporter factory
 */

#include <stdlib.h>
#include <string.h>

#include "spx_reporter_factory.h"
#include "spx_reporter_full.h"
#include "spx_reporter_fp.h"
#include "spx_reporter_trace.h"
#include "infrastructure/spx_error.h"

/*============================================================================
 * Reporter Information Table
 *============================================================================*/

static const spx_reporter_info_t reporter_info_table[] = {
    {
        .type = SPX_REPORTER_FULL,
        .name = "full",
        .description = "Full JSON report with call graph for web UI",
        .requires_file = 1,
        .requires_terminal = 0,
        .supports_live = 0
    },
    {
        .type = SPX_REPORTER_FLAT_PROFILE,
        .name = "fp",
        .description = "Flat profile summary for CLI",
        .requires_file = 0,
        .requires_terminal = 1,
        .supports_live = 1
    },
    {
        .type = SPX_REPORTER_TRACE,
        .name = "trace",
        .description = "Call trace output",
        .requires_file = 0,
        .requires_terminal = 0,
        .supports_live = 0
    },
    {
        .type = SPX_REPORTER_NULL,
        .name = "null",
        .description = "No-op reporter for testing and benchmarking",
        .requires_file = 0,
        .requires_terminal = 0,
        .supports_live = 0
    }
};

static const size_t REPORTER_INFO_COUNT = sizeof(reporter_info_table) / sizeof(reporter_info_table[0]);

/*============================================================================
 * Null Reporter Implementation
 *============================================================================*/

static spx_profiler_reporter_cost_t null_notify(
    spx_profiler_reporter_t *reporter,
    const spx_profiler_event_t *event)
{
    (void)reporter;
    (void)event;
    return SPX_PROFILER_REPORTER_COST_LIGHT;
}

static void null_destroy(spx_profiler_reporter_t *reporter)
{
    free(reporter);
}

/*============================================================================
 * Information Functions
 *============================================================================*/

const spx_reporter_info_t *spx_reporter_get_info(spx_reporter_type_t type)
{
    if (type >= REPORTER_INFO_COUNT) {
        return NULL;
    }
    return &reporter_info_table[type];
}

spx_reporter_type_t spx_reporter_type_from_string(const char *name)
{
    if (!name) {
        return SPX_REPORTER_COUNT;
    }

    for (size_t i = 0; i < REPORTER_INFO_COUNT; i++) {
        if (strcmp(name, reporter_info_table[i].name) == 0) {
            return reporter_info_table[i].type;
        }
    }

    /* Handle aliases */
    if (strcmp(name, "flat_profile") == 0 || strcmp(name, "flat-profile") == 0) {
        return SPX_REPORTER_FLAT_PROFILE;
    }

    return SPX_REPORTER_COUNT;
}

const char *spx_reporter_type_to_string(spx_reporter_type_t type)
{
    const spx_reporter_info_t *info = spx_reporter_get_info(type);
    return info ? info->name : "unknown";
}

/*============================================================================
 * Configuration Functions
 *============================================================================*/

void spx_reporter_config_init(spx_reporter_config_t *config, spx_reporter_type_t type)
{
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->type = type;

    switch (type) {
    case SPX_REPORTER_FULL:
        config->full.data_dir = "/tmp/spx";
        break;

    case SPX_REPORTER_FLAT_PROFILE:
        config->fp.focus = SPX_METRIC_WALL_TIME;
        config->fp.show_inclusive = 0;
        config->fp.show_relative = 0;
        config->fp.limit = 20;
        config->fp.live_mode = 0;
        config->fp.use_color = 1;
        break;

    case SPX_REPORTER_TRACE:
        config->trace.file_name = NULL;
        config->trace.safe_mode = 0;
        break;

    case SPX_REPORTER_NULL:
    default:
        break;
    }
}

int spx_reporter_config_validate(
    const spx_reporter_config_t *config,
    spx_error_t *error)
{
    if (!config) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL configuration");
        return -1;
    }

    switch (config->type) {
    case SPX_REPORTER_FULL:
        if (!config->full.data_dir || config->full.data_dir[0] == '\0') {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG,
                          "Full reporter requires data_dir");
            return -1;
        }
        break;

    case SPX_REPORTER_FLAT_PROFILE:
        if (config->fp.focus >= SPX_METRIC_COUNT) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG,
                          "Invalid focus metric: %d", config->fp.focus);
            return -1;
        }
        if (config->fp.limit == 0) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG,
                          "Flat profile limit must be > 0");
            return -1;
        }
        break;

    case SPX_REPORTER_TRACE:
        /* file_name can be NULL for stdout */
        break;

    case SPX_REPORTER_NULL:
        /* No validation needed */
        break;

    default:
        SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG,
                      "Unknown reporter type: %d", config->type);
        return -1;
    }

    return 0;
}

/*============================================================================
 * Factory Functions
 *============================================================================*/

spx_profiler_reporter_t *spx_reporter_create(
    const spx_reporter_config_t *config,
    spx_error_t *error)
{
    if (!config) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL configuration");
        return NULL;
    }

    /* Validate configuration first */
    if (spx_reporter_config_validate(config, error) != 0) {
        return NULL;
    }

    switch (config->type) {
    case SPX_REPORTER_FULL:
        return spx_reporter_full_create(config->full.data_dir);

    case SPX_REPORTER_FLAT_PROFILE:
        return spx_reporter_fp_create(
            config->fp.focus,
            config->fp.show_inclusive,
            config->fp.show_relative,
            config->fp.limit,
            config->fp.live_mode,
            config->fp.use_color
        );

    case SPX_REPORTER_TRACE:
        return spx_reporter_trace_create(
            config->trace.file_name,
            config->trace.safe_mode
        );

    case SPX_REPORTER_NULL:
        return spx_reporter_create_null(error);

    default:
        SPX_ERROR_SET(error, SPX_ERR_INVALID_CONFIG,
                      "Unknown reporter type: %d", config->type);
        return NULL;
    }
}

spx_profiler_reporter_t *spx_reporter_create_full(
    const char *data_dir,
    spx_error_t *error)
{
    spx_reporter_config_t config;
    spx_reporter_config_init(&config, SPX_REPORTER_FULL);
    config.full.data_dir = data_dir;
    return spx_reporter_create(&config, error);
}

spx_profiler_reporter_t *spx_reporter_create_fp(
    spx_metric_t focus,
    int show_inclusive,
    int show_relative,
    size_t limit,
    int live_mode,
    int use_color,
    spx_error_t *error)
{
    spx_reporter_config_t config;
    spx_reporter_config_init(&config, SPX_REPORTER_FLAT_PROFILE);
    config.fp.focus = focus;
    config.fp.show_inclusive = show_inclusive;
    config.fp.show_relative = show_relative;
    config.fp.limit = limit;
    config.fp.live_mode = live_mode;
    config.fp.use_color = use_color;
    return spx_reporter_create(&config, error);
}

spx_profiler_reporter_t *spx_reporter_create_trace(
    const char *file_name,
    int safe_mode,
    spx_error_t *error)
{
    spx_reporter_config_t config;
    spx_reporter_config_init(&config, SPX_REPORTER_TRACE);
    config.trace.file_name = file_name;
    config.trace.safe_mode = safe_mode;
    return spx_reporter_create(&config, error);
}

spx_profiler_reporter_t *spx_reporter_create_null(spx_error_t *error)
{
    (void)error;

    spx_profiler_reporter_t *reporter = malloc(sizeof(*reporter));
    if (!reporter) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_MEMORY, "Failed to allocate null reporter");
        return NULL;
    }

    reporter->notify = null_notify;
    reporter->destroy = null_destroy;

    return reporter;
}

/*============================================================================
 * Extension Registry (for future custom reporters)
 *============================================================================*/

#define MAX_CUSTOM_REPORTERS 16

static struct {
    spx_reporter_info_t info;
    spx_reporter_factory_func_t factory;
} custom_reporters[MAX_CUSTOM_REPORTERS];

static size_t custom_reporter_count = 0;

int spx_reporter_register(
    spx_reporter_type_t type,
    const spx_reporter_info_t *info,
    spx_reporter_factory_func_t factory,
    spx_error_t *error)
{
    if (type < SPX_REPORTER_COUNT) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                      "Cannot override built-in reporter type %d", type);
        return -1;
    }

    if (!info || !factory) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL info or factory");
        return -1;
    }

    if (custom_reporter_count >= MAX_CUSTOM_REPORTERS) {
        SPX_ERROR_SET(error, SPX_ERR_CAPACITY_EXCEEDED,
                      "Maximum custom reporters reached");
        return -1;
    }

    /* Check for duplicate */
    for (size_t i = 0; i < custom_reporter_count; i++) {
        if (custom_reporters[i].info.type == type) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                          "Reporter type %d already registered", type);
            return -1;
        }
    }

    custom_reporters[custom_reporter_count].info = *info;
    custom_reporters[custom_reporter_count].factory = factory;
    custom_reporter_count++;

    return 0;
}
