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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ZTS
#include <pthread.h>
#endif

#include <arpa/inet.h>

#include "spx_php.h"
#include "spx_utils.h"

int spx_utils_ip_match(const char *ip_address_str, const char *target)
{
    if (!ip_address_str || !target) {
        return 0;
    }

    if (strcmp(target, "*") == 0 || strcmp(target, ip_address_str) == 0) {
        return 1;
    }

    /* Subnet handling */
    const char *slash_ptr = strchr(target, '/');
    if (slash_ptr == NULL) {
        return 0;
    }

    const size_t slash_pos = slash_ptr - target;

    /* IPv4 address is 7-15 characters (min: "1.1.1.1", max: "255.255.255.255") */
    if (!(7 <= slash_pos && slash_pos <= 15)) {
        return 0;
    }

    const size_t target_suffix_len = strlen(slash_ptr);
    if (!(2 <= target_suffix_len && target_suffix_len <= 3)) {
        return 0;
    }

    /* Use safe buffer size for IPv4 addresses (max 15 chars + null terminator) */
    char target_ip_address_str[16];

    /* Safely copy IP address part (before slash) with null termination (Issue 1.3) */
    if (slash_pos >= sizeof(target_ip_address_str)) {
        return 0;
    }

    memcpy(target_ip_address_str, target, slash_pos);
    target_ip_address_str[slash_pos] = '\0';

    const in_addr_t target_ip_address = inet_addr(target_ip_address_str);
    if (target_ip_address == INADDR_NONE) {
        return 0;
    }

    /* Parse subnet mask bits (e.g., "24" from "/24") */
    char target_mask_str[4];  /* Max 2 digits + null terminator */
    size_t mask_len = strlen(slash_ptr + 1);

    if (mask_len >= sizeof(target_mask_str)) {
        return 0;
    }

    memcpy(target_mask_str, slash_ptr + 1, mask_len);
    target_mask_str[mask_len] = '\0';

    const long target_mask_bits = strtol(target_mask_str, NULL, 10);

    if (!(1 <= target_mask_bits && target_mask_bits <= 31)) {
        return 0;
    }

    const in_addr_t target_mask = (~0) << (32 - target_mask_bits);

    const in_addr_t ip_address = inet_addr(ip_address_str);
    if (ip_address == INADDR_NONE) {
        return 0;
    }

    if ((ntohl(ip_address) & target_mask) == (ntohl(target_ip_address) & target_mask)) {
        return 1;
    }

    return 0;
}

char *spx_utils_resolve_confined_file_absolute_path(const char *root_dir, const char *relative_path,
                                                    const char *suffix, char *dst, size_t size)
{
    if (size < PATH_MAX) {
        spx_utils_die("size < PATH_MAX");
    }

    char absolute_file_path[PATH_MAX];

    snprintf(absolute_file_path, sizeof(absolute_file_path), "%s%s%s", root_dir, relative_path,
             suffix == NULL ? "" : suffix);

    if (realpath(absolute_file_path, dst) == NULL) {
        return NULL;
    }

    char root_dir_real_path[PATH_MAX];
    if (realpath(root_dir, root_dir_real_path) == NULL) {
        return NULL;
    }

    char expected_path_prefix[PATH_MAX + 1];
    snprintf(expected_path_prefix, sizeof(expected_path_prefix), "%s/", root_dir_real_path);

    if (!spx_utils_str_starts_with(dst, expected_path_prefix)) {
        return NULL;
    }

    return dst;
}

char *spx_utils_json_escape(char *dst, const char *src, size_t limit)
{
    size_t i = 0;
    while (*src) {
        if (i >= limit) {
            goto limit_reached;
        }

        char escaped_char = 0;

        switch (*src) {
        case '\\':
        case '"':
        case '/':
            escaped_char = *src;

            break;

        case '\b':
            escaped_char = 'b';

            break;

        case '\f':
            escaped_char = 'f';

            break;

        case '\n':
            escaped_char = 'n';

            break;

        case '\r':
            escaped_char = 'r';

            break;

        case '\t':
            escaped_char = 't';

            break;
        }

        if (escaped_char != 0) {
            dst[i++] = '\\';

            if (i >= limit) {
                goto limit_reached;
            }

            dst[i++] = escaped_char;
        } else {
            dst[i++] = *src;
        }

        src++;
    }

    if (i >= limit) {
        goto limit_reached;
    }

    dst[i] = 0;

    return dst;

limit_reached:
    spx_utils_die("The provided buffer is too small to contain the escaped JSON string");

    /* unreacheable */
    return NULL;
}

int spx_utils_str_starts_with(const char *str, const char *prefix)
{
    return 0 == strncmp(str, prefix, strlen(prefix));
}

int spx_utils_str_ends_with(const char *str, const char *suffix)
{
    const size_t str_len = strlen(str);
    const size_t suffix_len = strlen(suffix);

    if (str_len < suffix_len) {
        return 0;
    }

    if (strcmp(str + str_len - suffix_len, suffix) == 0) {
        return 1;
    }

    return 0;
}

void spx_utils_die_(const char *msg, const char *file, size_t line)
{
    fprintf(stderr, "SPX Fatal error at %s:%zu - %s\n", file, line, msg);

#ifdef ZTS
    pthread_exit(NULL);
#else
    exit(EXIT_FAILURE);
#endif
}
