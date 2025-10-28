/* SPX - A simple profiler for PHP
 * Copyright (C) 2017-2025 Sylvain Lassaut <NoiseByNorthwest@gmail.com>
 *
 * String utility functions
 */

#ifndef SPX_STRING_H_DEFINED
#define SPX_STRING_H_DEFINED

#include <stdlib.h>
#include <string.h>

/**
 * Safely duplicate a string. Returns NULL if input is NULL.
 *
 * @param str String to duplicate (may be NULL)
 * @return Duplicated string or NULL
 */
static inline char *spx_strdup_safe(const char *str) {
    return str ? strdup(str) : NULL;
}

/**
 * Duplicate a string or use a default value if the input is NULL or empty.
 *
 * @param str String to duplicate (may be NULL or empty)
 * @param default_val Default value to use if str is NULL or empty
 * @return Duplicated string (either str or default_val)
 */
static inline char *spx_strdup_or_default(const char *str, const char *default_val) {
    if (!str || !*str) {
        return strdup(default_val);
    }
    return strdup(str);
}

#endif /* SPX_STRING_H_DEFINED */
