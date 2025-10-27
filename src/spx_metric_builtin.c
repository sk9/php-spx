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

#include "spx_metric_builtin.h"
#include "spx_resource_stats.h"
#include "spx_php.h"
#include "spx_error.h"

/* Forward declarations for local metric handlers */
static size_t metric_handler_idle_time(void);
static size_t metric_handler_io_bytes(void);
static size_t metric_handler_io_r_bytes(void);
static size_t metric_handler_io_w_bytes(void);

/* Memoization for I/O metrics */
static void memoize_io_stats(void);
static size_t memoized_metric_value(spx_metric_t metric);

/* Mapping from enum to registry ID */
static spx_metric_id_t builtin_registry_ids[SPX_METRIC_COUNT];
static int builtin_registered = 0;

/* Thread-local storage for memoized values */
#include "spx_thread.h"

static SPX_THREAD_TLS struct {
    int memoized;
    size_t value;
} memoized_metric_values[SPX_METRIC_COUNT];

void spx_metric_builtin_register_all(void)
{
    if (builtin_registered) {
        return;  /* Already registered */
    }

    /* Initialize all IDs to NONE */
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        builtin_registry_ids[i] = SPX_METRIC_ID_NONE;
    }

    /* Register all built-in metrics as plugins */
    spx_metric_plugin_t metrics[] = {
        /* Time metrics */
        {
            .key = "wt",
            .short_name = "Wall time",
            .name = "Wall time",
            .type = SPX_FMT_TIME,
            .releasable = 0,
            .handler = spx_resource_stats_wall_time
        },
        {
            .key = "ct",
            .short_name = "CPU time",
            .name = "CPU time",
            .type = SPX_FMT_TIME,
            .releasable = 0,
            .handler = spx_resource_stats_cpu_time
        },
        {
            .key = "it",
            .short_name = "Idle time",
            .name = "Idle time",
            .type = SPX_FMT_TIME,
            .releasable = 0,
            .handler = metric_handler_idle_time
        },

        /* Zend Engine memory metrics */
        {
            .key = "zm",
            .short_name = "ZE memory usage",
            .name = "Zend Engine memory usage",
            .type = SPX_FMT_MEMORY,
            .releasable = 1,
            .handler = spx_php_zend_memory_usage
        },
        {
            .key = "zmac",
            .short_name = "ZE alloc count",
            .name = "Zend Engine allocation count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_memory_alloc_count
        },
        {
            .key = "zmab",
            .short_name = "ZE alloc bytes",
            .name = "Zend Engine allocated bytes",
            .type = SPX_FMT_MEMORY,
            .releasable = 0,
            .handler = spx_php_zend_memory_alloc_bytes
        },
        {
            .key = "zmfc",
            .short_name = "ZE free count",
            .name = "Zend Engine free count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_memory_free_count
        },
        {
            .key = "zmfb",
            .short_name = "ZE free bytes",
            .name = "Zend Engine freed bytes",
            .type = SPX_FMT_MEMORY,
            .releasable = 0,
            .handler = spx_php_zend_memory_free_bytes
        },

        /* Zend Engine GC metrics */
        {
            .key = "zgr",
            .short_name = "ZE GC runs",
            .name = "Zend Engine GC run count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_gc_run_count
        },
        {
            .key = "zgb",
            .short_name = "ZE GC root buffer",
            .name = "Zend Engine GC root buffer length",
            .type = SPX_FMT_QUANTITY,
            .releasable = 1,
            .handler = spx_php_zend_gc_root_buffer_length
        },
        {
            .key = "zgc",
            .short_name = "ZE GC collected",
            .name = "Zend Engine GC collected cycle count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_gc_collected_count
        },

        /* Zend Engine code metrics */
        {
            .key = "zif",
            .short_name = "ZE file count",
            .name = "Zend Engine included file count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_included_file_count
        },
        {
            .key = "zil",
            .short_name = "ZE line count",
            .name = "Zend Engine included line count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_included_line_count
        },
        {
            .key = "zuc",
            .short_name = "ZE class count",
            .name = "Zend Engine user class count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_class_count
        },
        {
            .key = "zuf",
            .short_name = "ZE func. count",
            .name = "Zend Engine user function count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_function_count
        },
        {
            .key = "zuo",
            .short_name = "ZE opcodes count",
            .name = "Zend Engine user opcode count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_opcode_count
        },
        {
            .key = "zo",
            .short_name = "ZE object count",
            .name = "Zend Engine object count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 1,
            .handler = spx_php_zend_object_count
        },
        {
            .key = "ze",
            .short_name = "ZE error count",
            .name = "Zend Engine error count",
            .type = SPX_FMT_QUANTITY,
            .releasable = 0,
            .handler = spx_php_zend_error_count
        },

        /* Process memory metric */
        {
            .key = "mor",
            .short_name = "Own RSS",
            .name = "Process's own RSS",
            .type = SPX_FMT_MEMORY,
            .releasable = 1,
            .handler = spx_resource_stats_own_rss
        },

        /* I/O metrics */
        {
            .key = "io",
            .short_name = "I/O Bytes",
            .name = "I/O Bytes (reads + writes)",
            .type = SPX_FMT_MEMORY,
            .releasable = 0,
            .handler = metric_handler_io_bytes
        },
        {
            .key = "ior",
            .short_name = "I/O Read Bytes",
            .name = "I/O Read Bytes",
            .type = SPX_FMT_MEMORY,
            .releasable = 0,
            .handler = metric_handler_io_r_bytes
        },
        {
            .key = "iow",
            .short_name = "I/O Written Bytes",
            .name = "I/O Written Bytes",
            .type = SPX_FMT_MEMORY,
            .releasable = 0,
            .handler = metric_handler_io_w_bytes
        }
    };

    /* Register each metric and store the registry ID */
    const spx_metric_t enum_order[] = {
        SPX_METRIC_WALL_TIME,
        SPX_METRIC_CPU_TIME,
        SPX_METRIC_IDLE_TIME,
        SPX_METRIC_ZE_MEMORY_USAGE,
        SPX_METRIC_ZE_MEMORY_ALLOC_COUNT,
        SPX_METRIC_ZE_MEMORY_ALLOC_BYTES,
        SPX_METRIC_ZE_MEMORY_FREE_COUNT,
        SPX_METRIC_ZE_MEMORY_FREE_BYTES,
        SPX_METRIC_ZE_GC_RUNS,
        SPX_METRIC_ZE_GC_ROOT_BUFFER,
        SPX_METRIC_ZE_GC_COLLECTED,
        SPX_METRIC_ZE_INCLUDED_FILE_COUNT,
        SPX_METRIC_ZE_INCLUDED_LINE_COUNT,
        SPX_METRIC_ZE_USER_CLASS_COUNT,
        SPX_METRIC_ZE_USER_FUNCTION_COUNT,
        SPX_METRIC_ZE_USER_OPCODE_COUNT,
        SPX_METRIC_ZE_OBJECT_COUNT,
        SPX_METRIC_ZE_ERROR_COUNT,
        SPX_METRIC_MEM_OWN_RSS,
        SPX_METRIC_IO_BYTES,
        SPX_METRIC_IO_RBYTES,
        SPX_METRIC_IO_WBYTES
    };

    for (size_t i = 0; i < 22; i++) {
        spx_metric_id_t id = spx_metric_registry_register(&metrics[i]);
        if (id == SPX_METRIC_ID_NONE) {
            /* Registration failed - this is a fatal error */
            spx_fatal_error("Failed to register built-in metric");
        }
        builtin_registry_ids[enum_order[i]] = id;
    }

    builtin_registered = 1;
}

