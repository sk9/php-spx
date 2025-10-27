/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Centralized limits and capacity configuration
 */

#ifndef SPX_LIMITS_H_DEFINED
#define SPX_LIMITS_H_DEFINED

#include <stddef.h>

/**
 * Maximum call stack depth. Prevents stack overflow in deeply recursive code.
 * Exceeding this limit logs a warning and stops profiling deeper calls.
 *
 * Default: 2048
 * Environment variable: SPX_MAX_STACK_DEPTH
 */
#define SPX_DEFAULT_MAX_STACK_DEPTH 2048

/**
 * Maximum unique functions that can be tracked in a single profiling session.
 * Reaching this limit logs a warning but continues profiling.
 *
 * Default: 65536
 * Environment variable: SPX_MAX_FUNCTION_TABLE_SIZE
 */
#define SPX_DEFAULT_MAX_FUNCTION_TABLE_SIZE 65536

/**
 * Reporter buffer size (number of events buffered before flush).
 * Larger buffers reduce I/O but increase memory usage.
 *
 * Default: 16384
 * Environment variable: SPX_REPORTER_BUFFER_SIZE
 */
#define SPX_DEFAULT_REPORTER_BUFFER_SIZE 16384

/**
 * JSON escape buffer size for metadata generation.
 * Default: 8192 bytes
 */
#define SPX_DEFAULT_JSON_ESCAPE_BUFFER_SIZE (8 * 1024)

/**
 * Maximum size for report keys and identifiers.
 * Default: 512 bytes
 */
#define SPX_DEFAULT_MAX_KEY_SIZE 512

/**
 * Maximum pathname size (uses system PATH_MAX where available)
 */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define SPX_MAX_PATHNAME_SIZE PATH_MAX

/**
 * Initialize limits from environment variables.
 * Call this early in extension initialization.
 */
void spx_limits_init(void);

/**
 * Get configured maximum stack depth.
 * Returns SPX_DEFAULT_MAX_STACK_DEPTH or value from SPX_MAX_STACK_DEPTH env var.
 */
size_t spx_get_max_stack_depth(void);

/**
 * Get configured maximum function table size.
 * Returns SPX_DEFAULT_MAX_FUNCTION_TABLE_SIZE or value from SPX_MAX_FUNCTION_TABLE_SIZE env var.
 */
size_t spx_get_max_function_table_size(void);

/**
 * Get configured reporter buffer size.
 * Returns SPX_DEFAULT_REPORTER_BUFFER_SIZE or value from SPX_REPORTER_BUFFER_SIZE env var.
 */
size_t spx_get_reporter_buffer_size(void);

/**
 * Get JSON escape buffer size.
 */
size_t spx_get_json_escape_buffer_size(void);

/**
 * Get maximum key size.
 */
size_t spx_get_max_key_size(void);

#endif /* SPX_LIMITS_H_DEFINED */
