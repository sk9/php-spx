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
#include <string.h>
#include <unistd.h>

int spx_crypto_compare_strings_constant_time(const char *a, const char *b)
{
    if (!a || !b) {
        return 1;
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t compare_len = len_a < len_b ? len_a : len_b;

    unsigned char result = (unsigned char) (len_a ^ len_b);
    for (size_t i = 0; i < compare_len; i++) {
        result |= (unsigned char) a[i] ^ (unsigned char) b[i];
    }

    return result;
}

int spx_crypto_random_bytes(void *buffer, size_t size)
{
    if (!buffer || size == 0) {
        return -1;
    }

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
        total_read += (size_t) n;
    }

    close(fd);
    return 0;
}
