/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * Standardized error handling
 */

#ifndef SPX_ERROR_H_DEFINED
#define SPX_ERROR_H_DEFINED

#include "spx_thread.h"

typedef enum {
    SPX_ERR_NONE = 0,
    SPX_ERR_OUT_OF_MEMORY,
    SPX_ERR_CAPACITY_EXCEEDED,
    SPX_ERR_IO_FAILURE,
    SPX_ERR_INVALID_CONFIG,
    SPX_ERR_INTERNAL,
} spx_error_t;

/**
 * Set the last error for the current thread.
 *
 * @param code Error code
 * @param message Error message (may be NULL)
 */
void spx_error_set(spx_error_t code, const char *message);

/**
 * Get the last error code for the current thread.
 *
 * @return Error code
 */
spx_error_t spx_error_get_last(void);

/**
 * Get the last error message for the current thread.
 *
 * @return Error message (never NULL, may be empty string)
 */
const char *spx_error_get_message(void);

/**
 * Clear the last error.
 */
void spx_error_clear(void);

/**
 * Get a human-readable string for an error code.
 *
 * @param code Error code
 * @return Error description
 */
const char *spx_error_string(spx_error_t code);

/**
 * Fatal error - prints message and exits immediately.
 * Use only for unrecoverable errors.
 *
 * @param message Error message
 */
void spx_fatal_error(const char *message) __attribute__((noreturn));

#endif /* SPX_ERROR_H_DEFINED */
