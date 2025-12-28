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
 * @file spx_platform_stub.c
 * @brief Stub platform implementation for unsupported platforms
 *
 * This provides minimal functionality for platforms that don't have
 * a full implementation. Used for:
 * - FreeBSD (basic support)
 * - Windows (partial support)
 * - Unknown platforms (graceful degradation)
 */

#if !defined(__linux__) && !(defined(__APPLE__) && defined(__MACH__))

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#endif

#include "spx_platform.h"
#include "../spx_error.h"

/*============================================================================
 * Time Provider Implementation (POSIX fallback)
 *============================================================================*/

static size_t stub_get_wall_time_ns(void)
{
#ifdef CLOCK_MONOTONIC
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (size_t)ts.tv_sec * 1000000000ULL + (size_t)ts.tv_nsec;
    }
#endif

    /* Fallback to gettimeofday */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (size_t)tv.tv_sec * 1000000000ULL + (size_t)tv.tv_usec * 1000ULL;
}

static size_t stub_get_cpu_time_ns(void)
{
#ifdef CLOCK_PROCESS_CPUTIME_ID
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == 0) {
        return (size_t)ts.tv_sec * 1000000000ULL + (size_t)ts.tv_nsec;
    }
#endif
    /* Fallback: return wall time (not ideal but functional) */
    return stub_get_wall_time_ns();
}

static size_t stub_get_timestamp(void)
{
    return stub_get_wall_time_ns();
}

static size_t stub_timestamp_to_ns(size_t start, size_t end)
{
    return end - start;
}

static const spx_time_provider_t stub_time_provider = {
    .get_wall_time_ns = stub_get_wall_time_ns,
    .get_cpu_time_ns = stub_get_cpu_time_ns,
    .get_timestamp = stub_get_timestamp,
    .timestamp_to_ns = stub_timestamp_to_ns
};

/*============================================================================
 * Memory Provider Implementation (Stub - no stats available)
 *============================================================================*/

static int stub_memory_get_stats(spx_memory_stats_t *stats, spx_error_t *error)
{
    if (!stats) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL stats pointer");
        return -1;
    }

    /* Memory stats not available on this platform */
    memset(stats, 0, sizeof(*stats));
    return 0;
}

static size_t stub_memory_get_rss(void)
{
    /* Not available */
    return 0;
}

static const spx_memory_provider_t stub_memory_provider = {
    .get_stats = stub_memory_get_stats,
    .get_rss = stub_memory_get_rss
};

/*============================================================================
 * I/O Provider Implementation (Stub - no stats available)
 *============================================================================*/

static int stub_io_get_stats(spx_io_stats_t *stats, spx_error_t *error)
{
    if (!stats) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL stats pointer");
        return -1;
    }

    memset(stats, 0, sizeof(*stats));
    return 0;
}

static void stub_io_get_bytes(size_t *bytes_in, size_t *bytes_out)
{
    *bytes_in = 0;
    *bytes_out = 0;
}

static const spx_io_provider_t stub_io_provider = {
    .get_stats = stub_io_get_stats,
    .get_bytes = stub_io_get_bytes
};

/*============================================================================
 * Console Provider Implementation
 *============================================================================*/

static int stub_console_is_tty(void)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    return GetConsoleMode(h, &mode) != 0;
#else
    return isatty(STDOUT_FILENO);
#endif
}

static int stub_console_supports_ansi(void)
{
    if (!stub_console_is_tty()) {
        return 0;
    }

#ifdef _WIN32
    /* Windows 10+ supports ANSI with ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h, &mode)) {
        return (mode & 0x0004) != 0; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    }
    return 0;
#else
    const char *term = getenv("TERM");
    if (!term || strcmp(term, "dumb") == 0) {
        return 0;
    }
    return 1;
#endif
}

static int stub_console_get_width(void)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
#endif
    return 80;
}

static int stub_console_get_height(void)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        return w.ws_row;
    }
#endif
    return 24;
}

static const spx_console_provider_t stub_console_provider = {
    .is_tty = stub_console_is_tty,
    .supports_ansi = stub_console_supports_ansi,
    .get_width = stub_console_get_width,
    .get_height = stub_console_get_height
};

/*============================================================================
 * File System Provider Implementation
 *============================================================================*/

static int stub_fs_get_info(const char *path, spx_file_info_t *info,
                            int follow_symlinks, spx_error_t *error)
{
    if (!path || !info) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL path or info pointer");
        return -1;
    }

    struct stat st;
#ifdef _WIN32
    (void)follow_symlinks;
    int result = stat(path, &st);
#else
    int result = follow_symlinks ? stat(path, &st) : lstat(path, &st);
#endif

    if (result != 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Cannot stat '%s': %s", path, strerror(errno));
        return -1;
    }

    info->size = st.st_size;
    info->is_directory = S_ISDIR(st.st_mode);
#ifndef _WIN32
    info->is_symlink = S_ISLNK(st.st_mode);
#else
    info->is_symlink = 0;
#endif
    info->is_regular = S_ISREG(st.st_mode);
    info->mode = st.st_mode & 0777;

    return 0;
}

