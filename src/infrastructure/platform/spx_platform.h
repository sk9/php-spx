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
 * @file spx_platform.h
 * @brief Platform abstraction layer for cross-platform compatibility
 *
 * This module provides a unified interface for platform-specific operations,
 * allowing the core profiling code to remain platform-agnostic. The actual
 * implementations are in platform-specific files (spx_platform_linux.c, etc.)
 *
 * Design Principles:
 * - Interface Segregation: Separate interfaces for different concerns
 * - Dependency Inversion: Core code depends on this abstraction, not implementations
 * - Open/Closed: New platforms can be added without modifying existing code
 */

#ifndef SPX_PLATFORM_H_DEFINED
#define SPX_PLATFORM_H_DEFINED

#include <stddef.h>

#include "../spx_error.h"

/*============================================================================
 * Platform Identification
 *============================================================================*/

/**
 * @brief Supported platform types
 */
typedef enum {
    SPX_PLATFORM_LINUX,
    SPX_PLATFORM_MACOS,
    SPX_PLATFORM_FREEBSD,
    SPX_PLATFORM_WINDOWS,
    SPX_PLATFORM_UNKNOWN
} spx_platform_type_t;

/**
 * @brief Platform capabilities bitflags
 */
typedef enum {
    SPX_PLATFORM_CAP_WALL_TIME = 1 << 0,     /**< High-resolution wall time */
    SPX_PLATFORM_CAP_CPU_TIME = 1 << 1,      /**< Process CPU time */
    SPX_PLATFORM_CAP_MEMORY_RSS = 1 << 2,    /**< Resident set size */
    SPX_PLATFORM_CAP_IO_STATS = 1 << 3,      /**< I/O statistics */
    SPX_PLATFORM_CAP_PROCFS = 1 << 4,        /**< /proc filesystem */
    SPX_PLATFORM_CAP_MACH_PORTS = 1 << 5,    /**< Mach ports (macOS) */
    SPX_PLATFORM_CAP_KQUEUE = 1 << 6,        /**< kqueue events (BSD) */
    SPX_PLATFORM_CAP_TTY = 1 << 7,           /**< TTY/terminal support */
    SPX_PLATFORM_CAP_ANSI_COLORS = 1 << 8    /**< ANSI color sequences */
} spx_platform_caps_t;

/**
 * @brief Platform information structure
 */
typedef struct {
    spx_platform_type_t type;
    unsigned int capabilities;  /**< Bitmask of spx_platform_caps_t */
    const char *name;           /**< Human-readable platform name */
    const char *version;        /**< Platform version string */
} spx_platform_info_t;

/*============================================================================
 * Time Operations Interface
 *============================================================================*/

/**
 * @brief Time provider interface for wall-clock and CPU time measurements
 */
typedef struct spx_time_provider_t {
    /**
     * @brief Get current wall-clock time in nanoseconds
     * @return Nanoseconds since an arbitrary epoch
     */
    size_t (*get_wall_time_ns)(void);

    /**
     * @brief Get current process CPU time in nanoseconds
     * @return Nanoseconds of CPU time consumed
     */
    size_t (*get_cpu_time_ns)(void);

    /**
     * @brief Get high-resolution timestamp for profiling
     * @return Platform-specific high-resolution timestamp
     */
    size_t (*get_timestamp)(void);

    /**
     * @brief Convert timestamp difference to nanoseconds
     * @param start Start timestamp
     * @param end End timestamp
     * @return Duration in nanoseconds
     */
    size_t (*timestamp_to_ns)(size_t start, size_t end);
} spx_time_provider_t;

/*============================================================================
 * Memory Statistics Interface
 *============================================================================*/

/**
 * @brief Memory statistics structure
 */
typedef struct {
    size_t rss;          /**< Resident set size in bytes */
    size_t vss;          /**< Virtual memory size in bytes */
    size_t shared;       /**< Shared memory in bytes */
    size_t private_mem;  /**< Private memory in bytes (renamed from 'private') */
    size_t peak_rss;     /**< Peak RSS in bytes (if available) */
} spx_memory_stats_t;

/**
 * @brief Memory provider interface
 */
typedef struct spx_memory_provider_t {
    /**
     * @brief Get current process memory statistics
     * @param stats Output structure for memory statistics
     * @param error Output error on failure
     * @return 0 on success, -1 on failure
     */
    int (*get_stats)(spx_memory_stats_t *stats, spx_error_t *error);

    /**
     * @brief Get only RSS (resident set size) - optimized for hot path
     * @return Current RSS in bytes, 0 on error
     */
    size_t (*get_rss)(void);
} spx_memory_provider_t;

/*============================================================================
 * I/O Statistics Interface
 *============================================================================*/

/**
 * @brief I/O statistics structure
 */
typedef struct {
    size_t bytes_read;    /**< Total bytes read */
    size_t bytes_written; /**< Total bytes written */
    size_t read_ops;      /**< Number of read operations */
    size_t write_ops;     /**< Number of write operations */
} spx_io_stats_t;

/**
 * @brief I/O provider interface
 */
