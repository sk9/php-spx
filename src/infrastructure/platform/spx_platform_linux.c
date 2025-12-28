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
 * @file spx_platform_linux.c
 * @brief Linux-specific platform implementation
 */

#ifdef __linux__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "spx_platform.h"
#include "../spx_error.h"

/*============================================================================
 * Thread-Local Context for Procfs File Descriptors
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
    int status_fd;       /* /proc/self/status */
    int io_fd;           /* /proc/self/task/<tid>/io */
    size_t io_read_noise; /* Track I/O from reading procfs */
    char buf[2048];      /* Buffer for procfs reads */
} linux_ctx = {0, -1, -1, 0, {0}};

/*============================================================================
 * Time Provider Implementation
 *============================================================================*/

#define TIMESPEC_TO_NS(ts) ((size_t)(ts).tv_sec * 1000000000ULL + (size_t)(ts).tv_nsec)

static size_t linux_get_wall_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return TIMESPEC_TO_NS(ts);
}

static size_t linux_get_cpu_time_ns(void)
{
    struct timespec ts;
    /* Linux CLOCK_PROCESS_CPUTIME_ID works correctly across CPUs */
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return TIMESPEC_TO_NS(ts);
}

static size_t linux_get_timestamp(void)
{
    return linux_get_wall_time_ns();
}

static size_t linux_timestamp_to_ns(size_t start, size_t end)
{
    return end - start;
}

static const spx_time_provider_t linux_time_provider = {
    .get_wall_time_ns = linux_get_wall_time_ns,
    .get_cpu_time_ns = linux_get_cpu_time_ns,
    .get_timestamp = linux_get_timestamp,
    .timestamp_to_ns = linux_timestamp_to_ns
};

/*============================================================================
 * Memory Provider Implementation
 *============================================================================*/

/**
 * Parse value from /proc/self/status for a given key
 * Returns value in bytes (converts from KB if needed)
 */
static size_t parse_status_value(const char *buf, size_t buf_len, const char *key)
{
    size_t key_len = strlen(key);
    size_t i = 0;
    size_t line_start = 0;

    while (i < buf_len) {
        if (buf[i] == ':') {
            /* Check if this line starts with our key */
            if (i - line_start == key_len &&
                strncmp(buf + line_start, key, key_len) == 0) {
                /* Parse the numeric value after the colon */
                size_t value = 0;
                i++; /* Skip colon */
                while (i < buf_len && buf[i] != '\n') {
                    if (buf[i] >= '0' && buf[i] <= '9') {
                        value = value * 10 + (buf[i] - '0');
                    }
                    i++;
                }
                /* Values are in KB, convert to bytes */
                return value * 1024;
            }
        }
        if (buf[i] == '\n') {
            line_start = i + 1;
        }
        i++;
    }
    return 0;
}

static int linux_memory_get_stats(spx_memory_stats_t *stats, spx_error_t *error)
{
    if (!stats) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL stats pointer");
        return -1;
    }

    memset(stats, 0, sizeof(*stats));

    if (linux_ctx.status_fd < 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "procfs status file not open");
        return -1;
    }

    lseek(linux_ctx.status_fd, 0, SEEK_SET);
    ssize_t bytes_read = read(linux_ctx.status_fd, linux_ctx.buf, sizeof(linux_ctx.buf) - 1);
    if (bytes_read < 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_READ_FAILED, "Failed to read status: %s", strerror(errno));
        return -1;
    }
    linux_ctx.buf[bytes_read] = '\0';
    linux_ctx.io_read_noise += bytes_read;

    stats->rss = parse_status_value(linux_ctx.buf, bytes_read, "VmRSS");
    stats->vss = parse_status_value(linux_ctx.buf, bytes_read, "VmSize");
    stats->peak_rss = parse_status_value(linux_ctx.buf, bytes_read, "VmHWM");
    stats->private_mem = parse_status_value(linux_ctx.buf, bytes_read, "RssAnon");

    return 0;
}

