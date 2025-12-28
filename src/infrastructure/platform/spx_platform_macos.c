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
 * @file spx_platform_macos.c
 * @brief macOS-specific platform implementation
 */

#if defined(__APPLE__) && defined(__MACH__)

#include <errno.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "spx_platform.h"
#include "../spx_error.h"

/*============================================================================
 * Thread-Local Context
 *============================================================================*/

#ifndef SPX_THREAD_TLS
#ifdef ZTS
#define SPX_THREAD_TLS __thread
#else
#define SPX_THREAD_TLS
#endif
#endif

static SPX_THREAD_TLS struct {
    int initialized;
    mach_timebase_info_data_t timebase_info;
} macos_ctx = {0, {0, 0}};

/*============================================================================
 * Time Provider Implementation
 *============================================================================*/

static size_t macos_get_wall_time_ns(void)
{
    uint64_t time = mach_absolute_time();

    if (macos_ctx.timebase_info.denom == 0) {
        mach_timebase_info(&macos_ctx.timebase_info);
    }

    return time * macos_ctx.timebase_info.numer / macos_ctx.timebase_info.denom;
}

static size_t macos_get_cpu_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return (size_t) ts.tv_sec * 1000000000ULL + (size_t) ts.tv_nsec;
}

static size_t macos_get_timestamp(void)
{
    return mach_absolute_time();
}

static size_t macos_timestamp_to_ns(size_t start, size_t end)
{
    if (macos_ctx.timebase_info.denom == 0) {
        mach_timebase_info(&macos_ctx.timebase_info);
    }

    uint64_t elapsed = end - start;
    return elapsed * macos_ctx.timebase_info.numer / macos_ctx.timebase_info.denom;
}

static const spx_time_provider_t macos_time_provider = {.get_wall_time_ns = macos_get_wall_time_ns,
                                                        .get_cpu_time_ns = macos_get_cpu_time_ns,
                                                        .get_timestamp = macos_get_timestamp,
                                                        .timestamp_to_ns = macos_timestamp_to_ns};

/*============================================================================
 * Memory Provider Implementation
 *============================================================================*/

static int macos_memory_get_stats(spx_memory_stats_t *stats, spx_error_t *error)
{
    if (!stats) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL stats pointer");
        return -1;
    }

    memset(stats, 0, sizeof(*stats));

    task_vm_info_data_t vm_info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;

    kern_return_t kr = task_info(mach_task_self(), TASK_VM_INFO, (task_info_t) &vm_info, &count);

    if (kr != KERN_SUCCESS) {
        SPX_ERROR_SET(error, SPX_ERR_INTERNAL, "task_info failed: %s", mach_error_string(kr));
        return -1;
    }

    stats->rss = vm_info.phys_footprint;
    stats->vss = vm_info.virtual_size;
    stats->private_mem = vm_info.internal;
    stats->peak_rss = vm_info.phys_footprint; /* macOS doesn't track peak separately */

    return 0;
}

static size_t macos_memory_get_rss(void)
{
    task_vm_info_data_t vm_info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;

    kern_return_t kr = task_info(mach_task_self(), TASK_VM_INFO, (task_info_t) &vm_info, &count);

    if (kr != KERN_SUCCESS) {
        return 0;
    }

    return vm_info.phys_footprint;
}

static const spx_memory_provider_t macos_memory_provider = {.get_stats = macos_memory_get_stats,
                                                            .get_rss = macos_memory_get_rss};

/*============================================================================
 * I/O Provider Implementation
 *
 * Note: macOS doesn't provide per-process I/O statistics easily.
 * We return zeros as this feature is not available.
 *============================================================================*/

static int macos_io_get_stats(spx_io_stats_t *stats, spx_error_t *error)
{
    if (!stats) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL stats pointer");
        return -1;
    }

    /* I/O stats not available on macOS without significant overhead */
    memset(stats, 0, sizeof(*stats));
    return 0;
}

static void macos_io_get_bytes(size_t *bytes_in, size_t *bytes_out)
{
    /* Not available on macOS */
    *bytes_in = 0;
    *bytes_out = 0;
}

static const spx_io_provider_t macos_io_provider = {.get_stats = macos_io_get_stats,
                                                    .get_bytes = macos_io_get_bytes};

/*============================================================================
 * Console Provider Implementation
 *============================================================================*/

static int macos_console_is_tty(void)
{
    return isatty(STDOUT_FILENO);
}

