/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Centralized limits and capacity configuration
 */

#include "spx_limits.h"
#include <stdlib.h>
#include <string.h>

/* Thread-local or global limits storage */
static struct {
    size_t max_stack_depth;
    size_t max_function_table_size;
    size_t reporter_buffer_size;
    int initialized;
} limits = {
    .max_stack_depth = SPX_DEFAULT_MAX_STACK_DEPTH,
    .max_function_table_size = SPX_DEFAULT_MAX_FUNCTION_TABLE_SIZE,
    .reporter_buffer_size = SPX_DEFAULT_REPORTER_BUFFER_SIZE,
    .initialized = 0
};

static size_t parse_size_t_env(const char *env_var, size_t default_value) {
    const char *value = getenv(env_var);
    if (!value || !*value) {
        return default_value;
    }

    char *endptr = NULL;
    long parsed = strtol(value, &endptr, 10);

    /* Invalid parse or non-positive value */
    if (endptr == value || *endptr != '\0' || parsed <= 0) {
        return default_value;
    }

    return (size_t)parsed;
}

void spx_limits_init(void) {
    limits.max_stack_depth = parse_size_t_env(
        "SPX_MAX_STACK_DEPTH",
        SPX_DEFAULT_MAX_STACK_DEPTH
    );

    limits.max_function_table_size = parse_size_t_env(
        "SPX_MAX_FUNCTION_TABLE_SIZE",
        SPX_DEFAULT_MAX_FUNCTION_TABLE_SIZE
    );

    limits.reporter_buffer_size = parse_size_t_env(
        "SPX_REPORTER_BUFFER_SIZE",
        SPX_DEFAULT_REPORTER_BUFFER_SIZE
    );

    limits.initialized = 1;
}

size_t spx_get_max_stack_depth(void) {
    if (!limits.initialized) {
        spx_limits_init();
    }
    return limits.max_stack_depth;
}

size_t spx_get_max_function_table_size(void) {
    if (!limits.initialized) {
        spx_limits_init();
    }
    return limits.max_function_table_size;
}

size_t spx_get_reporter_buffer_size(void) {
    if (!limits.initialized) {
        spx_limits_init();
    }
    return limits.reporter_buffer_size;
}

size_t spx_get_json_escape_buffer_size(void) {
    return SPX_DEFAULT_JSON_ESCAPE_BUFFER_SIZE;
}

size_t spx_get_max_key_size(void) {
    return SPX_DEFAULT_MAX_KEY_SIZE;
}
