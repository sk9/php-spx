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

#include "spx_security_validation.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* String validation */
spx_validate_result_t spx_validate_string(const char *str,
                                          const spx_string_constraints_t *constraints,
                                          spx_error_t *error)
{
    if (!constraints) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL constraints");
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    if (!str) {
        if (constraints->allow_null) {
            return SPX_VALIDATE_SUCCESS;
        }
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL string");
        return SPX_VALIDATE_NULL_INPUT;
    }

    size_t len = strlen(str);

    if (len < constraints->min_length) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "String too short: %zu < %zu", len,
                      constraints->min_length);
        return SPX_VALIDATE_TOO_SHORT;
    }

    if (len > constraints->max_length) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "String too long: %zu > %zu", len,
                      constraints->max_length);
        return SPX_VALIDATE_TOO_LONG;
    }

    /* Validate character set if specified */
    if (constraints->allowed_charset) {
        for (size_t i = 0; i < len; i++) {
            if (!strchr(constraints->allowed_charset, str[i])) {
                SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                              "Invalid character at position %zu: 0x%02x", i,
                              (unsigned char) str[i]);
                return SPX_VALIDATE_INVALID_CHARS;
            }
        }
    }

    return SPX_VALIDATE_SUCCESS;
}

/* Integer parsing with validation - this replaces dangerous atoi() calls */
spx_parse_result_t spx_parse_long(const char *str, long *out_value, long min_value, long max_value,
                                  spx_error_t *error)
{
    if (!str) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL input string");
        return SPX_PARSE_NULL_INPUT;
    }

    if (*str == '\0') {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Empty input string");
        return SPX_PARSE_EMPTY_STRING;
    }

    if (!out_value) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL output pointer");
        return SPX_PARSE_NULL_INPUT;
    }

    char *endptr;
    errno = 0;
    long value = strtol(str, &endptr, 10);

    /* Check for parsing errors */
    if (errno == ERANGE) {
        if (value == LONG_MAX) {
            SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Value overflow: %s", str);
            return SPX_PARSE_OVERFLOW;
        }
        if (value == LONG_MIN) {
            SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Value underflow: %s", str);
            return SPX_PARSE_UNDERFLOW;
        }
    }

    /* Check if entire string was consumed */
    if (*endptr != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                      "Invalid characters in input: '%s' at position %ld", endptr,
                      (long) (endptr - str));
        return SPX_PARSE_INVALID_FORMAT;
    }

    /* Check bounds */
    if (value < min_value || value > max_value) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Value %ld out of range [%ld, %ld]", value,
                      min_value, max_value);
        return SPX_PARSE_OUT_OF_RANGE;
    }

    *out_value = value;
    return SPX_PARSE_SUCCESS;
}

spx_parse_result_t spx_parse_size_t(const char *str, size_t *out_value, size_t min_value,
                                    size_t max_value, spx_error_t *error)
{
    long value;
    spx_parse_result_t result = spx_parse_long(str, &value, 0, /* size_t is unsigned, so min is 0 */
                                               (long) max_value, error);

    if (result != SPX_PARSE_SUCCESS) {
        return result;
    }

    if (value < 0) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Negative value not allowed for size_t: %ld",
                      value);
        return SPX_PARSE_OUT_OF_RANGE;
    }

    if ((size_t) value < min_value) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Value %zu out of range [%zu, %zu]",
                      (size_t) value, min_value, max_value);
        return SPX_PARSE_OUT_OF_RANGE;
    }

    *out_value = (size_t) value;
    return SPX_PARSE_SUCCESS;
}

/* Path validation - prevents path traversal attacks */
spx_validate_result_t spx_validate_path(const char *path, const char *base_dir,
                                        spx_path_flags_t flags, char *resolved_path,
                                        size_t resolved_path_size, spx_error_t *error)
{
    /* Input validation */
    if (!path || !base_dir || !resolved_path) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL parameter");
        return SPX_VALIDATE_NULL_INPUT;
    }

    if (resolved_path_size < PATH_MAX) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Buffer too small (need PATH_MAX=%d, got %zu)",
                      PATH_MAX, resolved_path_size);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    /* Validate base directory exists */
    struct stat base_stat;
    if (stat(base_dir, &base_stat) != 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Base directory does not exist: %s", base_dir);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    if (!S_ISDIR(base_stat.st_mode)) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Base path is not a directory: %s", base_dir);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    /* Construct full path */
    char full_path[PATH_MAX];
    int written = snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, path);
    if (written >= (int) sizeof(full_path)) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Path too long");
        return SPX_VALIDATE_TOO_LONG;
    }

    /* Open file with O_NOFOLLOW if symlinks not allowed */
    int fd = -1;
    if (!(flags & SPX_PATH_ALLOW_SYMLINKS)) {
        int open_flags = O_RDONLY | O_NOFOLLOW;
        if (flags & SPX_PATH_MUST_BE_DIR) {
            open_flags |= O_DIRECTORY;
        }

        fd = open(full_path, open_flags);
        if (fd < 0) {
            if (errno == ELOOP || errno == EMLINK) {
                SPX_ERROR_SET(error, SPX_ERR_PATH_TRAVERSAL, "Symlink detected: %s", path);
                return SPX_VALIDATE_INVALID_FORMAT;
            }
            if (!(flags & SPX_PATH_MUST_EXIST)) {
                /* Path doesn't exist, but that's OK */
                strncpy(resolved_path, full_path, resolved_path_size - 1);
                resolved_path[resolved_path_size - 1] = '\0';
                return SPX_VALIDATE_SUCCESS;
            }
            SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Cannot open path: %s (errno=%d)", path,
                          errno);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
    }

    /* Get real path using procfs (Linux) or realpath */
    char real_path[PATH_MAX];