static size_t linux_memory_get_rss(void)
{
    if (linux_ctx.status_fd < 0) {
        return 0;
    }

    lseek(linux_ctx.status_fd, 0, SEEK_SET);
    ssize_t bytes_read = read(linux_ctx.status_fd, linux_ctx.buf, sizeof(linux_ctx.buf) - 1);
    if (bytes_read < 0) {
        return 0;
    }
    linux_ctx.buf[bytes_read] = '\0';
    linux_ctx.io_read_noise += bytes_read;

    /* Parse RssAnon (private anonymous memory) for more accurate profiling */
    return parse_status_value(linux_ctx.buf, bytes_read, "RssAnon");
}

static const spx_memory_provider_t linux_memory_provider = {
    .get_stats = linux_memory_get_stats,
    .get_rss = linux_memory_get_rss
};

/*============================================================================
 * I/O Provider Implementation
 *============================================================================*/

static int linux_io_get_stats(spx_io_stats_t *stats, spx_error_t *error)
{
    if (!stats) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL stats pointer");
        return -1;
    }

    memset(stats, 0, sizeof(*stats));

    if (linux_ctx.io_fd < 0) {
        /* I/O stats may not be available (permissions, kernel config) */
        return 0;
    }

    char buf[128];
    lseek(linux_ctx.io_fd, 0, SEEK_SET);
    ssize_t bytes_read = read(linux_ctx.io_fd, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_READ_FAILED, "Failed to read I/O stats: %s", strerror(errno));
        return -1;
    }
    buf[bytes_read] = '\0';
    linux_ctx.io_read_noise += bytes_read;

    /* Parse: rchar: <value>\nwchar: <value>\n... */
    size_t *targets[] = {&stats->bytes_read, &stats->bytes_written,
                         &stats->read_ops, &stats->write_ops};
    int target_idx = 0;
    const char *p = buf;

    while (*p && target_idx < 4) {
        if (*p >= '0' && *p <= '9') {
            size_t val = 0;
            while (*p >= '0' && *p <= '9') {
                val = val * 10 + (*p - '0');
                p++;
            }
            *targets[target_idx++] = val;
        }
        if (*p == '\n') {
            target_idx++; /* Skip to next value */
        }
        p++;
    }

    /* Subtract noise from our own reads */
    if (stats->bytes_read >= linux_ctx.io_read_noise) {
        stats->bytes_read -= linux_ctx.io_read_noise;
    }

    return 0;
}

static void linux_io_get_bytes(size_t *bytes_in, size_t *bytes_out)
{
    *bytes_in = 0;
    *bytes_out = 0;

    if (linux_ctx.io_fd < 0) {
        return;
    }

    char buf[64];
    lseek(linux_ctx.io_fd, 0, SEEK_SET);
    ssize_t bytes_read = read(linux_ctx.io_fd, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        return;
    }
    linux_ctx.io_read_noise += bytes_read;

    /* Parse just rchar and wchar (first two lines) */
    const char *p = buf;
    size_t *cur = bytes_in;

    while (*p && cur != NULL) {
        if (*p >= '0' && *p <= '9') {
            *cur = 0;
            while (*p >= '0' && *p <= '9') {
                *cur = *cur * 10 + (*p - '0');
                p++;
            }
        }
        if (*p == '\n') {
            if (cur == bytes_in) {
                cur = bytes_out;
            } else {
                break;
            }
        }
        p++;
    }

    /* Subtract noise from our own reads */
    if (*bytes_in >= linux_ctx.io_read_noise) {
        *bytes_in -= linux_ctx.io_read_noise;
    }
}

static const spx_io_provider_t linux_io_provider = {
    .get_stats = linux_io_get_stats,
    .get_bytes = linux_io_get_bytes
};

/*============================================================================
 * Console Provider Implementation
 *============================================================================*/

static int linux_console_is_tty(void)
{
    return isatty(STDOUT_FILENO);
}

static int linux_console_supports_ansi(void)
{
    if (!linux_console_is_tty()) {
        return 0;
    }

    /* Check TERM environment variable */
    const char *term = getenv("TERM");
    if (!term || strcmp(term, "dumb") == 0) {
        return 0;
    }

    return 1;
}

static int linux_console_get_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return 80; /* Default */
}

static int linux_console_get_height(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        return w.ws_row;
    }
    return 24; /* Default */
}