typedef struct spx_io_provider_t {
    /**
     * @brief Get current process I/O statistics
     * @param stats Output structure for I/O statistics
     * @param error Output error on failure
     * @return 0 on success, -1 on failure
     */
    int (*get_stats)(spx_io_stats_t *stats, spx_error_t *error);

    /**
     * @brief Get bytes read/written - optimized for hot path
     * @param bytes_in Output: bytes read
     * @param bytes_out Output: bytes written
     */
    void (*get_bytes)(size_t *bytes_in, size_t *bytes_out);
} spx_io_provider_t;

/*============================================================================
 * Console/Terminal Interface
 *============================================================================*/

/**
 * @brief Console provider interface for terminal operations
 */
typedef struct spx_console_provider_t {
    /**
     * @brief Check if stdout is a TTY
     * @return 1 if TTY, 0 otherwise
     */
    int (*is_tty)(void);

    /**
     * @brief Check if ANSI sequences are supported
     * @return 1 if supported, 0 otherwise
     */
    int (*supports_ansi)(void);

    /**
     * @brief Get terminal width in columns
     * @return Terminal width, or 80 as default
     */
    int (*get_width)(void);

    /**
     * @brief Get terminal height in rows
     * @return Terminal height, or 24 as default
     */
    int (*get_height)(void);
} spx_console_provider_t;

/*============================================================================
 * File System Interface
 *============================================================================*/

/**
 * @brief File information structure
 */
typedef struct {
    size_t size;
    int is_directory;
    int is_symlink;
    int is_regular;
    unsigned int mode;  /**< File permissions */
} spx_file_info_t;

/**
 * @brief File system provider interface
 */
typedef struct spx_fs_provider_t {
    /**
     * @brief Get file information
     * @param path File path
     * @param info Output file info
     * @param follow_symlinks Follow symlinks if true
     * @param error Output error on failure
     * @return 0 on success, -1 on failure
     */
    int (*get_info)(const char *path, spx_file_info_t *info, int follow_symlinks, spx_error_t *error);

    /**
     * @brief Create directory with specified permissions
     * @param path Directory path
     * @param mode Permissions (e.g., 0750)
     * @param error Output error on failure
     * @return 0 on success, -1 on failure
     */
    int (*mkdir)(const char *path, unsigned int mode, spx_error_t *error);

    /**
     * @brief Resolve path to absolute form
     * @param path Input path
     * @param resolved Output buffer for resolved path
     * @param size Size of output buffer
     * @param error Output error on failure
     * @return 0 on success, -1 on failure
     */
    int (*realpath)(const char *path, char *resolved, size_t size, spx_error_t *error);
} spx_fs_provider_t;

/*============================================================================
 * Unified Platform Interface
 *============================================================================*/

/**
 * @brief Complete platform interface aggregating all providers
 */
typedef struct spx_platform_t {
    const spx_platform_info_t *info;
    const spx_time_provider_t *time;
    const spx_memory_provider_t *memory;
    const spx_io_provider_t *io;
    const spx_console_provider_t *console;
    const spx_fs_provider_t *fs;
} spx_platform_t;

/*============================================================================
 * Platform Lifecycle Functions
 *============================================================================*/

/**
 * @brief Initialize the platform layer
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 *
 * Must be called once during extension initialization.
 */
int spx_platform_init(spx_error_t *error);

/**
 * @brief Shutdown the platform layer
 *
 * Should be called during extension shutdown.
 */
void spx_platform_shutdown(void);

/**
 * @brief Get the current platform interface
 * @return Pointer to the platform interface, or NULL if not initialized
 */
const spx_platform_t *spx_platform_get(void);

/**
 * @brief Get current platform type
 * @return Platform type enumeration value
 */
spx_platform_type_t spx_platform_get_type(void);

/**
 * @brief Check if platform has specific capability
 * @param cap Capability to check
 * @return 1 if capability is available, 0 otherwise
 */
int spx_platform_has_capability(spx_platform_caps_t cap);

/*============================================================================
 * Convenience Functions (Inline Wrappers)
 *============================================================================*/

/**
 * @brief Get wall time using platform-specific method
 * @return Wall time in nanoseconds
 */
static inline size_t spx_platform_wall_time_ns(void)
{
    const spx_platform_t *platform = spx_platform_get();
    return platform && platform->time ? platform->time->get_wall_time_ns() : 0;
}

/**
 * @brief Get CPU time using platform-specific method
 * @return CPU time in nanoseconds
 */
static inline size_t spx_platform_cpu_time_ns(void)
{
    const spx_platform_t *platform = spx_platform_get();
    return platform && platform->time ? platform->time->get_cpu_time_ns() : 0;
}

/**
 * @brief Get RSS using platform-specific method
 * @return RSS in bytes
 */
static inline size_t spx_platform_memory_rss(void)
{
    const spx_platform_t *platform = spx_platform_get();
    return platform && platform->memory ? platform->memory->get_rss() : 0;
}

/**
 * @brief Get I/O bytes using platform-specific method
 * @param bytes_in Output: bytes read
 * @param bytes_out Output: bytes written
 */
static inline void spx_platform_io_bytes(size_t *bytes_in, size_t *bytes_out)
{
    const spx_platform_t *platform = spx_platform_get();
    if (platform && platform->io) {
        platform->io->get_bytes(bytes_in, bytes_out);
    } else {
        *bytes_in = 0;
        *bytes_out = 0;
    }
}

#endif /* SPX_PLATFORM_H_DEFINED */
