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

/**
 * @file spx_http_handler.h
 * @brief HTTP request handler abstraction for SPX web UI
 *
 * This module provides a clean interface for handling HTTP requests
 * in the SPX web UI. It implements:
 *
 * - Request routing with pattern matching
 * - Content-type detection and response formatting
 * - Security validation (path confinement, authentication)
 * - Response caching and compression
 *
 * Design principles:
 * - Single Responsibility: Each handler handles one type of request
 * - Strategy Pattern: Handlers are interchangeable strategies
 * - Template Method: Common request handling with customizable parts
 */

#ifndef SPX_HTTP_HANDLER_H_DEFINED
#define SPX_HTTP_HANDLER_H_DEFINED

#include <stddef.h>

#include "infrastructure/spx_error.h"

/*============================================================================
 * HTTP Method Enumeration
 *============================================================================*/

typedef enum {
    SPX_HTTP_GET,
    SPX_HTTP_POST,
    SPX_HTTP_DELETE,
    SPX_HTTP_OPTIONS,
    SPX_HTTP_HEAD,
    SPX_HTTP_UNKNOWN
} spx_http_method_t;

/*============================================================================
 * HTTP Status Codes
 *============================================================================*/

typedef enum {
    SPX_HTTP_STATUS_OK = 200,
    SPX_HTTP_STATUS_CREATED = 201,
    SPX_HTTP_STATUS_NO_CONTENT = 204,
    SPX_HTTP_STATUS_NOT_MODIFIED = 304,
    SPX_HTTP_STATUS_BAD_REQUEST = 400,
    SPX_HTTP_STATUS_UNAUTHORIZED = 401,
    SPX_HTTP_STATUS_FORBIDDEN = 403,
    SPX_HTTP_STATUS_NOT_FOUND = 404,
    SPX_HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    SPX_HTTP_STATUS_INTERNAL_ERROR = 500
} spx_http_status_t;

/*============================================================================
 * HTTP Request Structure
 *============================================================================*/

/**
 * @brief HTTP request information
 */
typedef struct {
    spx_http_method_t method;     /**< Request method */
    const char *uri;              /**< Request URI */
    const char *query_string;     /**< Query string (after ?) */
    const char *content_type;     /**< Content-Type header */
    const char *accept;           /**< Accept header */
    const char *accept_encoding;  /**< Accept-Encoding header */
    const char *if_none_match;    /**< If-None-Match header (for caching) */
    const char *if_modified_since;/**< If-Modified-Since header */
    const char *origin;           /**< Origin header (for CORS) */
    size_t content_length;        /**< Content-Length header */
    const void *body;             /**< Request body */
} spx_http_request_t;

/*============================================================================
 * HTTP Response Structure
 *============================================================================*/

/**
 * @brief HTTP response builder
 */
typedef struct spx_http_response_t spx_http_response_t;

/**
 * @brief Create a new HTTP response
 * @return New response instance, or NULL on failure
 */
spx_http_response_t *spx_http_response_create(void);

/**
 * @brief Destroy an HTTP response
 * @param response Response to destroy
 */
void spx_http_response_destroy(spx_http_response_t *response);

/**
 * @brief Set response status code
 * @param response Response object
 * @param status HTTP status code
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_status(
    spx_http_response_t *response,
    spx_http_status_t status
);

/**
 * @brief Set a response header
 * @param response Response object
 * @param name Header name
 * @param value Header value
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_header(
    spx_http_response_t *response,
    const char *name,
    const char *value
);

/**
 * @brief Set Content-Type header
 * @param response Response object
 * @param content_type Content type value
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_content_type(
    spx_http_response_t *response,
    const char *content_type
);

/**
 * @brief Set response body
 * @param response Response object
 * @param data Body data
 * @param length Body length
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_body(
    spx_http_response_t *response,
    const void *data,
    size_t length
);

/**
 * @brief Set response body from string
 * @param response Response object
 * @param text Text body (null-terminated)
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_body_text(
    spx_http_response_t *response,
    const char *text
);

/**
 * @brief Send response body from file
 * @param response Response object
 * @param file_path Path to file
 * @param error Output error on failure
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_body_file(
    spx_http_response_t *response,
    const char *file_path,
    spx_error_t *error
);

/**
 * @brief Enable gzip compression for response
 * @param response Response object
 * @param enabled 1 to enable, 0 to disable
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_gzip(
    spx_http_response_t *response,
    int enabled
);

/**
 * @brief Add CORS headers
 * @param response Response object
 * @param origin Allowed origin (or "*")
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_cors(
    spx_http_response_t *response,
    const char *origin
);

/**
 * @brief Set cache control headers
 * @param response Response object
 * @param max_age Max age in seconds (0 for no-cache)
 * @param etag ETag value (or NULL)
 * @return Same response for chaining
 */
spx_http_response_t *spx_http_response_cache(
    spx_http_response_t *response,
    int max_age,
    const char *etag
);

