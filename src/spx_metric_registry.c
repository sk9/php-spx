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

#include "spx_metric_registry.h"
#include "spx_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 32
#define MAX_KEY_LENGTH 64

typedef struct {
    spx_metric_info_ex_t info;
    char key_storage[MAX_KEY_LENGTH];
    char short_name_storage[256];
    char name_storage[256];
} metric_entry_t;

static struct {
    metric_entry_t *metrics;
    size_t count;
    size_t capacity;
    int initialized;
} registry = {NULL, 0, 0, 0};

void spx_metric_registry_init(void)
{
    if (registry.initialized) {
        spx_metric_registry_shutdown();
    }

    registry.metrics = malloc(INITIAL_CAPACITY * sizeof(metric_entry_t));
    if (!registry.metrics) {
        spx_error_set(SPX_ERR_OUT_OF_MEMORY, "Failed to allocate metric registry");
        return;
    }

    registry.count = 0;
    registry.capacity = INITIAL_CAPACITY;
    registry.initialized = 1;
}

void spx_metric_registry_shutdown(void)
{
    if (!registry.initialized) {
        return;
    }

    free(registry.metrics);
    registry.metrics = NULL;
    registry.count = 0;
    registry.capacity = 0;
    registry.initialized = 0;
}

static int ensure_capacity(void)
{
    if (registry.count < registry.capacity) {
        return 1;
    }

    size_t new_capacity = registry.capacity * 2;
    metric_entry_t *new_metrics = realloc(registry.metrics, new_capacity * sizeof(metric_entry_t));
    if (!new_metrics) {
        spx_error_set(SPX_ERR_OUT_OF_MEMORY, "Failed to expand metric registry");
        return 0;
    }

    registry.metrics = new_metrics;
    registry.capacity = new_capacity;
    return 1;
}

spx_metric_id_t spx_metric_registry_register(const spx_metric_plugin_t *plugin)
{
    if (!registry.initialized) {
        spx_error_set(SPX_ERR_INTERNAL, "Registry not initialized");
        return SPX_METRIC_ID_NONE;
    }

    if (!plugin) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Plugin cannot be NULL");
        return SPX_METRIC_ID_NONE;
    }

    if (!plugin->key || !plugin->short_name || !plugin->name || !plugin->handler) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Plugin missing required fields");
        return SPX_METRIC_ID_NONE;
    }

    /* Check for duplicate key */
    {
        size_t i;
        for (i = 0; i < registry.count; i++) {
            if (strcmp(registry.metrics[i].info.key, plugin->key) == 0) {
                spx_error_set(SPX_ERR_INVALID_CONFIG, "Metric key already registered");
                return SPX_METRIC_ID_NONE;
            }
        }
    }

    /* Ensure we have space */
    if (!ensure_capacity()) {
        return SPX_METRIC_ID_NONE;
    }

    /* Allocate new entry */
    spx_metric_id_t id = registry.count;
    metric_entry_t *entry = &registry.metrics[id];

    /* Copy strings into storage */
    snprintf(entry->key_storage, MAX_KEY_LENGTH, "%s", plugin->key);
    snprintf(entry->short_name_storage, sizeof(entry->short_name_storage), "%s",
             plugin->short_name);
    snprintf(entry->name_storage, sizeof(entry->name_storage), "%s", plugin->name);

    /* Set up info structure */
    entry->info.id = id;
    entry->info.key = entry->key_storage;
    entry->info.short_name = entry->short_name_storage;
    entry->info.name = entry->name_storage;
    entry->info.type = plugin->type;
    entry->info.releasable = plugin->releasable;
    entry->info.handler = plugin->handler;

    registry.count++;
    return id;
}

spx_metric_id_t spx_metric_registry_get_by_key(const char *key)
{
    size_t i;
    if (!registry.initialized || !key) {
        return SPX_METRIC_ID_NONE;
    }

    for (i = 0; i < registry.count; i++) {
        if (strcmp(registry.metrics[i].info.key, key) == 0) {
            return registry.metrics[i].info.id;
        }
    }

    return SPX_METRIC_ID_NONE;
}

const spx_metric_info_ex_t *spx_metric_registry_get_info(spx_metric_id_t id)
{
    if (!registry.initialized || id >= registry.count) {
        return NULL;
    }

    return &registry.metrics[id].info;
}

size_t spx_metric_registry_get_count(void)
{
    return registry.initialized ? registry.count : 0;
}

void spx_metric_registry_foreach(void (*callback)(const spx_metric_info_ex_t *, void *),
                                 void *user_data)
{
    size_t i;
    if (!registry.initialized || !callback) {
        return;
    }

    for (i = 0; i < registry.count; i++) {
        callback(&registry.metrics[i].info, user_data);
    }
}

int spx_metric_registry_is_valid(spx_metric_id_t id)
{
    if (!registry.initialized) {
        return 0;
    }

    return id < registry.count;
}
