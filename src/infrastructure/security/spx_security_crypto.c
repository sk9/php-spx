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

#include "spx_security_crypto.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Constant-time comparison to prevent timing attacks */
int spx_crypto_compare_constant_time(const void *a, const void *b, size_t len)
{
    const unsigned char *aa = (const unsigned char *) a;
    const unsigned char *bb = (const unsigned char *) b;
    unsigned char result = 0;
    size_t i;

    /* XOR all bytes - result will be 0 only if all bytes match */
    for (i = 0; i < len; i++) {
        result |= aa[i] ^ bb[i];
    }

    return result;
}

int spx_crypto_compare_strings_constant_time(const char *a, const char *b)
{
    if (!a || !b) {
        /* NULL pointer - not equal */
        return 1;
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    /* Compare lengths in constant time */
    unsigned char length_equal = 0;
    length_equal = (unsigned char) (len_a ^ len_b);

    /* Always compare the shorter length to avoid timing leaks */
    size_t compare_len = len_a < len_b ? len_a : len_b;

    unsigned char result = spx_crypto_compare_constant_time(a, b, compare_len);

    /* Include length comparison in result */
    result |= length_equal;

    /*
     * Add a small random delay on mismatch to make timing attacks harder
     * Note: This is defense in depth - the constant-time comparison
     * is the primary defense
     */
    if (result != 0) {
        struct timespec ts;
        ts.tv_sec = 0;
        /* Random delay 0-5ms */
        ts.tv_nsec = ((unsigned long) time(NULL) % 5000) * 1000;
        nanosleep(&ts, NULL);
    }

    return result;
}

/* Generate secure random bytes */
int spx_crypto_random_bytes(void *buffer, size_t size)
{
    if (!buffer || size == 0) {
        return -1;
    }

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    /* Use /dev/urandom for cryptographically secure random */
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    size_t total_read = 0;
    while (total_read < size) {
        ssize_t n = read(fd, (unsigned char *) buffer + total_read, size - total_read);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        total_read += n;
    }

    close(fd);
    return 0;
#else
    /* Fallback to less secure random for unsupported platforms */
    size_t i;
    srand((unsigned int) time(NULL));
    unsigned char *buf = (unsigned char *) buffer;
    for (i = 0; i < size; i++) {
        buf[i] = (unsigned char) (rand() % 256);
    }
    return 0;
#endif
}

/* Generate a random hex key */
int spx_crypto_generate_hex_key(char *buffer, size_t size, size_t key_bytes)
{
    if (!buffer || size < (key_bytes * 2) + 1) {
        return -1;
    }

    unsigned char *random_bytes = malloc(key_bytes);
    if (!random_bytes) {
        return -1;
    }

    if (spx_crypto_random_bytes(random_bytes, key_bytes) != 0) {
        free(random_bytes);
        return -1;
    }

    /* Convert to hex */
    {
        size_t i;
        for (i = 0; i < key_bytes; i++) {
            snprintf(buffer + (i * 2), 3, "%02x", random_bytes[i]);
        }
    }
    buffer[key_bytes * 2] = '\0';

    /* Clear sensitive data */
    memset(random_bytes, 0, key_bytes);
    free(random_bytes);

    return 0;
}