/**
 * @brief Finalize and send the response
 * @param response Response object
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 */
int spx_http_response_send(spx_http_response_t *response, spx_error_t *error);

/*============================================================================
 * HTTP Handler Interface
 *============================================================================*/

/**
 * @brief HTTP request handler function type
 * @param request Request information
 * @param response Response builder
 * @param context Handler-specific context
 * @param error Output error on failure
 * @return 0 on success (response sent), -1 on error, 1 if not handled
 */
typedef int (*spx_http_handler_func_t)(
    const spx_http_request_t *request,
    spx_http_response_t *response,
    void *context,
    spx_error_t *error
);

/**
 * @brief Route definition for HTTP routing
 */
typedef struct {
    spx_http_method_t method;        /**< HTTP method (or SPX_HTTP_UNKNOWN for any) */
    const char *pattern;             /**< URI pattern (supports wildcards) */
    spx_http_handler_func_t handler; /**< Handler function */
    void *context;                   /**< Handler context */
} spx_http_route_t;

/*============================================================================
 * HTTP Router
 *============================================================================*/

typedef struct spx_http_router_t spx_http_router_t;

/**
 * @brief Create a new HTTP router
 * @return New router instance, or NULL on failure
 */
spx_http_router_t *spx_http_router_create(void);

/**
 * @brief Destroy an HTTP router
 * @param router Router to destroy
 */
void spx_http_router_destroy(spx_http_router_t *router);

/**
 * @brief Add a route to the router
 * @param router Router instance
 * @param route Route definition
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 */
int spx_http_router_add_route(
    spx_http_router_t *router,
    const spx_http_route_t *route,
    spx_error_t *error
);

/**
 * @brief Add a GET route
 * @param router Router instance
 * @param pattern URI pattern
 * @param handler Handler function
 * @param context Handler context
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 */
int spx_http_router_get(
    spx_http_router_t *router,
    const char *pattern,
    spx_http_handler_func_t handler,
    void *context,
    spx_error_t *error
);

/**
 * @brief Add a POST route
 * @param router Router instance
 * @param pattern URI pattern
 * @param handler Handler function
 * @param context Handler context
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 */
int spx_http_router_post(
    spx_http_router_t *router,
    const char *pattern,
    spx_http_handler_func_t handler,
    void *context,
    spx_error_t *error
);

/**
 * @brief Add a DELETE route
 * @param router Router instance
 * @param pattern URI pattern
 * @param handler Handler function
 * @param context Handler context
 * @param error Output error on failure
 * @return 0 on success, -1 on failure
 */
int spx_http_router_delete(
    spx_http_router_t *router,
    const char *pattern,
    spx_http_handler_func_t handler,
    void *context,
    spx_error_t *error
);

/**
 * @brief Handle an incoming request
 * @param router Router instance
 * @param request Request information
 * @param error Output error on failure
 * @return 0 on success, -1 on error
 */
int spx_http_router_handle(
    spx_http_router_t *router,
    const spx_http_request_t *request,
    spx_error_t *error
);

/*============================================================================
 * Utility Functions
 *============================================================================*/

/**
 * @brief Parse HTTP method from string
 * @param method Method string (e.g., "GET")
 * @return HTTP method enumeration
 */
spx_http_method_t spx_http_parse_method(const char *method);

/**
 * @brief Get HTTP method string
 * @param method HTTP method
 * @return Method string (e.g., "GET")
 */
const char *spx_http_method_string(spx_http_method_t method);

/**
 * @brief Get HTTP status text
 * @param status HTTP status code
 * @return Status text (e.g., "Not Found")
 */
const char *spx_http_status_text(spx_http_status_t status);

/**
 * @brief Detect content type from file extension
 * @param filename Filename or path
 * @return Content type string (e.g., "application/json")
 */
const char *spx_http_detect_content_type(const char *filename);

/**
 * @brief Check if path is safe (no traversal)
 * @param path Path to check
 * @param base_dir Base directory for confinement
 * @param error Output error on failure
 * @return 0 if safe, -1 if unsafe
 */
int spx_http_validate_path(
    const char *path,
    const char *base_dir,
    spx_error_t *error
);

/*============================================================================
 * Pre-built Handlers
 *============================================================================*/

/**
 * @brief Create a static file handler
 * @param base_dir Base directory for files
 * @param index_file Index file for directories (or NULL)
 * @return Handler function
 */
spx_http_handler_func_t spx_http_handler_static_files(
    const char *base_dir,
    const char *index_file
);

/**
 * @brief Create a JSON API handler
 * @return Handler function
 */
spx_http_handler_func_t spx_http_handler_json_api(void);

/**
 * @brief Create a not-found handler
 * @return Handler function
 */
spx_http_handler_func_t spx_http_handler_not_found(void);

#endif /* SPX_HTTP_HANDLER_H_DEFINED */