static int macos_console_supports_ansi(void)
{
    if (!macos_console_is_tty()) {
        return 0;
    }

    const char *term = getenv("TERM");
    if (!term || strcmp(term, "dumb") == 0) {
        return 0;
    }

    return 1;
}

static int macos_console_get_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return 80;
}

static int macos_console_get_height(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        return w.ws_row;
    }
    return 24;
}

static const spx_console_provider_t macos_console_provider = {
    .is_tty = macos_console_is_tty,
    .supports_ansi = macos_console_supports_ansi,
    .get_width = macos_console_get_width,
    .get_height = macos_console_get_height};

/*============================================================================
 * File System Provider Implementation
 *============================================================================*/

static int macos_fs_get_info(const char *path, spx_file_info_t *info, int follow_symlinks,
                             spx_error_t *error)
{
    if (!path || !info) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL path or info pointer");
        return -1;
    }

    struct stat st;
    int result = follow_symlinks ? stat(path, &st) : lstat(path, &st);
    if (result != 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Cannot stat '%s': %s", path, strerror(errno));
        return -1;
    }

    info->size = st.st_size;
    info->is_directory = S_ISDIR(st.st_mode);
    info->is_symlink = S_ISLNK(st.st_mode);
    info->is_regular = S_ISREG(st.st_mode);
    info->mode = st.st_mode & 0777;

    return 0;
}

static int macos_fs_mkdir(const char *path, unsigned int mode, spx_error_t *error)
{
    if (!path) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL path");
        return -1;
    }

    if (mkdir(path, mode) != 0 && errno != EEXIST) {
        SPX_ERROR_SET(error, SPX_ERR_DIR_CREATE_FAILED, "Cannot create directory '%s': %s", path,
                      strerror(errno));
        return -1;
    }

    return 0;
}

static int macos_fs_realpath(const char *path, char *resolved, size_t size, spx_error_t *error)
{
    if (!path || !resolved || size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Invalid parameters");
        return -1;
    }

    char *result = realpath(path, NULL);
    if (!result) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Cannot resolve path '%s': %s", path,
                      strerror(errno));
        return -1;
    }

    size_t len = strlen(result);
    if (len >= size) {
        free(result);
        SPX_ERROR_SET(error, SPX_ERR_BUFFER_OVERFLOW, "Path too long");
        return -1;
    }

    memcpy(resolved, result, len + 1);
    free(result);
    return 0;
}

static const spx_fs_provider_t macos_fs_provider = {
    .get_info = macos_fs_get_info, .mkdir = macos_fs_mkdir, .realpath = macos_fs_realpath};

/*============================================================================
 * Platform Info and Interface
 *============================================================================*/

static const spx_platform_info_t macos_platform_info = {
    .type = SPX_PLATFORM_MACOS,
    .capabilities = SPX_PLATFORM_CAP_WALL_TIME | SPX_PLATFORM_CAP_CPU_TIME |
                    SPX_PLATFORM_CAP_MEMORY_RSS | SPX_PLATFORM_CAP_MACH_PORTS |
                    SPX_PLATFORM_CAP_TTY | SPX_PLATFORM_CAP_ANSI_COLORS,
    .name = "macOS",
    .version = NULL};

static const spx_platform_t macos_platform = {.info = &macos_platform_info,
                                              .time = &macos_time_provider,
                                              .memory = &macos_memory_provider,
                                              .io = &macos_io_provider,
                                              .console = &macos_console_provider,
                                              .fs = &macos_fs_provider};

static int platform_initialized = 0;

/*============================================================================
 * Platform Lifecycle
 *============================================================================*/

int spx_platform_init(spx_error_t *error)
{
    (void) error; /* Unused on macOS */

    if (platform_initialized) {
        return 0;
    }

    /* Initialize timebase info */
    mach_timebase_info(&macos_ctx.timebase_info);
    macos_ctx.initialized = 1;
    platform_initialized = 1;

    return 0;
}

void spx_platform_shutdown(void)
{
    if (!platform_initialized) {
        return;
    }

    macos_ctx.initialized = 0;
    platform_initialized = 0;
}

const spx_platform_t *spx_platform_get(void)
{
    return platform_initialized ? &macos_platform : NULL;
}

spx_platform_type_t spx_platform_get_type(void)
{
    return SPX_PLATFORM_MACOS;
}

int spx_platform_has_capability(spx_platform_caps_t cap)
{
    return (macos_platform_info.capabilities & cap) != 0;
}

#endif /* defined(__APPLE__) && defined(__MACH__) */
