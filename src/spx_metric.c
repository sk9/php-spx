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


#include <string.h>
#include <stdlib.h>

#include "spx_metric.h"
#include "spx_metric_registry.h"
#include "spx_metric_builtin.h"
#include "spx_resource_stats.h"
#include "spx_thread.h"
#include "spx_php.h"

struct spx_metric_collector_t {
    int enabled_metrics[SPX_METRIC_COUNT];

    /* Optimization: track only enabled metrics to reduce iteration overhead */
    size_t enabled_count;
    spx_metric_t enabled_indices[SPX_METRIC_COUNT];

    double ref_values[SPX_METRIC_COUNT];
    double last_values[SPX_METRIC_COUNT];
    double current_fixed_noise[SPX_METRIC_COUNT];
};


static void collect_raw_values(const int * enabled_metrics, double * current_values);

/*
 * Compatibility layer: Populate spx_metric_info[] from registry
 * This ensures existing code continues to work without modification.
 */
static spx_metric_info_t metric_info_compat[SPX_METRIC_COUNT];
const spx_metric_info_t spx_metric_info[SPX_METRIC_COUNT] = {0};  /* Will be copied from compat */

static int metrics_initialized = 0;

void spx_metric_init(void)
{
    if (metrics_initialized) {
        return;
    }

    /* Initialize registry */
    spx_metric_registry_init();

    /* Register all built-in metrics */
    spx_metric_builtin_register_all();

    /* Populate compatibility array from registry */
    SPX_METRIC_FOREACH(i, {
        const spx_metric_info_ex_t *info = spx_metric_builtin_get_info(i);
        if (info) {
            metric_info_compat[i].key = info->key;
            metric_info_compat[i].short_name = info->short_name;
            metric_info_compat[i].name = info->name;
            metric_info_compat[i].type = info->type;
            metric_info_compat[i].releasable = info->releasable;
            metric_info_compat[i].handler = info->handler;
        }
    });

    /* Copy to const array (hack for compatibility) */
    memcpy((void*)spx_metric_info, metric_info_compat, sizeof(spx_metric_info));

    metrics_initialized = 1;
}

void spx_metric_shutdown(void)
{
    if (!metrics_initialized) {
        return;
    }

    spx_metric_registry_shutdown();
    metrics_initialized = 0;
}

spx_metric_t spx_metric_get_by_key(const char * key)
{
    if (!metrics_initialized) {
        return SPX_METRIC_NONE;
    }

    /* Use registry for lookup */
    spx_metric_id_t id = spx_metric_registry_get_by_key(key);
    if (id == SPX_METRIC_ID_NONE) {
        return SPX_METRIC_NONE;
    }

    /* Find corresponding enum value */
    SPX_METRIC_FOREACH(i, {
        spx_metric_id_t enum_id = spx_metric_builtin_get_registry_id(i);
        if (enum_id == id) {
            return i;
        }
    });

    return SPX_METRIC_NONE;
}

spx_metric_collector_t * spx_metric_collector_create(const int * enabled_metrics)
{
    spx_metric_collector_t * collector = malloc(sizeof(*collector));
    if (!collector) {
        return NULL;
    }

    collect_raw_values(enabled_metrics, collector->last_values);

    /* Build index of enabled metrics for fast iteration */
    collector->enabled_count = 0;

    SPX_METRIC_FOREACH(i, {
        collector->enabled_metrics[i] = enabled_metrics[i];
        collector->ref_values[i] = collector->last_values[i];
        collector->current_fixed_noise[i] = 0;

        if (enabled_metrics[i]) {
            collector->enabled_indices[collector->enabled_count++] = i;
        }
    });

    return collector;
}

void spx_metric_collector_destroy(spx_metric_collector_t * collector)
{
    free(collector);
}

void spx_metric_collector_collect(spx_metric_collector_t * collector, double * values)
{
    double current_values[SPX_METRIC_COUNT];

    collect_raw_values(collector->enabled_metrics, current_values);

    /*
     *  This branch is required to fix cpu / wall time inconsistency (cpu > wall time within a single thread).
     */
    if (
        collector->enabled_metrics[SPX_METRIC_WALL_TIME] &&
        collector->enabled_metrics[SPX_METRIC_CPU_TIME]
    ) {
        const double ct_surplus =
            (current_values[SPX_METRIC_CPU_TIME] - collector->last_values[SPX_METRIC_CPU_TIME])
            - (current_values[SPX_METRIC_WALL_TIME] - collector->last_values[SPX_METRIC_WALL_TIME])
        ;

        if (ct_surplus > 0) {
            collector->ref_values[SPX_METRIC_CPU_TIME] += ct_surplus;
            collector->ref_values[SPX_METRIC_IDLE_TIME] -= ct_surplus;
        }
    }

    SPX_METRIC_FOREACH(i, {
        if (!collector->enabled_metrics[i]) {
            continue;
        }

        if (!spx_metric_info[i].releasable) {
            const double diff = current_values[i] - collector->last_values[i];
            if (collector->current_fixed_noise[i] > diff) {
                collector->current_fixed_noise[i] = diff;
            }
        }

        collector->ref_values[i] += collector->current_fixed_noise[i];
        collector->current_fixed_noise[i] = 0;

        collector->last_values[i] = current_values[i];
        values[i] = collector->last_values[i] - collector->ref_values[i];
    });
}

void spx_metric_collector_noise_barrier(spx_metric_collector_t * collector)
{
    double current_values[SPX_METRIC_COUNT];
    collect_raw_values(collector->enabled_metrics, current_values);

    /* Optimization: Only update noise for enabled metrics */
    for (size_t i = 0; i < collector->enabled_count; i++) {
        spx_metric_t idx = collector->enabled_indices[i];
        collector->current_fixed_noise[idx] += current_values[idx] - collector->last_values[idx];
    }
}

void spx_metric_collector_add_fixed_noise(spx_metric_collector_t * collector, const double * noise)
{
    /* Optimization: Only add noise for enabled metrics */
    for (size_t i = 0; i < collector->enabled_count; i++) {
        spx_metric_t idx = collector->enabled_indices[i];
        collector->current_fixed_noise[idx] += noise[idx];
    }
}

/* Thread-local storage for memoized metric values */
static SPX_THREAD_TLS struct {
    int memoized;
    size_t value;
} memoized_metric_values[SPX_METRIC_COUNT];

static size_t memoized_metric_value(spx_metric_t metric)
{
    if (!memoized_metric_values[metric].memoized) {
        memoized_metric_values[metric].value = spx_metric_info[metric].handler();
        memoized_metric_values[metric].memoized = 1;
    }

    return memoized_metric_values[metric].value;
}

static void collect_raw_values(const int * enabled_metrics, double * current_values)
{
    /* Optimization: Only reset memoization for potentially used metrics */
    SPX_METRIC_FOREACH(i, {
        if (enabled_metrics[i]) {
            memoized_metric_values[i].memoized = 0;
        }
    });

    SPX_METRIC_FOREACH(i, {
        if (!enabled_metrics[i]) {
            current_values[i] = 0;
            continue;
        }

        current_values[i] = memoized_metric_value(i);
    });
}