#ifdef __linux__
    if (fd >= 0) {
        char fd_path[64];
        snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
        ssize_t len = readlink(fd_path, real_path, sizeof(real_path) - 1);
        if (len < 0) {
            close(fd);
            SPX_ERROR_SET(error, SPX_ERR_INTERNAL, "Failed to resolve path: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
        real_path[len] = '\0';
    } else
#endif
    {
        if (!realpath(full_path, real_path)) {
            if (fd >= 0)
                close(fd);
            if (!(flags & SPX_PATH_MUST_EXIST)) {
                /* If path doesn't need to exist, just normalize it */
                strncpy(resolved_path, full_path, resolved_path_size - 1);
                resolved_path[resolved_path_size - 1] = '\0';
                return SPX_VALIDATE_SUCCESS;
            }
            SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Path resolution failed: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
    }

    if (fd >= 0) {
        close(fd);
    }

    /* Verify path is within base directory */
    char base_real[PATH_MAX];
    if (!realpath(base_dir, base_real)) {
        SPX_ERROR_SET(error, SPX_ERR_INTERNAL, "Cannot resolve base directory: %s", base_dir);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    size_t base_len = strlen(base_real);
    if (strncmp(real_path, base_real, base_len) != 0) {
        SPX_ERROR_SET(error, SPX_ERR_PATH_TRAVERSAL, "Path escapes base directory: %s", path);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    /* Ensure there's a path separator after base (prevent partial match) */
    if (real_path[base_len] != '/' && real_path[base_len] != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_PATH_TRAVERSAL, "Path escapes base directory: %s", path);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    /* Additional checks */
    if (flags & SPX_PATH_MUST_EXIST) {
        struct stat st;
        if (stat(real_path, &st) != 0) {
            SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND, "Path does not exist: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }

        if ((flags & SPX_PATH_MUST_BE_DIR) && !S_ISDIR(st.st_mode)) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Path is not a directory: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }

        if ((flags & SPX_PATH_MUST_BE_FILE) && !S_ISREG(st.st_mode)) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Path is not a file: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
    }

    /* Copy resolved path to output */
    strncpy(resolved_path, real_path, resolved_path_size - 1);
    resolved_path[resolved_path_size - 1] = '\0';

    return SPX_VALIDATE_SUCCESS;
}

/* IP address validation */
spx_validate_result_t spx_validate_ip_address(const char *ip, spx_error_t *error)
{
    if (!ip) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL IP address");
        return SPX_VALIDATE_NULL_INPUT;
    }

    /* Try to parse as IPv4 */
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) == 1) {
        return SPX_VALIDATE_SUCCESS;
    }

    /* Try to parse as IPv6 */
    struct in6_addr addr6;
    if (inet_pton(AF_INET6, ip, &addr6) == 1) {
        return SPX_VALIDATE_SUCCESS;
    }

    SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Invalid IP address format: %s", ip);
    return SPX_VALIDATE_INVALID_FORMAT;
}

/* Check if IP is in whitelist */
spx_validate_result_t spx_validate_ip_in_whitelist(const char *ip, const char *whitelist,
                                                   spx_error_t *error)
{
    spx_validate_result_t result;

    /* Validate IP format first */
    result = spx_validate_ip_address(ip, error);
    if (result != SPX_VALIDATE_SUCCESS) {
        return result;
    }

    if (!whitelist || *whitelist == '\0') {
        SPX_ERROR_SET(error, SPX_ERR_ACCESS_DENIED, "Empty whitelist");
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    /* Check if wildcard */
    if (strcmp(whitelist, "*") == 0) {
        return SPX_VALIDATE_SUCCESS;
    }

    /* Parse whitelist (comma-separated) */
    char whitelist_copy[4096];
    strncpy(whitelist_copy, whitelist, sizeof(whitelist_copy) - 1);
    whitelist_copy[sizeof(whitelist_copy) - 1] = '\0';

    char *token = strtok(whitelist_copy, ",");
    while (token) {
        /* Trim whitespace */
        while (*token && isspace(*token))
            token++;
        char *end = token + strlen(token) - 1;
        while (end > token && isspace(*end))
            *end-- = '\0';

        /* Check for exact match */
        if (strcmp(ip, token) == 0) {
            return SPX_VALIDATE_SUCCESS;
        }

        /* Check for subnet match (simplified - only handles /xx notation) */
        char *slash = strchr(token, '/');
        if (slash) {
            /* TODO: Implement proper CIDR matching */
        }

        token = strtok(NULL, ",");
    }

    SPX_ERROR_SET(error, SPX_ERR_ACCESS_DENIED, "IP %s not in whitelist", ip);
    return SPX_VALIDATE_INVALID_FORMAT;
}
