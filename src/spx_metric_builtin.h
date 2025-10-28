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

#ifndef SPX_METRIC_BUILTIN_H
#define SPX_METRIC_BUILTIN_H

#include "spx_metric.h"
#include "spx_metric_registry.h"

/**
 * spx_metric_builtin - Built-in metric registration
 *
 * This module registers all built-in SPX metrics as plugins in the metric registry.
 * It provides a bridge between the old enum-based system and the new plugin-based system.
 *
 * Usage:
 *   // Call once during module initialization
 *   spx_metric_builtin_register_all();
 *
 *   // Old code continues to work
 *   spx_metric_t metric = SPX_METRIC_WALL_TIME;
 *   const spx_metric_info_t *info = spx_metric_get_info(metric);
 *
 *   // New code can use registry directly
 *   spx_metric_id_t id = spx_metric_registry_get_by_key("wt");
 */

/**
 * Register all built-in metrics as plugins.
 * Should be called once during module initialization, after spx_metric_registry_init().
 *
 * Registers 22 built-in metrics:
 * - Time: wall time, CPU time, idle time
 * - Memory: ZE memory usage, allocations, frees, own RSS
 * - GC: runs, root buffer, collected
 * - Code: included files/lines, classes, functions, opcodes, objects, errors
 * - I/O: total bytes, read bytes, write bytes
 */
void spx_metric_builtin_register_all(void);

/**
 * Get registry ID for a built-in metric enum value.
 * Provides compatibility layer between enum and registry.
 *
 * @param metric Built-in metric enum value
 * @return Registry ID, or SPX_METRIC_ID_NONE if invalid
 */
spx_metric_id_t spx_metric_builtin_get_registry_id(spx_metric_t metric);

/**
 * Get metric info for a built-in metric enum value.
 * Convenience function that queries the registry.
 *
 * @param metric Built-in metric enum value
 * @return Metric info, or NULL if invalid
 */
const spx_metric_info_ex_t *spx_metric_builtin_get_info(spx_metric_t metric);

#endif /* SPX_METRIC_BUILTIN_H */
