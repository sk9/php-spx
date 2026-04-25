# CLAUDE.md - AI Assistant Guide for PHP-SPX

This document provides essential information for AI assistants working with the PHP-SPX codebase.

## Project Overview

**SPX (Simple Profiling eXtension)** is a PHP profiler extension written in C. It provides:
- Low-overhead profiling for PHP 8.3 through 8.5
- Multi-metric collection (22 metrics: time, memory, I/O, GC, etc.)
- Web-based UI for analysis with timeline, flat profile, and flamegraph views
- CLI profiling with instant flat profile output
- Full call stack tracking (unlike Xhprof which aggregates by caller/callee pairs)

**Supported Platforms:** GNU/Linux, macOS, FreeBSD (x86-64 and ARM64 only)

## Repository Structure

```
php-spx/
├── src/                          # C source code
│   ├── php_spx.c                 # Main PHP extension entry point
│   ├── php_spx.h                 # Extension definitions and version
│   ├── spx_profiler.c/h          # Core profiler interface
│   ├── spx_profiler_tracer.c/h   # Tracing profiler implementation
│   ├── spx_profiler_sampler.c/h  # Sampling profiler implementation
│   ├── spx_reporter_*.c/h        # Output reporters (full, fp, trace)
│   ├── spx_metric*.c/h           # Metric collection system
│   ├── spx_php.c/h               # PHP/Zend Engine integration
│   ├── spx_config*.c/h           # Configuration handling
│   ├── spx_resource_stats*.c     # Platform-specific stats (linux/macos/freebsd)
│   └── infrastructure/           # Shared utilities
│       ├── spx_common.h          # Common macros and utilities
│       ├── spx_error.c/h         # Error handling system
│       ├── memory/               # Safe memory allocation
│       ├── security/             # Validation and crypto
│       ├── string/               # Safe string operations
│       └── platform/             # Platform abstraction
├── assets/web-ui/                # Web UI files (HTML, JS, CSS)
├── tests/                        # PHP extension tests (.phpt)
│   ├── unit/                     # C unit tests
│   └── data_dir/                 # Test fixtures
├── config.m4                     # PHP extension build configuration
├── Makefile.frag                 # Custom make rules for asset installation
└── .clang-format                 # Code formatting configuration
```

## Build System

### Building the Extension

```bash
phpize
./configure
make
sudo make install
```

### Build Flags

- `--enable-spx` - Enable the extension (required)
- `--enable-spx-dev` - Add debugging symbols
- `--with-zlib-dir=DIR` - Custom zlib location
- `--with-spx-assets-dir=DIR` - Custom web UI assets path

### Compiler Flags (from config.m4)

```
CFLAGS: -Werror -Wall -O3 -pthread -std=gnu11 -Wno-typedef-redefinition
```

## Coding Standards

### C Style Guidelines

- **Indentation:** 4 spaces
- **Line length:** 100 characters max
- **Braces:** Linux style (opening brace on same line except for functions)
- **Pointer alignment:** Right-aligned (`char *ptr`)
- **Cast spacing:** Space after C-style casts

Format code using: `clang-format -i src/*.c src/*.h`

### Naming Conventions

- **Functions:** `spx_module_action()` (e.g., `spx_profiler_create()`)
- **Types:** `spx_module_type_t` (e.g., `spx_profiler_event_t`)
- **Macros:** `SPX_MACRO_NAME` (e.g., `SPX_METRIC_COUNT`)
- **File names:** `spx_module.c/h` or `spx_module_submodule.c/h`

### Header Guard Pattern

```c
#ifndef SPX_MODULE_H_DEFINED
#define SPX_MODULE_H_DEFINED
/* ... */
#endif /* SPX_MODULE_H_DEFINED */
```

### Common Utility Macros (from spx_common.h)

```c
SPX_LIKELY(x)          // Branch prediction hints
SPX_UNLIKELY(x)
SPX_ARRAY_SIZE(arr)    // Array element count
SPX_MIN(a, b)          // Min/max
SPX_MAX(a, b)
SPX_CLAMP(val, min, max)
SPX_ASSERT(cond)       // Debug assertions (when SPX_DEBUG defined)
```

## Testing

### PHP Extension Tests (.phpt)

Run all tests:
```bash
make test
```

Run specific test:
```bash
php run-tests.php tests/spx_profiler_basic.phpt
```

