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

#ifndef SPX_SECURITY_CRYPTO_H_DEFINED
#define SPX_SECURITY_CRYPTO_H_DEFINED

#include <stddef.h>

/**
 * Constant-time memory comparison
 *
 * Compares two memory regions in constant time to prevent timing attacks.
 * Returns 0 if equal, non-zero if different.
 *
 * SECURITY: This function executes in constant time regardless of where
 * differences occur in the buffers.
 */
int spx_crypto_compare_constant_time(
    const void *a,
    const void *b,
    size_t len
);

/**
 * Constant-time string comparison
 *
 * Compares two null-terminated strings in constant time.
 * Returns 0 if equal, non-zero if different.
 *
 * SECURITY: This function executes in constant time regardless of string
 * content. Both strings must be non-NULL.
 */
int spx_crypto_compare_strings_constant_time(
    const char *a,
    const char *b
);

/**
 * Generate cryptographically secure random bytes
 *
 * Fills buffer with secure random bytes from /dev/urandom or equivalent.
 * Returns 0 on success, -1 on failure.
 */
int spx_crypto_random_bytes(void *buffer, size_t size);

/**
 * Generate a random hex key
 *
 * Generates a random key and formats it as hexadecimal string.
 * Buffer size should be at least (size * 2) + 1 bytes.
 * Returns 0 on success, -1 on failure.
 */
int spx_crypto_generate_hex_key(char *buffer, size_t size, size_t key_bytes);

#endif /* SPX_SECURITY_CRYPTO_H_DEFINED */
