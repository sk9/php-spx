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

#include "spx_session.h"
#include "spx_reporter_fp.h"
#include "spx_reporter_full.h"
#include "spx_reporter_trace.h"
#include "spx_profiler_tracer.h"
#include "spx_profiler_sampler.h"
#include "spx_error.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define REPORT_KEY_SIZE 512

struct spx_session_t {
    spx_config_t config;
    spx_profiler_t *profiler;
    spx_profiler_reporter_t *reporter;
    char report_key[REPORT_KEY_SIZE];
    int is_active;
};

spx_session_t *spx_session_create(const spx_config_t *config)
{
    if (!config) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Config cannot be NULL");
        return NULL;
    }

    spx_session_t *session = malloc(sizeof(*session));
    if (!session) {
        spx_error_set(SPX_ERR_OUT_OF_MEMORY, "Failed to allocate session");
        return NULL;
    }

    /* Copy configuration */
    memcpy(&session->config, config, sizeof(session->config));

    /* Initialize state */
    session->profiler = NULL;
    session->reporter = NULL;
    session->report_key[0] = '\0';
    session->is_active = 0;

    return session;
}

void spx_session_destroy(spx_session_t *session)
{
    if (!session) {
        return;
    }

    /* Auto-stop if still active */
    if (session->is_active) {
        spx_session_stop(session);
    }

    free(session);
}

int spx_session_start(spx_session_t *session, const char *data_dir)
{
    if (!session) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Session cannot be NULL");
        return -1;
    }

    /* Already active - ignore */
    if (session->is_active) {
        return 0;
    }

    session->report_key[0] = '\0';

    /* Create reporter based on config */
    switch (session->config.report) {
        default:
        case SPX_CONFIG_REPORT_FULL:
            session->reporter = spx_reporter_full_create(data_dir);
            if (session->reporter) {
                const char *key = spx_reporter_full_get_key(session->reporter);
                if (key) {
                    snprintf(session->report_key, REPORT_KEY_SIZE, "%s", key);
                }
            }
            break;

        case SPX_CONFIG_REPORT_FLAT_PROFILE:
            session->reporter = spx_reporter_fp_create(
                session->config.fp_focus,
                session->config.fp_inc,
                session->config.fp_rel,
                session->config.fp_limit,
                session->config.fp_live,
                session->config.fp_color
            );
            break;

        case SPX_CONFIG_REPORT_TRACE:
            session->reporter = spx_reporter_trace_create(
                session->config.trace_file,
                session->config.trace_safe
            );
            break;
    }

    if (!session->reporter) {
        spx_error_set(SPX_ERR_INTERNAL, "Failed to create reporter");
        goto error;
    }

    /* Create tracer profiler */
    session->profiler = spx_profiler_tracer_create(
        session->config.max_depth,
        session->config.enabled_metrics,
        session->reporter
    );

    if (!session->profiler) {
        spx_error_set(SPX_ERR_INTERNAL, "Failed to create profiler");
        goto error;
    }

    /* Wrap with sampler if configured */
    if (session->config.sampling_period > 0) {
        spx_profiler_t *sampling_profiler = spx_profiler_sampler_create(
            session->profiler,
            session->config.sampling_period
        );

        if (!sampling_profiler) {
            spx_error_set(SPX_ERR_INTERNAL, "Failed to create sampling profiler");
            goto error;
        }

        session->profiler = sampling_profiler;
    }

    session->is_active = 1;
    return 0;

error:
    if (session->profiler) {
        session->profiler->vtable.destroy(session->profiler);
        session->profiler = NULL;
    }

    if (session->reporter) {
        session->reporter->vtable.destroy(session->reporter);
        session->reporter = NULL;
    }

    return -1;
}

void spx_session_stop(spx_session_t *session)
{
    if (!session || !session->is_active) {
        return;
    }

    /* Cleanup profiler (destroys sampler if present, which destroys tracer) */
    if (session->profiler) {
        session->profiler->vtable.destroy(session->profiler);
        session->profiler = NULL;
    }

    /* Cleanup reporter */
    if (session->reporter) {
        session->reporter->vtable.destroy(session->reporter);
        session->reporter = NULL;
    }

    session->report_key[0] = '\0';
    session->is_active = 0;
}

int spx_session_is_active(const spx_session_t *session)
{
    return session ? session->is_active : 0;
}

spx_profiler_t *spx_session_get_profiler(spx_session_t *session)
{
    if (!session || !session->is_active) {
        return NULL;
    }

    return session->profiler;
}

spx_profiler_reporter_t *spx_session_get_reporter(spx_session_t *session)
{
    if (!session || !session->is_active) {
        return NULL;
    }

    return session->reporter;
}

const char *spx_session_get_report_key(const spx_session_t *session)
{
    if (!session || !session->is_active) {
        return NULL;
    }

    if (session->report_key[0] == '\0') {
        return NULL;
    }

    return session->report_key;
}

const spx_config_t *spx_session_get_config(const spx_session_t *session)
{
    return session ? &session->config : NULL;
}

int spx_session_set_custom_metadata(spx_session_t *session, const char *metadata)
{
    if (!session || !session->is_active) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Session not active");
        return -1;
    }

    if (session->config.report != SPX_CONFIG_REPORT_FULL) {
        spx_error_set(SPX_ERR_INVALID_CONFIG, "Custom metadata only valid for full reports");
        return -1;
    }

    if (!session->reporter) {
        spx_error_set(SPX_ERR_INTERNAL, "Reporter not available");
        return -1;
    }

    spx_reporter_full_set_custom_metadata_str(session->reporter, metadata);
    return 0;
}
