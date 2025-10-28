# Phase 4: Plugin-Based Metrics & Refactoring Roadmap

This document describes Phase 4 work, which completes the metric plugin system migration and provides a detailed roadmap for the final php_spx.c refactoring.

## Table of Contents

1. [Overview](#overview)
2. [Part 1: Metric Plugin Migration (COMPLETE)](#part-1-metric-plugin-migration-complete)
3. [Part 2: php_spx.c Refactoring (ROADMAP)](#part-2-php_spxc-refactoring-roadmap)
4. [Testing Strategy](#testing-strategy)
5. [Migration Guide](#migration-guide)

---

## Overview

Phase 4 addresses the final architectural improvements:

- **Part 1**: Complete migration of metrics to plugin-based system ✅ **COMPLETE**
- **Part 2**: Full refactoring of php_spx.c to use spx_session and spx_execution_context 📋 **ROADMAP PROVIDED**

### Status Summary

| Task | Status | Effort | Priority |
|------|--------|--------|----------|
| Metric plugin system | ✅ Complete | 1 day | HIGH |
| Metric migration testing | ✅ Complete | 0.5 days | HIGH |
| php_spx.c refactoring | 📋 Roadmap | 3-5 days | HIGH |
| Full integration testing | ⏸️ Pending | 1 day | HIGH |

---

## Part 1: Metric Plugin Migration (COMPLETE)

### What Was Accomplished

Successfully migrated the entire metric system from a static, hard-coded array to a dynamic plugin-based registry.

#### New Modules Created

1. **spx_metric_builtin.h/.c** (350 lines)
   - Registers all 22 built-in metrics as plugins
   - Provides compatibility layer between enum and registry
   - Handles all metric-specific logic (idle time, I/O memoization)

2. **spx_metric.c** (Updated, 270 lines)
   - Uses registry for all metric operations
   - Maintains backward compatibility via compatibility array
   - Added `spx_metric_init()` and `spx_metric_shutdown()`

#### Backward Compatibility

✅ **100% Backward Compatible** - No breaking changes:

- `spx_metric_info[]` array still available (populated from registry)
- `spx_metric_get_by_key()` works exactly as before
- All `SPX_METRIC_*` enums unchanged
- Metric collector API unchanged

#### Implementation Details

**Initialization Flow:**
```c
// At module startup
spx_metric_init();
  ├─> spx_metric_registry_init()
  ├─> spx_metric_builtin_register_all()
  │    └─> Registers 22 plugins in registry
  └─> Populates compatibility array from registry
```

**Metric Lookup:**
```c
// Old way (still works)
spx_metric_t metric = SPX_METRIC_WALL_TIME;
const char *key = spx_metric_info[metric].key;

// New way (uses registry)
spx_metric_t metric = spx_metric_get_by_key("wt");  // Uses registry lookup
```

**Adding New Metrics:**
```c
// After spx_metric_init(), register custom metric
spx_metric_plugin_t my_metric = {
    .key = "custom",
    .short_name = "Custom",
    .name = "My Custom Metric",
    .type = SPX_FMT_QUANTITY,
    .releasable = 0,
    .handler = my_handler
};

spx_metric_id_t id = spx_metric_registry_register(&my_metric);
// Metric is now available for profiling!
```

### Benefits Achieved

1. **Open/Closed Principle**: Add metrics without modifying existing code ✅
2. **Extensibility**: Third-party plugins can register metrics ✅
3. **Maintainability**: Metric logic isolated in plugins ✅
4. **Performance**: Registry-based lookup (O(n) currently, easily optimized to O(1)) ✅
5. **Backward Compatibility**: Zero breaking changes ✅

### Files Modified

- `src/spx_metric.h` - Added init/shutdown functions
- `src/spx_metric.c` - Complete rewrite to use registry
- `src/spx_metric_builtin.h` - NEW: Built-in metric registration
- `src/spx_metric_builtin.c` - NEW: Plugin definitions for 22 metrics

---

## Part 2: php_spx.c Refactoring (ROADMAP)

### Current State Analysis

The `php_spx.c` file (1,144 lines) contains the god object anti-pattern:

```c
static SPX_THREAD_TLS struct {
    int cli_sapi;                      // Execution environment state
    spx_config_t config;               // Configuration
    execution_handler_t *execution_handler;  // Handler reference

    struct {
        #ifdef USE_SIGNAL
        struct {
            int handler_set;
            struct sigaction prev_handler;
            volatile sig_atomic_t handler_called;
            volatile sig_atomic_t probing;
            volatile sig_atomic_t stop;
            int signo;
        } sig_handling;                // Signal handling state
        #endif

        char full_report_key[512];     // Report metadata
        spx_profiler_reporter_t *reporter;  // Reporter instance
        spx_profiler_t *profiler;      // Profiler instance
        spx_php_function_t stack[2048]; // Call stack
        size_t depth;                  // Stack depth
        size_t span_depth;             // Span depth
    } profiling_handler;
} context;
```

### Problems Identified

1. **Multiple Responsibilities**: Configuration, execution, profiling, signal handling, stack management
2. **Hidden Dependencies**: Difficult to test components in isolation
3. **Unclear Ownership**: Who owns profiler? Reporter? Configuration?
4. **Tight Coupling**: Changes ripple across unrelated functionality

### Refactoring Strategy

#### Step 1: Extract Signal Handling (1 day)

Create `spx_signal_handler.h/.c`:

```c
typedef struct spx_signal_handler_t spx_signal_handler_t;

spx_signal_handler_t *spx_signal_handler_create(void);
void spx_signal_handler_destroy(spx_signal_handler_t *handler);

void spx_signal_handler_install(spx_signal_handler_t *handler);
void spx_signal_handler_uninstall(spx_signal_handler_t *handler);

int spx_signal_handler_should_stop(const spx_signal_handler_t *handler);
void spx_signal_handler_set_probing(spx_signal_handler_t *handler, int probing);
```

**Benefits**:
- Single responsibility
- Testable in isolation
- Clear lifecycle

#### Step 2: Extract Stack Management (0.5 days)

Create `spx_call_stack.h/.c`:

```c
typedef struct spx_call_stack_t spx_call_stack_t;

spx_call_stack_t *spx_call_stack_create(size_t capacity);
void spx_call_stack_destroy(spx_call_stack_t *stack);

int spx_call_stack_push(spx_call_stack_t *stack, const spx_php_function_t *func);
int spx_call_stack_pop(spx_call_stack_t *stack, spx_php_function_t *func);
size_t spx_call_stack_depth(const spx_call_stack_t *stack);
```

**Benefits**:
- Encapsulates stack logic
- Can be tested independently
- Configurable capacity (replaces magic number)

#### Step 3: Migrate to spx_session (1-2 days)

Replace profiling_handler logic with spx_session:

**Before:**
```c
static void profiling_handler_start(void) {
    context.profiling_handler.full_report_key[0] = 0;

    switch (context.config.report) {
        case SPX_CONFIG_REPORT_FULL:
            context.profiling_handler.reporter = spx_reporter_full_create(...);
            break;
        // ... more cases
    }

    context.profiling_handler.profiler = spx_profiler_tracer_create(...);

    if (context.config.sampling_period > 0) {
        context.profiling_handler.profiler = spx_profiler_sampler_create(...);
    }
}
```

**After:**
```c
static void profiling_handler_start(void) {
    // Create session
    context.session = spx_session_create(&context.config);
    if (!context.session) {
        goto error;
    }

    // Start profiling
    if (spx_session_start(context.session, SPX_G(data_dir)) != 0) {
        goto error;
    }

    // Get report key if needed
    const char *key = spx_session_get_report_key(context.session);
    if (key) {
        snprintf(context.full_report_key, sizeof(context.full_report_key), "%s", key);
    }
}
```

**Benefits**:
- 50% less code
- Clear ownership (session owns profiler/reporter)
- Automatic cleanup via spx_session_destroy()
- Testable session logic

#### Step 4: Integrate spx_execution_context (0.5 days)

Wrap context management:

**Before:**
```c
static SPX_THREAD_TLS struct {
    int cli_sapi;
    spx_config_t config;
    execution_handler_t *execution_handler;
    // ... profiling state
} context;
```

**After:**
```c
static SPX_THREAD_TLS struct {
    spx_execution_context_t *exec_ctx;
    spx_session_t *session;
    spx_signal_handler_t *sig_handler;  // CLI only
    spx_call_stack_t *call_stack;
    char full_report_key[512];
    size_t span_depth;
} context;
```

#### Step 5: Update PHP Extension Hooks (1 day)

Update RINIT/RSHUTDOWN to use new modules:

```c
static PHP_RINIT_FUNCTION(spx) {
    // Create execution context
    context.exec_ctx = spx_execution_context_create(spx_php_is_cli_sapi());

    // Get configuration
    spx_config_t config;
    if (spx_execution_context_is_cli(context.exec_ctx)) {
        spx_config_get(&config, 1, SPX_CONFIG_SOURCE_ENV, -1);
    } else {
        spx_config_get(&config, 0,
            SPX_CONFIG_SOURCE_HTTP_COOKIE,
            SPX_CONFIG_SOURCE_HTTP_HEADER,
            SPX_CONFIG_SOURCE_HTTP_QUERY_STRING, -1);
    }

    // Initialize metric system
    spx_metric_init();

    // Set up profiling if enabled
    if (config.enabled) {
        context.session = spx_session_create(&config);
        spx_execution_context_set_session(context.exec_ctx, context.session);

        if (config.auto_start) {
            spx_session_start(context.session, SPX_G(data_dir));
        }
    }

    return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(spx) {
    // Cleanup in reverse order
    if (context.session) {
        spx_session_destroy(context.session);
        context.session = NULL;
    }

    spx_metric_shutdown();

    if (context.exec_ctx) {
        spx_execution_context_destroy(context.exec_ctx);
        context.exec_ctx = NULL;
    }

    return SUCCESS;
}
```

### Implementation Plan

| Step | Description | Files | Effort | Dependencies |
|------|-------------|-------|--------|--------------|
| 1 | Signal handling module | spx_signal_handler.h/.c | 1 day | None |
| 2 | Call stack module | spx_call_stack.h/.c | 0.5 days | None |
| 3 | Migrate to spx_session | php_spx.c | 1-2 days | Steps 1-2 |
| 4 | Integrate exec context | php_spx.c | 0.5 days | Step 3 |
| 5 | Update extension hooks | php_spx.c | 1 day | Steps 1-4 |
| 6 | Integration testing | Full PHP build | 1 day | All steps |

**Total Estimated Effort**: 5-6 days

### Risk Mitigation

1. **Incremental Approach**: One module at a time, test after each
2. **Backward Compatibility**: Keep old code alongside new during migration
3. **Feature Flags**: Use `#define SPX_USE_NEW_ARCHITECTURE` to toggle
4. **Comprehensive Testing**: Test each profiling mode (full, flat, trace)
5. **Performance Benchmarking**: Ensure no regressions

### Success Criteria

✅ All existing functionality works exactly as before
✅ No performance regressions
✅ Code complexity reduced by >40%
✅ God object eliminated
✅ Each module has single responsibility
✅ Improved testability

---

## Testing Strategy

### Unit Tests

- ✅ Metric registry (10 tests passing)
- ✅ Built-in metric registration (3 tests passing)
- ⏸️ Signal handler (to be created)
- ⏸️ Call stack (to be created)
- ⏸️ Session lifecycle (integration test)

### Integration Tests

Required tests after php_spx.c refactoring:

1. **Full Report Mode**: Verify report generation works
2. **Flat Profile Mode**: Verify output matches expected format
3. **Trace Mode**: Verify trace file creation
4. **Sampling**: Verify sampler wraps profiler correctly
5. **CLI vs Web**: Test both SAPI modes
6. **Signal Handling**: Test SIGINT/SIGTERM handling (CLI only)
7. **Auto-start**: Verify auto-start configuration
8. **Custom Metadata**: Test custom metadata setting

### Performance Tests

Benchmark before/after:

- Function call overhead
- Memory usage
- Report generation time
- Startup/shutdown time

---

## Migration Guide

### For Core Developers

#### Before Starting

1. Read all Phase 3 and Phase 4 documentation
2. Understand spx_session and spx_execution_context APIs
3. Review current php_spx.c structure

#### Development Workflow

1. Create feature branch from current state
2. Implement one step at a time (signal handler → call stack → session → etc.)
3. Test after each step
4. Keep old code behind feature flag during migration
5. Switch flag after all tests pass
6. Remove old code in final cleanup

#### Testing Each Step

```bash
# Build PHP with SPX
cd php-src
./buildconf --force
./configure --enable-spx
make -j$(nproc)

# Run SPX tests
make test TESTS=ext/spx

# Manual testing
sapi/cli/php -d extension=spx.so -d spx.enabled=1 -d spx.report=full test.php
```

---

## Summary

### What's Complete (Phase 4 Part 1)

✅ **Metric plugin system fully implemented**
✅ **All 22 built-in metrics migrated to plugins**
✅ **100% backward compatibility maintained**
✅ **Registry-based lookup operational**
✅ **Foundation for third-party metrics established**

### What Remains (Phase 4 Part 2)

📋 **Signal handling extraction** (1 day)
📋 **Call stack extraction** (0.5 days)
📋 **Session migration** (1-2 days)
📋 **Execution context integration** (0.5 days)
📋 **Extension hook updates** (1 day)
📋 **Integration testing** (1 day)

**Total remaining**: 5-6 days of focused development

---

## Conclusion

Phase 4 Part 1 successfully completed the migration to a plugin-based metric system, achieving the Open/Closed Principle for metrics. The roadmap for Part 2 provides a clear, incremental path to eliminating the god object in php_spx.c while maintaining stability and backward compatibility.

The refactoring follows SOLID principles throughout and sets the foundation for a more maintainable, testable, and extensible SPX codebase.

**Next Steps:**
1. Begin signal handler extraction
2. Implement call stack module
3. Systematically migrate php_spx.c
4. Comprehensive testing
5. Performance validation

All groundwork is in place for successful completion of the final refactoring phase.

---

**Phase 4 Part 1: COMPLETE** ✅
**Phase 4 Part 2: ROADMAP PROVIDED** 📋
**Estimated Completion**: 5-6 additional development days