spx_metric_id_t spx_metric_builtin_get_registry_id(spx_metric_t metric)
{
    if (metric >= SPX_METRIC_COUNT || metric == SPX_METRIC_NONE) {
        return SPX_METRIC_ID_NONE;
    }

    if (!builtin_registered) {
        spx_error_set(SPX_ERR_INTERNAL, "Built-in metrics not registered");
        return SPX_METRIC_ID_NONE;
    }

    return builtin_registry_ids[metric];
}

const spx_metric_info_ex_t *spx_metric_builtin_get_info(spx_metric_t metric)
{
    spx_metric_id_t id = spx_metric_builtin_get_registry_id(metric);
    if (id == SPX_METRIC_ID_NONE) {
        return NULL;
    }

    return spx_metric_registry_get_info(id);
}

/* ===== Local metric handlers ===== */

static size_t metric_handler_idle_time(void)
{
    return memoized_metric_value(SPX_METRIC_WALL_TIME) - memoized_metric_value(SPX_METRIC_CPU_TIME);
}

static size_t metric_handler_io_bytes(void)
{
    memoize_io_stats();
    return memoized_metric_value(SPX_METRIC_IO_RBYTES) + memoized_metric_value(SPX_METRIC_IO_WBYTES);
}

static size_t metric_handler_io_r_bytes(void)
{
    memoize_io_stats();
    return memoized_metric_value(SPX_METRIC_IO_RBYTES);
}

static size_t metric_handler_io_w_bytes(void)
{
    memoize_io_stats();
    return memoized_metric_value(SPX_METRIC_IO_WBYTES);
}

static void memoize_io_stats(void)
{
    if (memoized_metric_values[SPX_METRIC_IO_RBYTES].memoized) {
        return;
    }

    size_t r_bytes, w_bytes;
    spx_resource_stats_io(&r_bytes, &w_bytes);

    memoized_metric_values[SPX_METRIC_IO_RBYTES].memoized = 1;
    memoized_metric_values[SPX_METRIC_IO_RBYTES].value = r_bytes;

    memoized_metric_values[SPX_METRIC_IO_WBYTES].memoized = 1;
    memoized_metric_values[SPX_METRIC_IO_WBYTES].value = w_bytes;
}

static size_t memoized_metric_value(spx_metric_t metric)
{
    if (memoized_metric_values[metric].memoized) {
        return memoized_metric_values[metric].value;
    }

    /* Get the metric info from registry */
    const spx_metric_info_ex_t *info = spx_metric_builtin_get_info(metric);
    if (!info) {
        return 0;
    }

    size_t value = info->handler();

    memoized_metric_values[metric].memoized = 1;
    memoized_metric_values[metric].value = value;

    return value;
}
