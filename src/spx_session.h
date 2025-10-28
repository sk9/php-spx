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

#ifndef SPX_SESSION_H
#define SPX_SESSION_H

#include "spx_config.h"
#include "spx_profiler.h"

/**
 * spx_session - Manages a profiling session
 *
 * This module encapsulates all state related to a single profiling session,
 * including configuration, profiler, and reporter instances. It provides a
 * clean interface for session lifecycle management.
 *
 * Benefits:
 * - Single Responsibility: Only manages session state
 * - Testability: Can be tested independently of PHP extension lifecycle
 * - Clear ownership: Session owns its profiler and reporter
 *
 * Usage:
 *   spx_session_t *session = spx_session_create(&config);
 *   spx_session_start(session, data_dir);
 *   // ... profiling happens ...
 *   spx_session_stop(session);
 *   spx_session_destroy(session);
 */

typedef struct spx_session_t spx_session_t;

/**
 * Create a new profiling session with the given configuration.
 *
 * @param config Configuration to use for this session (copied internally)
 * @return New session instance, or NULL on failure
 */
spx_session_t *spx_session_create(const spx_config_t *config);

/**
 * Destroy a profiling session and free all resources.
 * Automatically stops profiling if still active.
 *
 * @param session Session to destroy
 */
void spx_session_destroy(spx_session_t *session);

/**
 * Start profiling with this session.
 * Creates the appropriate reporter and profiler based on configuration.
 *
 * @param session Session to start
 * @param data_dir Directory for full report output (may be NULL for other report types)
 * @return 0 on success, -1 on failure
 */
int spx_session_start(spx_session_t *session, const char *data_dir);

/**
 * Stop profiling for this session.
 * Finalizes reports and cleans up profiler/reporter.
 *
 * @param session Session to stop
 */
void spx_session_stop(spx_session_t *session);

/**
 * Check if session is currently profiling.
 *
 * @param session Session to check
 * @return 1 if profiling, 0 otherwise
 */
int spx_session_is_active(const spx_session_t *session);

/**
 * Get the profiler instance for this session.
 * Only valid while session is active.
 *
 * @param session Session
 * @return Profiler instance, or NULL if not profiling
 */
spx_profiler_t *spx_session_get_profiler(spx_session_t *session);

/**
 * Get the reporter instance for this session.
 * Only valid while session is active.
 *
 * @param session Session
 * @return Reporter instance, or NULL if not profiling
 */
spx_profiler_reporter_t *spx_session_get_reporter(spx_session_t *session);

/**
 * Get the full report key for this session.
 * Only valid for full report mode while session is active.
 *
 * @param session Session
 * @return Report key string, or NULL if not in full report mode
 */
const char *spx_session_get_report_key(const spx_session_t *session);

/**
 * Get the configuration for this session.
 *
 * @param session Session
 * @return Configuration (read-only)
 */
const spx_config_t *spx_session_get_config(const spx_session_t *session);

/**
 * Set custom metadata string for full reports.
 * Only valid for full report mode.
 *
 * @param session Session
 * @param metadata Custom metadata string
 * @return 0 on success, -1 on failure
 */
int spx_session_set_custom_metadata(spx_session_t *session, const char *metadata);

#endif /* SPX_SESSION_H */
