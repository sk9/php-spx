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

#ifndef SPX_METRIC_REGISTRY_H
#define SPX_METRIC_REGISTRY_H

#include "spx_fmt.h"
#include <stddef.h>

/**
 * spx_metric_registry - Plugin-based metric registration system
 *
 * This module implements the Open/Closed Principle for metric management.
 * New metrics can be added by registering plugins without modifying existing code.
 *
 * Benefits:
 * - Open for extension: Add new metrics via plugin registration
 * - Closed for modification: Existing metric code unchanged
 * - Runtime flexibility: Metrics can be registered dynamically
 * - Clean separation: Metric implementation decoupled from registry
 *
 * Usage:
 *   // Register a new metric
 *   spx_metric_plugin_t my_metric = {
 *       .key = "mm",
 *       .short_name = "My Metric",
 *       .name = "My Custom Metric",
 *       .type = SPX_FMT_QUANTITY,
 *       .releasable = 0,
 *       .handler = my_metric_handler
 *   };
 *   spx_metric_id_t id = spx_metric_registry_register(&my_metric);
 *
 *   // Lookup metric by key
 *   spx_metric_id_t id = spx_metric_registry_get_by_key("mm");
 */

/* Metric ID type - replaces enum spx_metric_t for dynamic metrics */
typedef size_t spx_metric_id_t;

#define SPX_METRIC_ID_NONE ((spx_metric_id_t)-1)

/* Metric plugin definition */
typedef struct {
    const char *key;              /* Short key (e.g., "wt", "ct") */
    const char *short_name;       /* Display name */
    const char *name;             /* Full descriptive name */
    spx_fmt_value_type_t type;    /* Value format type */
    int releasable;               /* Whether metric can be released */
    size_t (*handler)(void);      /* Function to collect metric value */
} spx_metric_plugin_t;

/* Metric info - returned by registry for queries */
typedef struct {
    spx_metric_id_t id;
    const char *key;
    const char *short_name;
    const char *name;
    spx_fmt_value_type_t type;
    int releasable;
    size_t (*handler)(void);
} spx_metric_info_ex_t;

/**
 * Initialize the metric registry.
 * Must be called before any other registry functions.
 */
void spx_metric_registry_init(void);

/**
 * Shutdown the metric registry and free all resources.
 */
void spx_metric_registry_shutdown(void);

/**
 * Register a new metric plugin.
 *
 * @param plugin Metric plugin definition (copied internally)
 * @return Metric ID, or SPX_METRIC_ID_NONE on failure
 */
spx_metric_id_t spx_metric_registry_register(const spx_metric_plugin_t *plugin);

/**
 * Get metric ID by key.
 *
 * @param key Metric key (e.g., "wt", "ct")
 * @return Metric ID, or SPX_METRIC_ID_NONE if not found
 */
spx_metric_id_t spx_metric_registry_get_by_key(const char *key);

/**
 * Get metric info by ID.
 *
 * @param id Metric ID
 * @return Metric info, or NULL if ID invalid
 */
const spx_metric_info_ex_t *spx_metric_registry_get_info(spx_metric_id_t id);

/**
 * Get total number of registered metrics.
 *
 * @return Metric count
 */
size_t spx_metric_registry_get_count(void);

/**
 * Iterate over all registered metrics.
 * Callback is called once for each metric.
 *
 * @param callback Function to call for each metric
 * @param user_data User data passed to callback
 */
void spx_metric_registry_foreach(void (*callback)(const spx_metric_info_ex_t *, void *),
                                  void *user_data);

/**
 * Check if a metric ID is valid.
 *
 * @param id Metric ID to check
 * @return 1 if valid, 0 otherwise
 */
int spx_metric_registry_is_valid(spx_metric_id_t id);

#endif /* SPX_METRIC_REGISTRY_H */
