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

/*
 *  Constant-time string comparison. Returns 0 if equal, non-zero otherwise.
 *  Both inputs must be non-NULL.
 */
int spx_crypto_compare_strings_constant_time(const char *a, const char *b);

/*
 *  Fill buffer with cryptographically secure random bytes from /dev/urandom.
 *  Returns 0 on success, -1 on failure.
 */
int spx_crypto_random_bytes(void *buffer, size_t size);

#endif /* SPX_SECURITY_CRYPTO_H_DEFINED */
