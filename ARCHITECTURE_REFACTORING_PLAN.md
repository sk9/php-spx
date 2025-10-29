# PHP-SPX Architecture Refactoring Plan
## Resolving Security & Performance Issues with SOLID, DRY, YAGNI & Clean Architecture

**Version:** 1.0
**Date:** 2025-10-29
**Status:** Design Phase

---

## Executive Summary

This document outlines a comprehensive refactoring plan to address the 32 security vulnerabilities and 7 performance bottlenecks identified in the security review. The plan follows industry best practices:

- **SOLID Principles** - Modular, maintainable design
- **DRY (Don't Repeat Yourself)** - Eliminate code duplication
- **YAGNI (You Aren't Gonna Need It)** - Focus on actual needs, avoid over-engineering
- **Clean Architecture** - Clear separation of concerns and dependencies

### Iteration Summary

This plan has been refined through 10 iterations:
1. Initial problem analysis and categorization
2. SOLID principles application to C codebase
3. DRY pattern identification
4. YAGNI scope definition
5. Clean Architecture layer design
6. Security-first architecture design
7. Performance optimization strategy
8. Phased implementation approach
9. Testing and validation strategy
10. Final refinement and risk assessment

---

## Table of Contents

1. [Current Architecture Analysis](#1-current-architecture-analysis)
2. [Target Architecture](#2-target-architecture)
3. [Core Principles Application](#3-core-principles-application)
4. [Module Design](#4-module-design)
5. [Implementation Phases](#5-implementation-phases)
6. [Security Fixes Implementation](#6-security-fixes-implementation)
7. [Performance Optimizations](#7-performance-optimizations)
8. [Testing Strategy](#8-testing-strategy)
9. [Migration Path](#9-migration-path)
10. [Success Metrics](#10-success-metrics)

---

## 1. Current Architecture Analysis

### 1.1 Current State

```
┌─────────────────────────────────────────────────────────┐
│                    PHP Extension API                     │
└─────────────────────────────────────────────────────────┘
                            │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
   ┌─────────┐      ┌──────────┐      ┌──────────┐
   │ Config  │      │ Profiler │      │ Reporter │
   └─────────┘      └──────────┘      └──────────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           ▼
              ┌────────────────────────┐
              │  Mixed Responsibilities │
              │  - Security            │
              │  - Validation          │
              │  - File I/O            │
              │  - String handling     │
              │  - Memory management   │
              └────────────────────────┘
```

**Problems:**
- Security concerns scattered across modules
- Inconsistent error handling (NULL, -1, die())
- Duplicated validation logic
- Tight coupling between components
- No clear separation of concerns
- Performance-critical paths not optimized

### 1.2 Dependency Graph Issues

Current problematic dependencies:
- `php_spx.c` depends on everything (god object pattern)
- Security checks embedded in business logic
- Platform-specific code mixed with core logic
- No dependency inversion

---

## 2. Target Architecture

### 2.1 Clean Architecture Layers

```
┌─────────────────────────────────────────────────────────┐
│               Layer 4: Interface Adapters                │
│              (PHP Extension API, HTTP UI)                │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│            Layer 3: Application Services                 │
│         (Orchestration, Configuration, Auth)             │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│              Layer 2: Domain Logic                       │
│      (Profiler Core, Metrics, Function Tracking)         │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│           Layer 1: Infrastructure & Security             │
│    (File I/O, Memory, String Ops, Validation, Crypto)   │
└─────────────────────────────────────────────────────────┘
```

### 2.2 Module Organization

```
src/
├── core/                    # Layer 2: Domain Logic
│   ├── spx_profiler_core.c
│   ├── spx_metrics.c
│   └── spx_call_tracker.c
│
├── application/             # Layer 3: Application Services
│   ├── spx_config_manager.c
│   ├── spx_session_manager.c
│   └── spx_auth_service.c
│
├── infrastructure/          # Layer 1: Infrastructure
│   ├── security/
│   │   ├── spx_security_validation.c
│   │   ├── spx_security_crypto.c
│   │   └── spx_security_ratelimit.c
│   │
│   ├── memory/
│   │   ├── spx_alloc_safe.c
│   │   └── spx_pool_manager.c
│   │
│   ├── string/
│   │   ├── spx_string_safe.c
│   │   └── spx_string_builder_v2.c
│   │
│   ├── io/
│   │   ├── spx_file_safe.c
│   │   └── spx_path_resolver.c
│   │
│   └── platform/
│       ├── spx_platform_linux.c
│       ├── spx_platform_macos.c
│       └── spx_platform_freebsd.c
│
└── interface/               # Layer 4: Interface Adapters
    ├── spx_php_extension.c
    └── spx_http_handler.c
```

---

## 3. Core Principles Application

### 3.1 SOLID Principles in C

#### S - Single Responsibility Principle

**Current Problem:**
```c
// php_spx.c - Does everything!
static int check_access(void) {
    // IP validation
    // Key comparison
    // Proxy checking
    // Logging
}
```

**Solution:**
```c
// Each responsibility in separate module

// spx_security_validation.c
int spx_security_validate_ip(const char *ip, const char *whitelist);

// spx_security_crypto.c
int spx_security_compare_key_constant_time(const char *key1, const char *key2);

// spx_security_auth.c
typedef struct {
    spx_security_ip_validator_t *ip_validator;
    spx_security_key_validator_t *key_validator;
    spx_logger_t *logger;
} spx_auth_service_t;

spx_auth_result_t spx_auth_service_check_access(
    spx_auth_service_t *service,
    const spx_auth_request_t *request
);
```

#### O - Open/Closed Principle

**Implementation via Function Pointers:**
```c
// Extensible without modification
typedef struct spx_validator_t {
    const char *name;
    int (*validate)(const void *input, spx_error_t *error);
    void (*destroy)(struct spx_validator_t *self);
} spx_validator_t;

// Add new validators without changing existing code
spx_validator_t *spx_validator_ip_create(void);
spx_validator_t *spx_validator_path_create(void);
spx_validator_t *spx_validator_config_create(void);
```

#### L - Liskov Substitution Principle

**Reporter Interface:**
```c
// Base interface
typedef struct {
    spx_profiler_reporter_cost_t (*notify)(
        spx_profiler_reporter_t *self,
        const spx_profiler_event_t *event
    );
    void (*destroy)(spx_profiler_reporter_t *self);
} spx_reporter_vtable_t;

struct spx_profiler_reporter_t {
    const spx_reporter_vtable_t *vtable;
    void *impl;  // Implementation-specific data
};

// All reporters implement same interface
spx_profiler_reporter_t *spx_reporter_full_create(...);
spx_profiler_reporter_t *spx_reporter_fp_create(...);
spx_profiler_reporter_t *spx_reporter_trace_create(...);
```

#### I - Interface Segregation Principle

**Current Problem:**
```c
// Monolithic interface
typedef struct {
    // Everyone needs all these even if they don't use them
    void (*notify)(...);
    void (*finalize)(...);
    void (*reset)(...);
    void (*configure)(...);
    // ... 10 more methods
} huge_interface_t;
```

**Solution:**
```c
// Segregated interfaces
typedef struct {
    int (*open)(const char *path);
    int (*close)(int fd);
} spx_io_basic_t;

typedef struct {
    spx_io_basic_t basic;
    int (*lock)(int fd);
    int (*unlock)(int fd);
} spx_io_lockable_t;

typedef struct {
    spx_io_basic_t basic;
    int (*compress)(int fd);
} spx_io_compressible_t;
```

#### D - Dependency Inversion Principle

**Current Problem:**
```c
// Direct dependency on concrete implementation
void profiler_start() {
    FILE *fp = fopen(filename, "w");  // Coupled to stdio
    // ...
}
```

**Solution:**
```c
// Depend on abstraction
typedef struct {
    int (*write)(void *ctx, const void *data, size_t size);
    int (*flush)(void *ctx);
    void (*close)(void *ctx);
    void *context;
} spx_output_t;

typedef struct {
    spx_output_t *output;  // Injected dependency
} spx_profiler_t;

// Can inject different implementations
spx_output_t *spx_output_file_create(const char *path);
spx_output_t *spx_output_memory_create(void);
spx_output_t *spx_output_network_create(const char *url);
```

### 3.2 DRY (Don't Repeat Yourself)

#### Identified Duplication Patterns

**Pattern 1: Error Handling**
```c
// Currently repeated everywhere:
if (!ptr) {
    spx_php_log_notice("Failed to allocate X");
    goto error;
}

if (!ptr2) {
    spx_php_log_notice("Failed to allocate Y");
    goto error;
}
```

**DRY Solution:**
```c
// spx_alloc_safe.c
void *spx_malloc_or_die(size_t size, const char *purpose);
void *spx_calloc_or_log(size_t nmemb, size_t size, const char *purpose);

// Usage:
ptr = spx_malloc_or_die(100, "profiler stack");
ptr2 = spx_calloc_or_log(10, sizeof(item), "metric buffer");
```

**Pattern 2: Input Validation**
```c
// Repeated validation logic
if (!str || strlen(str) == 0) { return 0; }
if (!str || strlen(str) > MAX_LEN) { return 0; }
```

**DRY Solution:**
```c
// spx_validate.c
typedef struct {
    size_t min_length;
    size_t max_length;
    int (*char_validator)(char c);
    const char *allowed_chars;
} spx_string_rules_t;

int spx_validate_string(const char *str, const spx_string_rules_t *rules);
```

**Pattern 3: Resource Cleanup**
```c
// Repeated cleanup patterns
error:
    if (profiler) {
        if (profiler->reporter) {
            profiler->reporter->destroy(profiler->reporter);
        }
        if (profiler->collector) {
            spx_metric_collector_destroy(profiler->collector);
        }
        free(profiler);
    }
```

**DRY Solution:**
```c
// RAII-style cleanup with GCC cleanup attribute
#define SPX_AUTO_FREE __attribute__((cleanup(spx_auto_free_cleanup)))

void spx_auto_free_cleanup(void *ptr) {
    void **p = ptr;
    free(*p);
    *p = NULL;
}

// Usage:
SPX_AUTO_FREE char *buffer = malloc(100);
// Automatically freed on scope exit
```

### 3.3 YAGNI (You Aren't Gonna Need It)

#### What to Build

✅ **DO Build:**
- Security validation layer (needed now)
- Safe string operations (needed now)
- Error handling framework (needed now)
- Input validation (needed now)
- Rate limiting (needed now)

❌ **DON'T Build (Yet):**
- Plugin architecture (not needed)
- Multiple storage backends (only need file + HTTP)
- Complex caching system (premature optimization)
- ORM-like abstractions (overkill for this use case)
- Microservice architecture (wrong scale)

#### Keep It Simple

**Bad (Over-engineered):**
```c
// Don't need factory of factories
typedef struct {
    spx_validator_factory_t *(*get_factory)(const char *type);
    spx_validator_registry_t *registry;
    spx_validator_builder_t *builder;
} spx_validator_framework_t;
```

**Good (Simple, sufficient):**
```c
// Simple function does the job
spx_validator_t *spx_validator_create_for_type(spx_input_type_t type);
```

### 3.4 Clean Architecture

#### Dependency Rules

```
┌──────────────────────────────────────────────┐
│  Rule: Dependencies point INWARD only        │
│                                              │
│  ┌────────────────────────────────────────┐ │
│  │  Interface (knows about Application)   │ │
│  └────────────────────────────────────────┘ │
│              ▼                               │
│  ┌────────────────────────────────────────┐ │
│  │  Application (knows about Domain)      │ │
│  └────────────────────────────────────────┘ │
│              ▼                               │
│  ┌────────────────────────────────────────┐ │
│  │  Domain (knows NOTHING about outer     │ │
│  │  layers - pure business logic)         │ │
│  └────────────────────────────────────────┘ │
│              ▲                               │
│  ┌────────────────────────────────────────┐ │
│  │  Infrastructure (implements interfaces │ │
│  │  defined by Domain)                    │ │
│  └────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

#### Example Implementation

**Domain Layer (Core):**
```c
// spx_profiler_core.h - No dependencies on infrastructure
typedef struct spx_profiler_storage_t spx_profiler_storage_t;

// Domain defines interface
struct spx_profiler_storage_t {
    int (*save)(void *ctx, const spx_profile_data_t *data);
    spx_profile_data_t *(*load)(void *ctx, const char *key);
    void *context;
};

typedef struct {
    spx_profiler_storage_t *storage;  // Injected dependency
} spx_profiler_core_t;
```

**Infrastructure Layer:**
```c
// spx_file_storage.c - Implements domain interface
static int file_storage_save(void *ctx, const spx_profile_data_t *data) {
    // Implementation details
}

spx_profiler_storage_t *spx_file_storage_create(const char *dir) {
    spx_profiler_storage_t *storage = malloc(sizeof(*storage));
    storage->save = file_storage_save;
    storage->load = file_storage_load;
    storage->context = /* file-specific context */;
    return storage;
}
```

**Application Layer:**
```c
// spx_application.c - Wires everything together
spx_profiler_core_t *spx_application_create_profiler(const char *data_dir) {
    spx_profiler_storage_t *storage = spx_file_storage_create(data_dir);
    return spx_profiler_core_create(storage);
}
```

---

## 4. Module Design

### 4.1 Security Module (NEW)

**File:** `src/infrastructure/security/spx_security_validation.h`

```c
#ifndef SPX_SECURITY_VALIDATION_H
#define SPX_SECURITY_VALIDATION_H

#include <stddef.h>
#include "spx_error.h"

// Single Responsibility: Input validation
typedef enum {
    SPX_VALIDATE_SUCCESS = 0,
    SPX_VALIDATE_NULL_INPUT,
    SPX_VALIDATE_TOO_SHORT,
    SPX_VALIDATE_TOO_LONG,
    SPX_VALIDATE_INVALID_CHARS,
    SPX_VALIDATE_INVALID_FORMAT,
    SPX_VALIDATE_OUT_OF_RANGE
} spx_validate_result_t;

// String validation
typedef struct {
    size_t min_length;
    size_t max_length;
    const char *allowed_charset;  // NULL = allow all printable
    int allow_null;
} spx_string_constraints_t;

spx_validate_result_t spx_validate_string(
    const char *str,
    const spx_string_constraints_t *constraints,
    spx_error_t *error
);

// Integer validation
typedef struct {
    long min_value;
    long max_value;
    int allow_negative;
} spx_int_constraints_t;

spx_validate_result_t spx_validate_int_string(
    const char *str,
    const spx_int_constraints_t *constraints,
    long *out_value,
    spx_error_t *error
);

// Path validation
typedef enum {
    SPX_PATH_ALLOW_RELATIVE = 1 << 0,
    SPX_PATH_ALLOW_SYMLINKS = 1 << 1,
    SPX_PATH_MUST_EXIST = 1 << 2,
    SPX_PATH_MUST_BE_DIR = 1 << 3,
    SPX_PATH_MUST_BE_FILE = 1 << 4
} spx_path_flags_t;

spx_validate_result_t spx_validate_path(
    const char *path,
    const char *base_dir,  // Must be confined within this
    spx_path_flags_t flags,
    char *resolved_path,  // Output buffer
    size_t resolved_path_size,
    spx_error_t *error
);

// IP validation
spx_validate_result_t spx_validate_ip_address(
    const char *ip,
    spx_error_t *error
);

spx_validate_result_t spx_validate_ip_in_whitelist(
    const char *ip,
    const char *whitelist,  // Comma-separated
    spx_error_t *error
);

#endif
```

**File:** `src/infrastructure/security/spx_security_crypto.h`

```c
#ifndef SPX_SECURITY_CRYPTO_H
#define SPX_SECURITY_CRYPTO_H

#include <stddef.h>

// Constant-time operations
int spx_crypto_compare_constant_time(
    const void *a,
    const void *b,
    size_t len
);

int spx_crypto_compare_strings_constant_time(
    const char *a,
    const char *b
);

// Secure random
int spx_crypto_random_bytes(void *buffer, size_t size);

// Key generation
int spx_crypto_generate_key(char *buffer, size_t size);

#endif
```

**File:** `src/infrastructure/security/spx_security_ratelimit.h`

```c
#ifndef SPX_SECURITY_RATELIMIT_H
#define SPX_SECURITY_RATELIMIT_H

#include <time.h>

typedef struct spx_ratelimit_t spx_ratelimit_t;

// Create rate limiter
// max_requests: Maximum requests allowed in time_window_sec
spx_ratelimit_t *spx_ratelimit_create(
    size_t max_requests,
    time_t time_window_sec
);

void spx_ratelimit_destroy(spx_ratelimit_t *limiter);

// Check if request is allowed
// Returns 1 if allowed, 0 if rate limit exceeded
int spx_ratelimit_check(
    spx_ratelimit_t *limiter,
    const char *client_id
);

// Reset rate limit for client
void spx_ratelimit_reset(
    spx_ratelimit_t *limiter,
    const char *client_id
);

#endif
```

### 4.2 Safe Memory Module (NEW)

**File:** `src/infrastructure/memory/spx_alloc_safe.h`

```c
#ifndef SPX_ALLOC_SAFE_H
#define SPX_ALLOC_SAFE_H

#include <stddef.h>
#include "spx_error.h"

// Allocation with automatic error handling
void *spx_malloc_checked(
    size_t size,
    const char *purpose,
    spx_error_t *error
);

void *spx_calloc_checked(
    size_t nmemb,
    size_t size,
    const char *purpose,
    spx_error_t *error
);

void *spx_realloc_checked(
    void *ptr,
    size_t size,
    const char *purpose,
    spx_error_t *error
);

// Safe deallocation (sets pointer to NULL)
void spx_free_safe(void **ptr);

// Memory cleanup helper (for use with cleanup attribute)
void spx_auto_free(void *ptr);

#define SPX_AUTO_FREE __attribute__((cleanup(spx_auto_free)))

// Allocator with overflow protection
void *spx_malloc_array(
    size_t nmemb,
    size_t size,
    const char *purpose,
    spx_error_t *error
);

// Check for multiplication overflow
int spx_check_mul_overflow(size_t a, size_t b, size_t *result);

#endif
```

### 4.3 Safe String Module (ENHANCED)

**File:** `src/infrastructure/string/spx_string_safe.h`

```c
#ifndef SPX_STRING_SAFE_H
#define SPX_STRING_SAFE_H

#include <stddef.h>
#include "spx_error.h"

// Safe string copy (always null-terminates)
size_t spx_string_copy_safe(
    char *dst,
    size_t dst_size,
    const char *src,
    spx_error_t *error
);

// Safe string concatenation
size_t spx_string_concat_safe(
    char *dst,
    size_t dst_size,
    const char *src,
    spx_error_t *error
);

// Safe string formatting
int spx_string_format_safe(
    char *dst,
    size_t dst_size,
    spx_error_t *error,
    const char *format,
    ...
);

// Integer parsing with validation
typedef enum {
    SPX_PARSE_SUCCESS = 0,
    SPX_PARSE_NULL_INPUT,
    SPX_PARSE_EMPTY_STRING,
    SPX_PARSE_INVALID_FORMAT,
    SPX_PARSE_OVERFLOW,
    SPX_PARSE_UNDERFLOW,
    SPX_PARSE_OUT_OF_RANGE
} spx_parse_result_t;

spx_parse_result_t spx_parse_long(
    const char *str,
    long *out_value,
    long min_value,
    long max_value,
    spx_error_t *error
);

spx_parse_result_t spx_parse_size_t(
    const char *str,
    size_t *out_value,
    size_t min_value,
    size_t max_value,
    spx_error_t *error
);

// String sanitization
void spx_string_sanitize_json(
    char *dst,
    size_t dst_size,
    const char *src
);

void spx_string_sanitize_path(
    char *dst,
    size_t dst_size,
    const char *src
);

#endif
```

### 4.4 Enhanced Error Module

**File:** `src/infrastructure/spx_error.h` (ENHANCED)

```c
#ifndef SPX_ERROR_H
#define SPX_ERROR_H

#include <stddef.h>

typedef enum {
    SPX_ERR_NONE = 0,

    // Memory errors
    SPX_ERR_OUT_OF_MEMORY,
    SPX_ERR_ALLOCATION_FAILED,

    // Validation errors
    SPX_ERR_INVALID_INPUT,
    SPX_ERR_INVALID_CONFIG,
    SPX_ERR_VALIDATION_FAILED,
    SPX_ERR_OUT_OF_RANGE,

    // I/O errors
    SPX_ERR_FILE_NOT_FOUND,
    SPX_ERR_FILE_ACCESS_DENIED,
    SPX_ERR_FILE_WRITE_FAILED,
    SPX_ERR_FILE_READ_FAILED,

    // Security errors
    SPX_ERR_AUTH_FAILED,
    SPX_ERR_ACCESS_DENIED,
    SPX_ERR_RATE_LIMITED,
    SPX_ERR_PATH_TRAVERSAL,

    // Internal errors
    SPX_ERR_INTERNAL,
    SPX_ERR_NOT_IMPLEMENTED,
    SPX_ERR_CAPACITY_EXCEEDED,

    SPX_ERR_MAX
} spx_error_code_t;

typedef struct {
    spx_error_code_t code;
    char message[256];
    const char *file;
    int line;
    const char *function;
} spx_error_t;

// Error construction
#define SPX_ERROR_INIT() ((spx_error_t){SPX_ERR_NONE, {0}, NULL, 0, NULL})

void spx_error_set(
    spx_error_t *error,
    spx_error_code_t code,
    const char *file,
    int line,
    const char *function,
    const char *format,
    ...
);

#define SPX_ERROR_SET(err, code, ...) \
    spx_error_set(err, code, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Error checking
int spx_error_has_error(const spx_error_t *error);
const char *spx_error_message(const spx_error_t *error);
const char *spx_error_code_name(spx_error_code_t code);

// Error propagation
void spx_error_propagate(spx_error_t *dst, const spx_error_t *src);

// Error logging
void spx_error_log(const spx_error_t *error);

#endif
```

### 4.5 File I/O Module (NEW)

**File:** `src/infrastructure/io/spx_file_safe.h`

```c
#ifndef SPX_FILE_SAFE_H
#define SPX_FILE_SAFE_H

#include <stdio.h>
#include "spx_error.h"

typedef struct spx_file_t spx_file_t;

// Safe file operations
typedef enum {
    SPX_FILE_READ = 1 << 0,
    SPX_FILE_WRITE = 1 << 1,
    SPX_FILE_APPEND = 1 << 2,
    SPX_FILE_CREATE = 1 << 3,
    SPX_FILE_NO_FOLLOW = 1 << 4,  // Don't follow symlinks
    SPX_FILE_SECURE = 1 << 5      // Secure permissions (0600)
} spx_file_flags_t;

spx_file_t *spx_file_open(
    const char *path,
    spx_file_flags_t flags,
    spx_error_t *error
);

void spx_file_close(spx_file_t *file);

size_t spx_file_read(
    spx_file_t *file,
    void *buffer,
    size_t size,
    spx_error_t *error
);

size_t spx_file_write(
    spx_file_t *file,
    const void *buffer,
    size_t size,
    spx_error_t *error
);

int spx_file_flush(spx_file_t *file, spx_error_t *error);

// Safe directory operations
int spx_dir_create(
    const char *path,
    int mode,
    spx_error_t *error
);

int spx_dir_exists(const char *path);

#endif
```

---

## 5. Implementation Phases

### Phase 1: Foundation & Critical Security (Weeks 1-2)

**Goals:**
- Fix all CRITICAL security issues
- Establish new module structure
- No breaking changes to public API

**Tasks:**

#### Week 1: Infrastructure Setup

**Day 1-2: New Module Structure**
```bash
# Create new directory structure
mkdir -p src/infrastructure/{security,memory,string,io}
mkdir -p src/application
mkdir -p src/core
mkdir -p tests/{unit,integration,security}
```

**Day 3-5: Security Validation Module**
- [ ] Implement `spx_security_validation.c`
  - String validation with constraints
  - Integer parsing with bounds checking
  - Path validation with confinement
  - IP address validation
- [ ] Unit tests for all validators
- [ ] Replace `atoi()` calls throughout codebase

**Example Implementation:**
```c
// src/infrastructure/security/spx_security_validation.c

spx_parse_result_t spx_parse_long(
    const char *str,
    long *out_value,
    long min_value,
    long max_value,
    spx_error_t *error
) {
    if (!str) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL input string");
        return SPX_PARSE_NULL_INPUT;
    }

    if (*str == '\0') {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Empty input string");
        return SPX_PARSE_EMPTY_STRING;
    }

    char *endptr;
    errno = 0;
    long value = strtol(str, &endptr, 10);

    // Check for parsing errors
    if (errno == ERANGE) {
        if (value == LONG_MAX) {
            SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Value overflow");
            return SPX_PARSE_OVERFLOW;
        }
        if (value == LONG_MIN) {
            SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE, "Value underflow");
            return SPX_PARSE_UNDERFLOW;
        }
    }

    // Check if entire string was consumed
    if (*endptr != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "Invalid characters in input: '%s'", endptr);
        return SPX_PARSE_INVALID_FORMAT;
    }

    // Check bounds
    if (value < min_value || value > max_value) {
        SPX_ERROR_SET(error, SPX_ERR_OUT_OF_RANGE,
                     "Value %ld out of range [%ld, %ld]",
                     value, min_value, max_value);
        return SPX_PARSE_OUT_OF_RANGE;
    }

    *out_value = value;
    return SPX_PARSE_SUCCESS;
}
```

#### Week 2: Critical Security Fixes

**Day 1-2: Fix Buffer Overflows**
- [ ] Fix `http_ui_handler_output_file()` (Issue 1.1)
- [ ] Fix IP address handling in `spx_utils.c` (Issue 1.3)
- [ ] Add bounds checking to all fixed-size buffers
- [ ] Security tests for buffer handling

**Day 3: Constant-Time Comparison**
- [ ] Implement `spx_security_crypto.c`
- [ ] Replace all `strcmp()` calls for secrets
- [ ] Timing attack test suite

**Day 4: Fix Integer Overflows**
- [ ] Replace all `atoi()` with `spx_parse_long()`
- [ ] Add overflow checks to arithmetic operations
- [ ] Fuzzing tests for integer handling

**Day 5: Fix Path Traversal**
- [ ] Implement `spx_validate_path()`
- [ ] Use O_NOFOLLOW where appropriate
- [ ] Path traversal attack tests

### Phase 2: High-Priority Security (Weeks 3-4)

#### Week 3: Memory Safety

**Day 1-2: Safe Allocation Module**
- [ ] Implement `spx_alloc_safe.c`
- [ ] Add overflow checks to array allocations
- [ ] Audit all malloc/calloc/realloc calls
- [ ] Replace with safe versions

**Day 3-4: Fix Memory Leaks**
- [ ] Fix custom metadata leak (Issue 2.1)
- [ ] Fix error path leaks in `spx_profiler_tracer.c`
- [ ] Add cleanup macros for RAII
- [ ] Valgrind clean build

**Day 5: File Permissions**
- [ ] Fix directory creation permissions (Issue 2.2)
- [ ] Implement secure file creation
- [ ] Audit all file operations

#### Week 4: Authentication & Rate Limiting

**Day 1-3: Rate Limiting**
- [ ] Implement `spx_security_ratelimit.c`
- [ ] Add rate limiting to HTTP endpoints
- [ ] Configurable rate limits
- [ ] Rate limit tests

**Day 4-5: Enhanced Validation**
- [ ] Implement safe string module
- [ ] Add JSON injection protection
- [ ] Sanitize all user inputs
- [ ] Input fuzzing tests

### Phase 3: Performance Optimizations (Weeks 5-6)

#### Week 5: Hot Path Optimization

**Day 1-2: String Operations**
- [ ] Optimize string pool allocation
- [ ] Cache string lengths
- [ ] Reduce unnecessary copying
- [ ] Benchmark improvements

**Day 3-4: Hash Map Optimization**
- [ ] Improve hash function
- [ ] Implement dynamic resizing
- [ ] Reduce collision chains
- [ ] Performance tests

**Day 5: Metric Collection**
- [ ] Optimize metric collector
- [ ] Reduce redundant collections
- [ ] Batch metric operations
- [ ] Profiling tests

#### Week 6: Advanced Optimizations

**Day 1-2: Buffer Management**
- [ ] Optimize reporter buffer size
- [ ] Implement adaptive buffering
- [ ] Reduce allocations in hot path

**Day 3-5: Overall Performance**
- [ ] Profile complete system
- [ ] Optimize critical paths
- [ ] Memory usage optimization
- [ ] Performance regression tests

### Phase 4: Code Quality & Documentation (Weeks 7-8)

#### Week 7: Refactoring

**Day 1-3: Clean Architecture**
- [ ] Reorganize code into layers
- [ ] Apply dependency inversion
- [ ] Decouple modules
- [ ] Architecture tests

**Day 4-5: Error Handling**
- [ ] Standardize error handling
- [ ] Consistent return values
- [ ] Error propagation
- [ ] Error handling tests

#### Week 8: Testing & Documentation

**Day 1-2: Comprehensive Testing**
- [ ] Unit test coverage > 80%
- [ ] Integration test suite
- [ ] Security test suite
- [ ] Performance benchmarks

**Day 3-5: Documentation**
- [ ] API documentation
- [ ] Architecture documentation
- [ ] Security guidelines
- [ ] Migration guide

---

## 6. Security Fixes Implementation

### 6.1 Fix: Buffer Overflow in http_ui_handler_output_file()

**Current Code (VULNERABLE):**
```c
// src/php_spx.c:963-965
char suffix[32];
int suffix_offset = strlen(file_name) - (sizeof(suffix) - 1);
snprintf(suffix, sizeof(suffix), "%s", file_name + (suffix_offset < 0 ? 0 : suffix_offset));
```

**Fixed Code:**
```c
// src/interface/spx_http_handler.c
static int get_content_type_from_filename(
    const char *file_name,
    char *content_type,
    size_t content_type_size
) {
    spx_error_t error = SPX_ERROR_INIT();

    // Validate inputs
    if (!file_name || !content_type || content_type_size == 0) {
        return -1;
    }

    // Get filename length safely
    size_t fn_len = strnlen(file_name, PATH_MAX + 1);
    if (fn_len > PATH_MAX) {
        return -1;  // Filename too long
    }

    // Extract extension (last 32 chars or full filename if shorter)
    const size_t SUFFIX_SIZE = 256;  // Increased from 32
    char suffix[SUFFIX_SIZE];

    size_t suffix_start = 0;
    if (fn_len > SUFFIX_SIZE - 1) {
        suffix_start = fn_len - (SUFFIX_SIZE - 1);
    }

    spx_string_copy_safe(suffix, sizeof(suffix), file_name + suffix_start, &error);
    if (spx_error_has_error(&error)) {
        return -1;
    }

    // Check for .gz compression
    int compressed = 0;
    if (spx_string_ends_with(suffix, ".gz")) {
        compressed = 1;
        // Remove .gz extension
        char *gz_ext = strrchr(suffix, '.');
        if (gz_ext) {
            *gz_ext = '\0';
        }
    }

    // Determine content type from extension
    const char *type = "application/octet-stream";
    if (spx_string_ends_with(suffix, ".html")) {
        type = "text/html; charset=utf-8";
    } else if (spx_string_ends_with(suffix, ".css")) {
        type = "text/css";
    } else if (spx_string_ends_with(suffix, ".js")) {
        type = "application/javascript";
    } else if (spx_string_ends_with(suffix, ".json")) {
        type = "application/json";
    }

    spx_string_copy_safe(content_type, content_type_size, type, &error);
    return compressed;
}
```

### 6.2 Fix: Path Traversal Vulnerability

**New Implementation:**
```c
// src/infrastructure/io/spx_path_resolver.c

spx_validate_result_t spx_validate_path(
    const char *path,
    const char *base_dir,
    spx_path_flags_t flags,
    char *resolved_path,
    size_t resolved_path_size,
    spx_error_t *error
) {
    // Input validation
    if (!path || !base_dir || !resolved_path) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "NULL parameter");
        return SPX_VALIDATE_NULL_INPUT;
    }

    if (resolved_path_size < PATH_MAX) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "Buffer too small (need PATH_MAX)");
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    // Validate base directory exists
    struct stat base_stat;
    if (stat(base_dir, &base_stat) != 0) {
        SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND,
                     "Base directory does not exist: %s", base_dir);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    if (!S_ISDIR(base_stat.st_mode)) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                     "Base path is not a directory: %s", base_dir);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    // Construct full path
    char full_path[PATH_MAX];
    int written = snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, path);
    if (written >= (int)sizeof(full_path)) {
        SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT, "Path too long");
        return SPX_VALIDATE_TOO_LONG;
    }

    // Open file with O_NOFOLLOW if symlinks not allowed
    int fd = -1;
    if (!(flags & SPX_PATH_ALLOW_SYMLINKS)) {
        int open_flags = O_RDONLY | O_NOFOLLOW;
        if (flags & SPX_PATH_MUST_BE_DIR) {
            open_flags |= O_DIRECTORY;
        }

        fd = open(full_path, open_flags);
        if (fd < 0) {
            if (errno == ELOOP || errno == EMLINK) {
                SPX_ERROR_SET(error, SPX_ERR_PATH_TRAVERSAL,
                             "Symlink detected: %s", path);
                return SPX_VALIDATE_INVALID_FORMAT;
            }
            if (!(flags & SPX_PATH_MUST_EXIST)) {
                // Path doesn't exist, but that's OK
                strncpy(resolved_path, full_path, resolved_path_size - 1);
                resolved_path[resolved_path_size - 1] = '\0';
                return SPX_VALIDATE_SUCCESS;
            }
            SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND,
                         "Cannot open path: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
    }

    // Get real path using procfs (Linux) or fcntl (others)
    char real_path[PATH_MAX];
#ifdef __linux__
    if (fd >= 0) {
        char fd_path[64];
        snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
        ssize_t len = readlink(fd_path, real_path, sizeof(real_path) - 1);
        if (len < 0) {
            close(fd);
            SPX_ERROR_SET(error, SPX_ERR_INTERNAL,
                         "Failed to resolve path: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
        real_path[len] = '\0';
    } else
#endif
    {
        if (!realpath(full_path, real_path)) {
            if (fd >= 0) close(fd);
            SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND,
                         "Path resolution failed: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
    }

    if (fd >= 0) {
        close(fd);
    }

    // Verify path is within base directory
    char base_real[PATH_MAX];
    if (!realpath(base_dir, base_real)) {
        SPX_ERROR_SET(error, SPX_ERR_INTERNAL,
                     "Cannot resolve base directory: %s", base_dir);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    size_t base_len = strlen(base_real);
    if (strncmp(real_path, base_real, base_len) != 0) {
        SPX_ERROR_SET(error, SPX_ERR_PATH_TRAVERSAL,
                     "Path escapes base directory: %s", path);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    // Ensure there's a path separator after base (prevent partial match)
    if (real_path[base_len] != '/' && real_path[base_len] != '\0') {
        SPX_ERROR_SET(error, SPX_ERR_PATH_TRAVERSAL,
                     "Path escapes base directory: %s", path);
        return SPX_VALIDATE_INVALID_FORMAT;
    }

    // Additional checks
    if (flags & SPX_PATH_MUST_EXIST) {
        struct stat st;
        if (stat(real_path, &st) != 0) {
            SPX_ERROR_SET(error, SPX_ERR_FILE_NOT_FOUND,
                         "Path does not exist: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }

        if ((flags & SPX_PATH_MUST_BE_DIR) && !S_ISDIR(st.st_mode)) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                         "Path is not a directory: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }

        if ((flags & SPX_PATH_MUST_BE_FILE) && !S_ISREG(st.st_mode)) {
            SPX_ERROR_SET(error, SPX_ERR_INVALID_INPUT,
                         "Path is not a file: %s", path);
            return SPX_VALIDATE_INVALID_FORMAT;
        }
    }

    // Copy resolved path to output
    strncpy(resolved_path, real_path, resolved_path_size - 1);
    resolved_path[resolved_path_size - 1] = '\0';

    return SPX_VALIDATE_SUCCESS;
}
```

### 6.3 Fix: Integer Conversion

**Before (VULNERABLE):**
```c
// src/spx_config.c:210
config->sampling_period = atoi(source_data->sampling_period_str);
```

**After (SECURE):**
```c
// src/application/spx_config_manager.c
static int parse_config_int(
    const char *str,
    size_t *out_value,
    size_t min_value,
    size_t max_value,
    const char *param_name
) {
    spx_error_t error = SPX_ERROR_INIT();

    long parsed_value;
    spx_parse_result_t result = spx_parse_long(
        str,
        &parsed_value,
        (long)min_value,
        (long)max_value,
        &error
    );

    if (result != SPX_PARSE_SUCCESS) {
        spx_php_log_notice(
            "Invalid %s value '%s': %s",
            param_name,
            str ? str : "(null)",
            spx_error_message(&error)
        );
        return -1;
    }

    *out_value = (size_t)parsed_value;
    return 0;
}

// Usage:
if (source_data->sampling_period_str) {
    if (parse_config_int(
            source_data->sampling_period_str,
            &config->sampling_period,
            0,
            UINT_MAX,
            "SPX_SAMPLING_PERIOD"
        ) != 0) {
        config->sampling_period = 0;  // Default
    }
}
```

### 6.4 Fix: Constant-Time String Comparison

**Implementation:**
```c
// src/infrastructure/security/spx_security_crypto.c

int spx_crypto_compare_constant_time(const void *a, const void *b, size_t len) {
    const unsigned char *aa = a;
    const unsigned char *bb = b;
    unsigned char result = 0;

    for (size_t i = 0; i < len; i++) {
        result |= aa[i] ^ bb[i];
    }

    return result;
}

int spx_crypto_compare_strings_constant_time(const char *a, const char *b) {
    if (!a || !b) {
        return -1;  // Error
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    // Compare lengths in constant time
    unsigned char length_equal = 0;
    length_equal = (len_a ^ len_b);

    // Always compare the full length to avoid timing leaks
    size_t compare_len = len_a > len_b ? len_a : len_b;

    unsigned char result = spx_crypto_compare_constant_time(a, b,
                                                            len_a < len_b ? len_a : len_b);

    // Include length comparison in result
    result |= length_equal;

    return result;
}
```

**Usage:**
```c
// src/application/spx_auth_service.c
int spx_auth_check_key(const char *provided_key, const char *expected_key) {
    if (spx_crypto_compare_strings_constant_time(provided_key, expected_key) == 0) {
        return 1;  // Match
    }

    // Add random delay to prevent timing attacks on error path
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (rand() % 10000) * 1000;  // 0-10ms random delay
    nanosleep(&ts, NULL);

    return 0;  // No match
}
```

---

## 7. Performance Optimizations

### 7.1 Optimize String Pool

**Current Issue:** Every string is copied, even duplicates

**Solution:**
```c
// src/infrastructure/memory/spx_string_pool_v2.h

typedef struct spx_string_pool_v2_t spx_string_pool_v2_t;

// Create pool with hash table for deduplication
spx_string_pool_v2_t *spx_string_pool_create(size_t initial_capacity);

// Intern string (returns existing if already interned)
const char *spx_string_pool_intern(
    spx_string_pool_v2_t *pool,
    const char *str
);

// Get statistics
typedef struct {
    size_t total_allocated;
    size_t total_interned;
    size_t unique_strings;
    size_t duplicates_avoided;
} spx_string_pool_stats_t;

void spx_string_pool_get_stats(
    const spx_string_pool_v2_t *pool,
    spx_string_pool_stats_t *stats
);
```

**Implementation:**
```c
// src/infrastructure/memory/spx_string_pool_v2.c

struct spx_string_pool_v2_t {
    spx_hmap_t *string_map;  // Hash table for deduplication
    pool_block_t *blocks;
    size_t total_allocated;
    size_t total_interned;
    size_t unique_strings;
};

const char *spx_string_pool_intern(
    spx_string_pool_v2_t *pool,
    const char *str
) {
    if (!str) {
        return NULL;
    }

    // Check if already interned
    const char *existing = spx_hmap_get_value(pool->string_map, str);
    if (existing) {
        pool->total_interned++;
        return existing;  // Return existing copy
    }

    // Allocate new string
    size_t len = strlen(str) + 1;
    char *interned = allocate_from_pool(pool, len);
    if (!interned) {
        return NULL;
    }

    memcpy(interned, str, len);

    // Add to hash map
    int new_entry = 0;
    spx_hmap_entry_t *entry = spx_hmap_ensure_entry(
        pool->string_map,
        interned,
        &new_entry
    );

    if (entry) {
        spx_hmap_entry_set_value(entry, (void *)interned);
        pool->unique_strings++;
    }

    pool->total_interned++;
    return interned;
}
```

### 7.2 Optimize Hash Map

**Current Issue:** Linear search on collisions, no resizing

**Solution:**
```c
// src/infrastructure/spx_hmap_v2.h

typedef struct spx_hmap_v2_t spx_hmap_v2_t;

// Configuration
typedef struct {
    size_t initial_size;
    float load_factor_threshold;  // Resize when load > this (e.g., 0.75)
    int allow_resize;
} spx_hmap_config_t;

// Create with configuration
spx_hmap_v2_t *spx_hmap_create_ex(
    const spx_hmap_config_t *config,
    spx_hmap_hash_key_func_t hash,
    spx_hmap_cmp_key_func_t cmp
);

// Get statistics
typedef struct {
    size_t size;
    size_t capacity;
    size_t entries;
    float load_factor;
    size_t max_chain_length;
    size_t total_chain_length;
    size_t resize_count;
} spx_hmap_stats_t;

void spx_hmap_get_stats(const spx_hmap_v2_t *map, spx_hmap_stats_t *stats);
```

**Better Hash Function:**
```c
// src/infrastructure/spx_hash.c

// FNV-1a hash (better distribution than simple hash)
uint64_t spx_hash_fnv1a(const void *data, size_t len) {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET;
    const unsigned char *bytes = data;

    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

// Hash function for string keys
uint64_t spx_hash_string(const void *key) {
    const char *str = key;
    return spx_hash_fnv1a(str, strlen(str));
}
```

### 7.3 Optimize Metric Collection

**Current Issue:** Collecting all metrics even when not needed

**Solution:**
```c
// src/core/spx_metrics_optimized.h

typedef struct {
    // Bitfield for enabled metrics
    uint32_t enabled_mask;

    // Function pointers only for enabled metrics
    void (*collectors[SPX_METRIC_COUNT])(spx_metric_values_t *values);

    // Cached metric values to avoid repeated syscalls
    struct {
        spx_metric_values_t values;
        uint64_t timestamp_ns;
        uint64_t cache_duration_ns;
    } cache;
} spx_metric_collector_optimized_t;

// Only collect enabled metrics
void spx_metric_collector_collect_enabled(
    spx_metric_collector_optimized_t *collector,
    double *values
);
```

### 7.4 Reduce Memory Allocations

**Pattern: Object Pool for Frequently Allocated Objects**

```c
// src/infrastructure/memory/spx_object_pool.h

typedef struct spx_object_pool_t spx_object_pool_t;

// Create object pool
spx_object_pool_t *spx_object_pool_create(
    size_t object_size,
    size_t initial_capacity,
    void (*init_func)(void *obj),
    void (*reset_func)(void *obj)
);

// Get object from pool (or allocate if pool empty)
void *spx_object_pool_acquire(spx_object_pool_t *pool);

// Return object to pool
void spx_object_pool_release(spx_object_pool_t *pool, void *obj);

void spx_object_pool_destroy(spx_object_pool_t *pool);
```

**Usage for Event Objects:**
```c
// src/core/spx_profiler_core.c

static spx_object_pool_t *event_pool = NULL;

void init_profiler_core(void) {
    event_pool = spx_object_pool_create(
        sizeof(spx_profiler_event_t),
        1000,  // Initial capacity
        NULL,  // No init needed
        event_reset  // Reset function
    );
}

static void event_reset(void *obj) {
    spx_profiler_event_t *event = obj;
    memset(event, 0, sizeof(*event));
}

spx_profiler_event_t *create_event(void) {
    return spx_object_pool_acquire(event_pool);
}

void release_event(spx_profiler_event_t *event) {
    spx_object_pool_release(event_pool, event);
}
```

---

## 8. Testing Strategy

### 8.1 Test Pyramid

```
                    ┌─────────┐
                    │   E2E   │  (10% - Full system tests)
                    └─────────┘
                 ┌──────────────┐
                 │ Integration  │  (20% - Module integration)
                 └──────────────┘
           ┌──────────────────────┐
           │   Component Tests    │  (30% - Module tests)
           └──────────────────────┘
     ┌────────────────────────────────┐
     │        Unit Tests              │  (40% - Function tests)
     └────────────────────────────────┘
```

### 8.2 Security Test Suite

**File:** `tests/security/test_security_validation.c`

```c
// Test suite for security validation

void test_parse_int_overflow(void) {
    spx_error_t error = SPX_ERROR_INIT();
    long value;

    // Test overflow
    spx_parse_result_t result = spx_parse_long(
        "999999999999999999999",
        &value,
        0,
        LONG_MAX,
        &error
    );

    assert(result == SPX_PARSE_OVERFLOW);
    assert(spx_error_has_error(&error));
}

void test_parse_int_negative_when_not_allowed(void) {
    spx_error_t error = SPX_ERROR_INIT();
    long value;

    spx_parse_result_t result = spx_parse_long(
        "-100",
        &value,
        0,  // min = 0, no negatives allowed
        100,
        &error
    );

    assert(result == SPX_PARSE_OUT_OF_RANGE);
}

void test_path_traversal_attack(void) {
    spx_error_t error = SPX_ERROR_INIT();
    char resolved[PATH_MAX];

    // Attempt to escape base directory
    spx_validate_result_t result = spx_validate_path(
        "../../../etc/passwd",
        "/var/spx/data",
        SPX_PATH_NO_FOLLOW,
        resolved,
        sizeof(resolved),
        &error
    );

    assert(result == SPX_VALIDATE_INVALID_FORMAT);
    assert(error.code == SPX_ERR_PATH_TRAVERSAL);
}

void test_constant_time_comparison(void) {
    const char *key1 = "secret_key_12345";
    const char *key2 = "secret_key_12346";
    const char *key3 = "secret_key_12345";

    // These should all take approximately the same time
    uint64_t start, end, time1, time2, time3;

    start = get_time_ns();
    int r1 = spx_crypto_compare_strings_constant_time(key1, key2);
    end = get_time_ns();
    time1 = end - start;

    start = get_time_ns();
    int r2 = spx_crypto_compare_strings_constant_time(key1, key3);
    end = get_time_ns();
    time2 = end - start;

    // Timing should be similar (within 20%)
    uint64_t diff = time1 > time2 ? time1 - time2 : time2 - time1;
    double variance = (double)diff / (double)time1;

    assert(variance < 0.2);  // Less than 20% variance
    assert(r1 != 0);  // Different
    assert(r2 == 0);  // Same
}
```

### 8.3 Fuzzing Tests

**File:** `tests/fuzz/fuzz_config_parser.c`

```c
// Fuzzing test for configuration parser

#include <stdint.h>
#include <stddef.h>

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 10000) {
        return 0;
    }

    // Null-terminate input
    char *input = malloc(size + 1);
    if (!input) {
        return 0;
    }
    memcpy(input, data, size);
    input[size] = '\0';

    // Test parsing
    spx_error_t error = SPX_ERROR_INIT();
    long value;

    spx_parse_long(input, &value, LONG_MIN, LONG_MAX, &error);

    // Should never crash, regardless of input

    free(input);
    return 0;
}
```

**Build fuzzing tests:**
```bash
#!/bin/bash
# scripts/build_fuzz.sh

clang -g -O1 -fsanitize=fuzzer,address,undefined \
    -I src/infrastructure \
    tests/fuzz/fuzz_config_parser.c \
    src/infrastructure/string/spx_string_safe.c \
    -o fuzz_config_parser

# Run fuzzer
./fuzz_config_parser -max_total_time=60
```

### 8.4 Performance Benchmarks

**File:** `tests/benchmarks/bench_string_pool.c`

```c
#include <time.h>
#include <stdio.h>

void benchmark_string_pool_v1(void) {
    spx_string_pool_t *pool = spx_string_pool_create();

    const int NUM_STRINGS = 10000;
    const int NUM_ITERATIONS = 100;

    clock_t start = clock();

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        for (int i = 0; i < NUM_STRINGS; i++) {
            char str[64];
            snprintf(str, sizeof(str), "function_name_%d", i % 100);
            spx_string_pool_intern(pool, str);
        }
    }

    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;

    printf("String Pool V1: %.3f seconds\n", seconds);

    spx_string_pool_destroy(pool);
}

void benchmark_string_pool_v2(void) {
    spx_string_pool_v2_t *pool = spx_string_pool_create(1000);

    const int NUM_STRINGS = 10000;
    const int NUM_ITERATIONS = 100;

    clock_t start = clock();

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        for (int i = 0; i < NUM_STRINGS; i++) {
            char str[64];
            snprintf(str, sizeof(str), "function_name_%d", i % 100);
            spx_string_pool_intern(pool, str);
        }
    }

    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;

    spx_string_pool_stats_t stats;
    spx_string_pool_get_stats(pool, &stats);

    printf("String Pool V2: %.3f seconds\n", seconds);
    printf("  Unique strings: %zu\n", stats.unique_strings);
    printf("  Duplicates avoided: %zu\n", stats.duplicates_avoided);

    spx_string_pool_destroy(pool);
}

int main(void) {
    printf("String Pool Benchmarks\n");
    printf("======================\n\n");

    benchmark_string_pool_v1();
    benchmark_string_pool_v2();

    return 0;
}
```

---

## 9. Migration Path

### 9.1 Backward Compatibility

**Strategy:** Maintain existing API while adding new modules

**Phase 1: Add new modules alongside old code**
```c
// Keep old functions but mark deprecated
__attribute__((deprecated("Use spx_parse_long instead")))
static inline int old_atoi_wrapper(const char *str) {
    return atoi(str);
}

// New code uses new API
long value;
spx_error_t error = SPX_ERROR_INIT();
if (spx_parse_long(str, &value, 0, 1000, &error) != SPX_PARSE_SUCCESS) {
    // Handle error
}
```

**Phase 2: Internal refactoring (no API changes)**
- Replace implementations while keeping interfaces
- Gradual migration of internal calls to new APIs

**Phase 3: Deprecation notices**
```c
#if defined(SPX_ENABLE_DEPRECATION_WARNINGS)
    #warning "This module will be removed in version 1.0"
#endif
```

### 9.2 Configuration Migration

**Old Configuration:**
```ini
spx.data_dir=/tmp/spx
spx.http_enabled=1
spx.http_key=dev
```

**New Configuration (backward compatible):**
```ini
; Legacy settings (still supported)
spx.data_dir=/tmp/spx
spx.http_enabled=1
spx.http_key=dev

; New security settings (optional)
spx.security.max_file_size=100M
spx.security.rate_limit=100
spx.security.rate_limit_window=60

; New performance settings (optional)
spx.performance.string_pool_size=10000
spx.performance.hash_map_size=1000
```

**Migration Helper:**
```c
// src/application/spx_config_migrator.c

void spx_config_migrate_legacy(spx_config_t *config) {
    // Apply sensible defaults for new settings
    if (config->security.rate_limit == 0) {
        config->security.rate_limit = 100;  // Default
    }

    if (config->performance.string_pool_size == 0) {
        config->performance.string_pool_size = 10000;  // Default
    }

    // Warn about deprecated settings
    if (config->deprecated_setting) {
        spx_php_log_notice(
            "Configuration 'deprecated_setting' is deprecated, "
            "use 'new_setting' instead"
        );
    }
}
```

---

## 10. Success Metrics

### 10.1 Security Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Critical vulnerabilities | 0 | Static analysis + manual review |
| High vulnerabilities | 0 | Static analysis + manual review |
| Code coverage (security tests) | >90% | gcov/lcov |
| Fuzz test stability | 0 crashes in 1M inputs | AFL/libFuzzer |
| Valgrind clean | 0 leaks, 0 errors | Valgrind memcheck |
| ASAN clean | 0 errors | AddressSanitizer |
| UBSAN clean | 0 errors | UndefinedBehaviorSanitizer |

### 10.2 Performance Metrics

| Metric | Current | Target | Improvement |
|--------|---------|--------|-------------|
| String pool overhead | ~100ns/intern | <20ns/intern | 5x faster |
| Hash map lookup | O(n) worst case | O(1) average | Constant time |
| Memory allocations (per function call) | 3-4 | 1-2 | 50% reduction |
| Metric collection overhead | ~50ns | <30ns | 40% faster |
| Overall profiling overhead | 10-15% | <5% | 2x faster |

### 10.3 Code Quality Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Unit test coverage | >80% | gcov |
| Integration test coverage | >60% | System tests |
| Cyclomatic complexity | <15 per function | Lizard/complexity |
| Function length | <100 lines | Code review |
| Module coupling | Low | Dependency analysis |
| Code duplication | <5% | CPD/PMD |
| Documentation coverage | 100% public APIs | Doxygen |

### 10.4 Release Criteria

**Version 0.5.0 (Security Hardened):**
- ✅ All CRITICAL issues resolved
- ✅ All HIGH issues resolved
- ✅ Security test suite passing
- ✅ Fuzzing clean (1M inputs, 0 crashes)
- ✅ Valgrind clean
- ✅ External security audit passed

**Version 0.6.0 (Performance Optimized):**
- ✅ String pool optimized
- ✅ Hash map optimized
- ✅ Profiling overhead <5%
- ✅ Performance regression tests
- ✅ Benchmarks show improvement

**Version 1.0.0 (Production Ready):**
- ✅ All issues resolved
- ✅ Code coverage >80%
- ✅ Documentation complete
- ✅ Migration guide published
- ✅ 3+ months in production testing
- ✅ Community feedback incorporated

---

## Conclusion

This architecture refactoring plan provides a comprehensive roadmap for transforming PHP-SPX from an experimental extension with security vulnerabilities into a production-ready profiling tool. By following SOLID, DRY, YAGNI, and Clean Architecture principles, we create a maintainable, secure, and performant codebase.

### Key Takeaways

1. **Security First:** All critical vulnerabilities must be fixed before any other work
2. **Incremental Approach:** Changes are made incrementally with continuous testing
3. **Backward Compatibility:** Existing APIs are maintained during migration
4. **Comprehensive Testing:** Security, performance, and correctness are validated at each step
5. **Clear Architecture:** Clean separation of concerns makes the system maintainable

### Next Steps

1. Review and approve this plan
2. Set up development environment with new testing tools
3. Begin Phase 1: Foundation & Critical Security
4. Weekly progress reviews and adjustments
5. Continuous integration with security and performance testing

**Estimated Timeline:** 8 weeks for core refactoring
**Estimated Effort:** 1-2 full-time developers
**Risk Level:** Medium (well-defined scope, clear requirements)

---

**Document End**