Test file format:
```
--TEST--
Description of test
--ENV--
SPX_ENABLED=1
--FILE--
<?php /* test code */ ?>
--EXPECTF--
Expected output with %s placeholders
```

### C Unit Tests

```bash
cd tests/unit
make
make run
```

Unit tests cover: limits, metric values, string utils, string pool, hmap, call stack.

## Architecture Overview

### Core Components

1. **php_spx.c** - Extension lifecycle (MINIT, RINIT, RSHUTDOWN, MSHUTDOWN)
2. **spx_profiler.c** - Abstract profiler interface with reporter notification
3. **spx_profiler_tracer.c** - Tracing implementation (hooks every function call)
4. **spx_profiler_sampler.c** - Sampling implementation (periodic collection)
5. **spx_metric.c** - Metric system with registry
6. **spx_reporter_full.c** - Web UI report format (gzipped JSON)
7. **spx_reporter_fp.c** - CLI flat profile output
8. **spx_reporter_trace.c** - Trace file output

### Key Data Structures

```c
// Profiler event notification
typedef struct {
    spx_profiler_event_type_t type;  // CALL_START, CALL_END, FINALIZE
    const int *enabled_metrics;
    size_t called;
    const spx_profiler_metric_values_t *cum;
    size_t depth;
    /* ... */
} spx_profiler_event_t;

// Metric values (22 metrics)
typedef struct {
    double values[SPX_METRIC_COUNT];
} spx_profiler_metric_values_t;
```

### Error Handling (infrastructure/spx_error.h)

```c
spx_error_t error = SPX_ERROR_INIT();
SPX_ERROR_SET(&error, SPX_ERR_INVALID_INPUT, "Invalid value: %d", val);
if (spx_error_has_error(&error)) {
    spx_error_log(&error);
}
```

Error categories: memory, validation, I/O, security, internal.

## PHP Version Compatibility

The code supports PHP 8.3 through 8.5. The Zend API floor is enforced at
`src/php_spx.h` with `ZEND_MODULE_API_NO < 20230831`. The codebase no
longer branches on older PHP versions.

Common API version checks (for reference):
- `20230831` - PHP 8.3 (current floor)
- `20240924` - PHP 8.4
- `20250925` - PHP 8.5

## Web UI Assets

Located in `assets/web-ui/`:
- `index.html` - Control panel
- `report.html` - Analysis screen
- `js/` - JavaScript modules (profileData, widget, callGraph, etc.)
- `css/main.css` - Styles

The UI is served directly by the extension when accessing `?SPX_UI_URI=/`.

## CI/CD Pipeline

GitHub Actions workflow (`.github/workflows/main.yml`):
- Builds on Linux (gcc), Debian (docker), macOS (clang)
- Tests all PHP versions 5.4-8.5
- Creates release artifacts (ZIP with .so + assets)

## Common Development Tasks

### Adding a New Metric

1. Add enum value in `src/spx_metric.h` (before `SPX_METRIC_COUNT`)
2. Add metric info in `src/spx_metric.c` (`spx_metric_info` array)
3. Implement collection handler (typically in `spx_resource_stats*.c`)
4. Add tests

### Adding a New Reporter

1. Create `src/spx_reporter_name.c/h`
2. Implement `spx_profiler_reporter_t` interface
3. Register in `spx_reporter_factory.c`
4. Add to `config.m4` source list
5. Add configuration handling in `spx_config.c`

### Modifying Web UI

1. Edit files in `assets/web-ui/`
2. Test locally by running PHP with profiling enabled
3. Ensure compatibility with recent Chrome and Firefox

## Important Notes

- **Thread Safety:** ZTS (Thread Safe) builds have a small overhead; use `SPX_THREAD_TLS` macro
- **Memory:** Use safe allocation from `infrastructure/memory/spx_alloc_safe.h`
- **Strings:** Use safe string operations from `infrastructure/string/spx_string_safe.h`
- **Security:** Validate all input; see `infrastructure/security/spx_security_validation.h`
- **Performance:** Hot paths use `SPX_HOT` attribute; avoid allocations in tracing callbacks

## Contributing Guidelines

From CONTRIBUTING.md:
- Always open an issue first before creating a PR
- Only compatibility patches and bug fixes will be merged
- Project is experimental; API may change

## License

GPL-3.0 - See LICENSE file for details.