static const spx_console_provider_t linux_console_provider = {
    .is_tty = linux_console_is_tty,
    .supports_ansi = linux_console_supports_ansi,
    .get_width = linux_console_get_width,
    .get_height = linux_console_get_height
};

/*============================================================================
 * File System Provider Implementation
 *============================================================================*/

static int linux_fs_get_info(const char *path, spx_file_info_t *info,
                             int follow_symlinks, spx_error_t *error)
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

static int linux_fs_mkdir(const char *path, unsigned int mode, spx_error_t *error)
{
    if (!path) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL path");
        return -1;
    }

    if (mkdir(path, mode) != 0 && errno != EEXIST) {
        SPX_ERROR_SET(error, SPX_ERR_DIR_CREATE_FAILED,
                      "Cannot create directory '%s': %s", path, strerror(errno));
        return -1;
    }

    return 0;
}

static int linux_fs_realpath(const char *path, char *resolved,
                             size_t size, spx_error_t *error)
{
    if (!path || !resolved || size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Invalid parameters");
        return -1;
    }

    char *result = realpath(path, NULL);
    if (!result) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND,
                      "Cannot resolve path '%s': %s", path, strerror(errno));
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

static const spx_fs_provider_t linux_fs_provider = {
    .get_info = linux_fs_get_info,
    .mkdir = linux_fs_mkdir,
    .realpath = linux_fs_realpath
};

/*============================================================================
 * Platform Info and Interface
 *============================================================================*/

static const spx_platform_info_t linux_platform_info = {
    .type = SPX_PLATFORM_LINUX,
    .capabilities = SPX_PLATFORM_CAP_WALL_TIME | SPX_PLATFORM_CAP_CPU_TIME |
                    SPX_PLATFORM_CAP_MEMORY_RSS | SPX_PLATFORM_CAP_IO_STATS |
                    SPX_PLATFORM_CAP_PROCFS | SPX_PLATFORM_CAP_TTY |
                    SPX_PLATFORM_CAP_ANSI_COLORS,
    .name = "Linux",
    .version = NULL /* Could read /proc/version */
};

static const spx_platform_t linux_platform = {
    .info = &linux_platform_info,
    .time = &linux_time_provider,
    .memory = &linux_memory_provider,
    .io = &linux_io_provider,
    .console = &linux_console_provider,
    .fs = &linux_fs_provider
};

static int platform_initialized = 0;

/*============================================================================
 * Platform Lifecycle
 *============================================================================*/

int spx_platform_init(spx_error_t *error)
{
    if (platform_initialized) {
        return 0;
    }

    /* Open procfs file descriptors */
    linux_ctx.status_fd = open("/proc/self/status", O_RDONLY);
    if (linux_ctx.status_fd < 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND,
                      "Cannot open /proc/self/status: %s", strerror(errno));
        /* Non-fatal: continue without memory stats */
    }

    /* Open per-thread I/O stats */
    char io_path[64];
    snprintf(io_path, sizeof(io_path), "/proc/self/task/%ld/io", syscall(SYS_gettid));
    linux_ctx.io_fd = open(io_path, O_RDONLY);
    /* I/O stats may not be available; non-fatal */

    linux_ctx.io_read_noise = 0;
    linux_ctx.initialized = 1;
    platform_initialized = 1;

    return 0;
}

void spx_platform_shutdown(void)
{
    if (!platform_initialized) {
        return;
    }

    if (linux_ctx.status_fd >= 0) {
        close(linux_ctx.status_fd);
        linux_ctx.status_fd = -1;
    }

    if (linux_ctx.io_fd >= 0) {
        close(linux_ctx.io_fd);
        linux_ctx.io_fd = -1;
    }

    linux_ctx.initialized = 0;
    platform_initialized = 0;
}

const spx_platform_t *spx_platform_get(void)
{
    return platform_initialized ? &linux_platform : NULL;
}

spx_platform_type_t spx_platform_get_type(void)
{
    return SPX_PLATFORM_LINUX;
}

int spx_platform_has_capability(spx_platform_caps_t cap)
{
    return (linux_platform_info.capabilities & cap) != 0;
}

#endif /* __linux__ */