static int stub_fs_mkdir(const char *path, unsigned int mode, spx_error_t *error)
{
    if (!path) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL path");
        return -1;
    }

#ifdef _WIN32
    (void)mode;
    if (_mkdir(path) != 0 && errno != EEXIST) {
#else
    if (mkdir(path, mode) != 0 && errno != EEXIST) {
#endif
        SPX_ERROR_SET(error, SPX_ERR_DIR_CREATE_FAILED,
                      "Cannot create directory '%s': %s", path, strerror(errno));
        return -1;
    }

    return 0;
}

static int stub_fs_realpath(const char *path, char *resolved,
                            size_t size, spx_error_t *error)
{
    if (!path || !resolved || size == 0) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Invalid parameters");
        return -1;
    }

#ifdef _WIN32
    char *result = _fullpath(NULL, path, 0);
#else
    char *result = realpath(path, NULL);
#endif

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

static const spx_fs_provider_t stub_fs_provider = {
    .get_info = stub_fs_get_info,
    .mkdir = stub_fs_mkdir,
    .realpath = stub_fs_realpath
};

/*============================================================================
 * Platform Info and Interface
 *============================================================================*/

#if defined(__FreeBSD__)
static const spx_platform_info_t stub_platform_info = {
    .type = SPX_PLATFORM_FREEBSD,
    .capabilities = SPX_PLATFORM_CAP_WALL_TIME | SPX_PLATFORM_CAP_CPU_TIME |
                    SPX_PLATFORM_CAP_TTY | SPX_PLATFORM_CAP_ANSI_COLORS |
                    SPX_PLATFORM_CAP_KQUEUE,
    .name = "FreeBSD",
    .version = NULL
};
#elif defined(_WIN32)
static const spx_platform_info_t stub_platform_info = {
    .type = SPX_PLATFORM_WINDOWS,
    .capabilities = SPX_PLATFORM_CAP_WALL_TIME | SPX_PLATFORM_CAP_CPU_TIME |
                    SPX_PLATFORM_CAP_TTY,
    .name = "Windows",
    .version = NULL
};
#else
static const spx_platform_info_t stub_platform_info = {
    .type = SPX_PLATFORM_UNKNOWN,
    .capabilities = SPX_PLATFORM_CAP_WALL_TIME,
    .name = "Unknown",
    .version = NULL
};
#endif

static const spx_platform_t stub_platform = {
    .info = &stub_platform_info,
    .time = &stub_time_provider,
    .memory = &stub_memory_provider,
    .io = &stub_io_provider,
    .console = &stub_console_provider,
    .fs = &stub_fs_provider
};

static int platform_initialized = 0;

/*============================================================================
 * Platform Lifecycle
 *============================================================================*/

int spx_platform_init(spx_error_t *error)
{
    (void)error;

    if (platform_initialized) {
        return 0;
    }

    platform_initialized = 1;
    return 0;
}

void spx_platform_shutdown(void)
{
    platform_initialized = 0;
}

const spx_platform_t *spx_platform_get(void)
{
    return platform_initialized ? &stub_platform : NULL;
}

spx_platform_type_t spx_platform_get_type(void)
{
    return stub_platform_info.type;
}

int spx_platform_has_capability(spx_platform_caps_t cap)
{
    return (stub_platform_info.capabilities & cap) != 0;
}

#endif /* Unsupported platforms */
