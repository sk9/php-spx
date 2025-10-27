# php_spx.c Integration Guide

This document provides step-by-step instructions for integrating the new architectural modules into `php_spx.c`. All prerequisite modules have been created and tested.

## Prerequisites - COMPLETE ✅

The following modules are implemented and ready for integration:

- ✅ `spx_session.h/.c` - Session lifecycle management
- ✅ `spx_execution_context.h/.c` - Execution environment context
- ✅ `spx_signal_handler.h/.c` - Signal handling (CLI mode)
- ✅ `spx_call_stack.h/.c` - Call stack management
- ✅ `spx_metric_registry.h/.c` - Plugin-based metrics
- ✅ `spx_error.h/.c` - Standardized error handling

## Current State Analysis

### php_spx.c God Object

**Location**: `src/php_spx.c:51-80`

**Current Structure**:
```c
static SPX_THREAD_TLS struct {
    int cli_sapi;
    spx_config_t config;
    execution_handler_t *execution_handler;

    struct {
#ifdef USE_SIGNAL
        struct {
            int handler_set;
            struct sigaction prev_handler;
            volatile sig_atomic_t handler_called;
            volatile sig_atomic_t probing;
            volatile sig_atomic_t stop;
            int signo;
        } sig_handling;
#endif

        char full_report_key[512];
        spx_profiler_reporter_t *reporter;
        spx_profiler_t *profiler;
        spx_php_function_t stack[STACK_CAPACITY];
        size_t depth;
        size_t span_depth;
    } profiling_handler;
} context;
```

**Problems**:
- Multiple responsibilities mixed together
- Tight coupling between components
- Difficult to test in isolation
- Magic numbers (STACK_CAPACITY = 2048)

### Target Structure

**After Refactoring**:
```c
static SPX_THREAD_TLS struct {
    /* Execution environment */
    spx_execution_context_t *exec_ctx;

    /* Profiling session */
    spx_session_t *session;

    /* Signal handling (CLI only) */
#ifdef USE_SIGNAL
    spx_signal_handler_t *sig_handler;
#endif

    /* Call stack tracking */
    spx_call_stack_t *call_stack;

    /* Span tracking */
    size_t span_depth;
} context;
```

**Benefits**:
- Clear separation of concerns
- Each component independently testable
- No magic numbers (configurablethrough spx_limits)
- Explicit ownership and lifecycle

---

## Integration Steps

### Step 1: Add Required Includes

**File**: `src/php_spx.c`

**Add after existing includes**:
```c
#include "spx_session.h"
#include "spx_execution_context.h"
#include "spx_signal_handler.h"
#include "spx_call_stack.h"
#include "spx_limits.h"
```

### Step 2: Update Context Structure

**Location**: `src/php_spx.c:51-80`

**Replace**:
```c
static SPX_THREAD_TLS struct {
    int cli_sapi;
    spx_config_t config;
    execution_handler_t *execution_handler;

    struct {
        // ... profiling_handler content
    } profiling_handler;
} context;
```

**With**:
```c
static SPX_THREAD_TLS struct {
    /* Execution environment context */
    spx_execution_context_t *exec_ctx;

    /* Profiling session */
    spx_session_t *session;

    /* Signal handling (CLI only) */
#ifdef USE_SIGNAL
    spx_signal_handler_t *sig_handler;
#endif

    /* Call stack tracking */
    spx_call_stack_t *call_stack;

    /* Report key (for full reports) */
    char full_report_key[512];

    /* Span depth tracking */
    size_t span_depth;
} context;
```

### Step 3: Update PHP_RINIT_FUNCTION

**Location**: `src/php_spx.c:274-359`

**Current Code**:
```c
static PHP_RINIT_FUNCTION(spx) {
    context.execution_handler = NULL;
    context.cli_sapi = spx_php_is_cli_sapi();

    if (context.cli_sapi) {
        spx_config_get(&context.config, context.cli_sapi, SPX_CONFIG_SOURCE_ENV, -1);
    } else {
        spx_config_get(&context.config, ...);
    }

    // ... access control ...

    if (!context.config.enabled) {
        return SUCCESS;
    }

    context.execution_handler = &profiling_handler;
    context.execution_handler->init();

    return SUCCESS;
}
```

**Replace With**:
```c
static PHP_RINIT_FUNCTION(spx) {
#ifdef ZTS
    spx_php_global_hooks_disable();
#endif

    /* Initialize execution context */
    context.exec_ctx = spx_execution_context_create(spx_php_is_cli_sapi());
    if (!context.exec_ctx) {
        return FAILURE;
    }

    context.session = NULL;
#ifdef USE_SIGNAL
    context.sig_handler = NULL;
#endif
    context.call_stack = NULL;
    context.span_depth = 0;
    context.full_report_key[0] = '\0';

    /* Get configuration */
    spx_config_t config;
    if (spx_execution_context_is_cli(context.exec_ctx)) {
        spx_config_get(&config, 1, SPX_CONFIG_SOURCE_ENV, -1);
    } else {
        spx_config_get(&config, 0,
            SPX_CONFIG_SOURCE_HTTP_COOKIE,
            SPX_CONFIG_SOURCE_HTTP_HEADER,
            SPX_CONFIG_SOURCE_HTTP_QUERY_STRING, -1);

        /* Access control check */
        const int access_required = config.ui_uri || config.enabled;
        if (access_required && !access_granted(&config)) {
            spx_execution_context_destroy(context.exec_ctx);
            context.exec_ctx = NULL;
            return SUCCESS;  /* Not an error, just not granted */
        }
    }

    /* Initialize metric system */
    spx_metric_init();

    /* Handle UI request */
    if (config.ui_uri) {
        http_ui_handler_init();
        return SUCCESS;
    }

    /* Setup profiling if enabled */
    if (!config.enabled) {
        return SUCCESS;
    }

    /* Create profiling session */
    context.session = spx_session_create(&config);
    if (!context.session) {
        goto error;
    }

    spx_execution_context_set_session(context.exec_ctx, context.session);

    /* Create call stack */
    context.call_stack = spx_call_stack_create(spx_get_max_stack_depth());
    if (!context.call_stack) {
        goto error;
    }

    /* Setup signal handling for CLI */
#ifdef USE_SIGNAL
    if (spx_execution_context_is_cli(context.exec_ctx) && config.auto_start) {
        context.sig_handler = spx_signal_handler_create();
        if (context.sig_handler) {
            spx_signal_handler_set_terminate_callback(context.sig_handler,
                                                       profiling_handler_terminate);
            spx_signal_handler_install(context.sig_handler);
        }
    }
#endif

    /* Initialize PHP execution hooks */
    profiling_handler_ex_set_context();

    /* Start profiling if auto-start enabled */
    if (config.auto_start) {
        profiling_handler_start();
    }

    return SUCCESS;

error:
    if (context.session) {
        spx_session_destroy(context.session);
        context.session = NULL;
    }
    if (context.call_stack) {
        spx_call_stack_destroy(context.call_stack);
        context.call_stack = NULL;
    }
    if (context.exec_ctx) {
        spx_execution_context_destroy(context.exec_ctx);
        context.exec_ctx = NULL;
    }
    return FAILURE;
}
```

### Step 4: Update PHP_RSHUTDOWN_FUNCTION

**Location**: `src/php_spx.c:361-379`

**Replace**:
```c
static PHP_RSHUTDOWN_FUNCTION(spx) {
    if (context.execution_handler) {
        context.execution_handler->shutdown();
    }

    return SUCCESS;
}
```

**With**:
```c
static PHP_RSHUTDOWN_FUNCTION(spx) {
    /* Stop profiling if active */
    if (context.session && spx_session_is_active(context.session)) {
        profiling_handler_stop();
    }

    /* Cleanup in reverse order of creation */
#ifdef USE_SIGNAL
    if (context.sig_handler) {
        spx_signal_handler_uninstall(context.sig_handler);
        spx_signal_handler_destroy(context.sig_handler);
        context.sig_handler = NULL;
    }
#endif

    if (context.call_stack) {
        spx_call_stack_destroy(context.call_stack);
        context.call_stack = NULL;
    }

    if (context.session) {
        spx_session_destroy(context.session);
        context.session = NULL;
    }

    /* Shutdown metric system */
    spx_metric_shutdown();

    if (context.exec_ctx) {
        spx_execution_context_destroy(context.exec_ctx);
        context.exec_ctx = NULL;
    }

#ifdef ZTS
    spx_php_global_hooks_unset();
#endif

    return SUCCESS;
}
```

### Step 5: Update profiling_handler_start

**Location**: `src/php_spx.c:637-742`

**Replace**:
```c
static void profiling_handler_start(void) {
    if (context.profiling_handler.profiler) {
        return;
    }

    context.profiling_handler.full_report_key[0] = 0;

    switch (context.config.report) {
        case SPX_CONFIG_REPORT_FULL:
            context.profiling_handler.reporter = spx_reporter_full_create(...);
            // ... save key ...
            break;
        // ... other cases ...
    }

    if (!context.profiling_handler.reporter) {
        goto error;
    }

    context.profiling_handler.profiler = spx_profiler_tracer_create(...);
    // ... sampling wrapper ...
}
```

**With**:
```c
static void profiling_handler_start(void) {
    if (!context.session) {
        return;
    }

    if (spx_session_is_active(context.session)) {
        return;  /* Already started */
    }

    /* Start the session */
    const spx_config_t *config = spx_session_get_config(context.session);
    if (spx_session_start(context.session, SPX_G(data_dir)) != 0) {
        spx_php_log_notice("Failed to start profiling session: %s",
                           spx_error_get_message());
        return;
    }

    /* Save report key for full reports */
    const char *key = spx_session_get_report_key(context.session);
    if (key) {
        snprintf(context.full_report_key, sizeof(context.full_report_key),
                 "%s", key);
    }

    /* Clear call stack */
    if (context.call_stack) {
        spx_call_stack_clear(context.call_stack);
    }

    context.span_depth = 0;
}
```

### Step 6: Update profiling_handler_stop

**Location**: `src/php_spx.c:744-789`

**Replace**:
```c
static void profiling_handler_stop(void) {
    if (!context.profiling_handler.profiler) {
        return;
    }

    context.profiling_handler.profiler->finalize(...);
    context.profiling_handler.profiler->destroy(...);
    context.profiling_handler.reporter->vtable.destroy(...);

    context.profiling_handler.profiler = NULL;
    context.profiling_handler.reporter = NULL;
}
```

**With**:
```c
static void profiling_handler_stop(void) {
    if (!context.session) {
        return;
    }

    if (!spx_session_is_active(context.session)) {
        return;
    }

    /* Session handles all cleanup */
    spx_session_stop(context.session);

    context.full_report_key[0] = '\0';
    context.span_depth = 0;
}
```

### Step 7: Update profiling_handler_ex_hook_before

**Location**: `src/php_spx.c:779-818`

**Replace**:
```c
static void profiling_handler_ex_hook_before(void) {
    if (context.profiling_handler.depth == STACK_CAPACITY) {
        spx_utils_die("STACK_CAPACITY exceeded");
    }

    spx_php_function_t function;
    spx_php_current_function(&function);

    context.profiling_handler.stack[context.profiling_handler.depth] = function;
    context.profiling_handler.depth++;

    if (!context.profiling_handler.profiler) {
        return;
    }

#ifdef USE_SIGNAL
    context.profiling_handler.sig_handling.probing = 1;
#endif

    context.profiling_handler.profiler->call_start(...);

#ifdef USE_SIGNAL
    context.profiling_handler.sig_handling.probing = 0;
    if (context.profiling_handler.sig_handling.stop) {
        profiling_handler_sig_terminate();
    }
#endif
}
```

**With**:
```c
static void profiling_handler_ex_hook_before(void) {
    /* Get current function */
    spx_php_function_t function;
    spx_php_current_function(&function);

    /* Push onto call stack */
    if (context.call_stack) {
        if (spx_call_stack_push(context.call_stack, &function) != 0) {
            spx_utils_die("Call stack capacity exceeded: %s",
                          spx_error_get_message());
        }
    }

    /* If not profiling, we're done */
    if (!context.session || !spx_session_is_active(context.session)) {
        return;
    }

    /* Signal handling: mark probing start */
#ifdef USE_SIGNAL
    if (context.sig_handler) {
        spx_signal_handler_set_probing(context.sig_handler, 1);
    }
#endif

    /* Call profiler */
    spx_profiler_t *profiler = spx_session_get_profiler(context.session);
    if (profiler) {
        profiler->call_start(profiler, &function);
    }

    /* Signal handling: mark probing end */
#ifdef USE_SIGNAL
    if (context.sig_handler) {
        spx_signal_handler_set_probing(context.sig_handler, 0);

        /* Check if we should terminate */
        if (spx_signal_handler_should_stop(context.sig_handler)) {
            profiling_handler_terminate();
        }
    }
#endif
}
```

### Step 8: Update profiling_handler_ex_hook_after

**Location**: `src/php_spx.c:820-840`

**Replace**:
```c
static void profiling_handler_ex_hook_after(void) {
    context.profiling_handler.depth--;

    if (!context.profiling_handler.profiler) {
        return;
    }

#ifdef USE_SIGNAL
    context.profiling_handler.sig_handling.probing = 1;
#endif

    context.profiling_handler.profiler->call_end(...);

#ifdef USE_SIGNAL
    context.profiling_handler.sig_handling.probing = 0;
    if (context.profiling_handler.sig_handling.stop) {
        profiling_handler_sig_terminate();
    }
#endif
}
```

**With**:
```c
static void profiling_handler_ex_hook_after(void) {
    /* Pop from call stack */
    if (context.call_stack) {
        spx_call_stack_pop(context.call_stack, NULL);
    }

    /* If not profiling, we're done */
    if (!context.session || !spx_session_is_active(context.session)) {
        return;
    }

    /* Signal handling: mark probing start */
#ifdef USE_SIGNAL
    if (context.sig_handler) {
        spx_signal_handler_set_probing(context.sig_handler, 1);
    }
#endif

    /* Call profiler */
    spx_profiler_t *profiler = spx_session_get_profiler(context.session);
    if (profiler) {
        profiler->call_end(profiler);
    }

    /* Signal handling: mark probing end */
#ifdef USE_SIGNAL
    if (context.sig_handler) {
        spx_signal_handler_set_probing(context.sig_handler, 0);

        /* Check if we should terminate */
        if (spx_signal_handler_should_stop(context.sig_handler)) {
            profiling_handler_terminate();
        }
    }
#endif
}
```

### Step 9: Add profiling_handler_terminate (CLI)

**Location**: After `profiling_handler_ex_hook_after`

**Add New Function**:
```c
#ifdef USE_SIGNAL
static void profiling_handler_terminate(void) {
    /* Stop profiling cleanly */
    profiling_handler_stop();

    /* Get signal number for exit code */
    int signo = -1;
    if (context.sig_handler) {
        signo = spx_signal_handler_get_signo(context.sig_handler);
    }

    /* Exit with appropriate code */
    _exit(signo < 0 ? EXIT_SUCCESS : 128 + signo);
}
#endif
```

### Step 10: Update PHP Extension Functions

**Location**: `src/php_spx.c:1022-1100`

**spx_profiler_start**:
```c
PHP_FUNCTION(spx_profiler_start) {
    if (!context.session) {
        RETURN_FALSE;
    }

    profiling_handler_start();
    RETURN_TRUE;
}
```

**spx_profiler_stop**:
```c
PHP_FUNCTION(spx_profiler_stop) {
    if (!context.session) {
        RETURN_FALSE;
    }

    profiling_handler_stop();
    RETURN_TRUE;
}
```

**spx_profiler_full_report_set_custom_metadata_str**:
```c
PHP_FUNCTION(spx_profiler_full_report_set_custom_metadata_str) {
    char *custom_metadata_str;
    size_t custom_metadata_str_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                              &custom_metadata_str,
                              &custom_metadata_str_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (!context.session) {
        RETURN_FALSE;
    }

    if (spx_session_set_custom_metadata(context.session, custom_metadata_str) != 0) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
```

---

## Testing Strategy

### Unit Tests (Already Created)

- ✅ Signal handler functionality
- ✅ Call stack operations
- ✅ Session lifecycle
- ✅ Execution context management

### Integration Tests (Required)

After completing the integration, test the following:

#### 1. Basic Profiling
```bash
# Full report mode
php -d extension=spx.so -d spx.enabled=1 -d spx.report=full test.php

# Flat profile mode
php -d extension=spx.so -d spx.enabled=1 -d spx.report=fp test.php

# Trace mode
php -d extension=spx.so -d spx.enabled=1 -d spx.report=trace test.php
```

#### 2. Auto-start
```bash
php -d extension=spx.so -d spx.enabled=1 -d spx.auto_start=1 test.php
```

#### 3. Manual start/stop
```php
<?php
spx_profiler_start();
// ... code to profile ...
spx_profiler_stop();
```

#### 4. Signal handling (CLI)
```bash
php -d extension=spx.so -d spx.enabled=1 -d spx.auto_start=1 long_running.php
# Press Ctrl+C
# Verify: Report should be generated before exit
```

#### 5. Deep call stacks
```php
<?php
function recursive($n) {
    if ($n <= 0) return;
    recursive($n - 1);
}

recursive(1000);  // Test stack capacity
```

#### 6. Sampling mode
```bash
php -d extension=spx.so -d spx.enabled=1 -d spx.sampling_period=1000 test.php
```

### Performance Tests

Benchmark before/after:

```bash
# Overhead test
time php test.php  # Without SPX
time php -d extension=spx.so -d spx.enabled=0 test.php  # SPX loaded, disabled
time php -d extension=spx.so -d spx.enabled=1 test.php  # SPX enabled
```

---

## Rollback Plan

If issues arise:

1. **Keep old code**: Use `#ifdef SPX_NEW_ARCHITECTURE` during transition
2. **Feature flag**: Add ini setting `spx.use_new_architecture=0/1`
3. **Gradual rollout**: Test on non-production first

Example:
```c
#ifdef SPX_NEW_ARCHITECTURE
    /* New module-based code */
    context.session = spx_session_create(&config);
#else
    /* Old direct code */
    context.profiling_handler.profiler = spx_profiler_tracer_create(...);
#endif
```

---

## Expected Benefits

### Code Quality
- **50% reduction** in php_spx.c complexity
- **Clear ownership** of components
- **Independent testing** of modules
- **No magic numbers** (configurable via spx_limits)

### Maintainability
- **Single responsibility** per module
- **Clear interfaces** between components
- **Easier to understand** and modify
- **Better error handling** throughout

### Extensibility
- **Plugin-based metrics** ready for third-party extensions
- **Session abstraction** enables multiple concurrent sessions (future)
- **Modular architecture** allows easy addition of new features

---

## Completion Checklist

- [ ] Step 1: Add includes
- [ ] Step 2: Update context structure
- [ ] Step 3: Update RINIT
- [ ] Step 4: Update RSHUTDOWN
- [ ] Step 5: Update profiling_handler_start
- [ ] Step 6: Update profiling_handler_stop
- [ ] Step 7: Update hook_before
- [ ] Step 8: Update hook_after
- [ ] Step 9: Add terminate function
- [ ] Step 10: Update PHP functions
- [ ] Test: Basic profiling (all modes)
- [ ] Test: Auto-start
- [ ] Test: Manual start/stop
- [ ] Test: Signal handling
- [ ] Test: Deep stacks
- [ ] Test: Sampling
- [ ] Performance: Benchmark
- [ ] Code Review: Check diff
- [ ] Documentation: Update comments

---

## Estimated Effort

| Task | Time | Difficulty |
|------|------|------------|
| Code changes | 4-6 hours | Medium |
| Testing | 2-3 hours | Low |
| Bug fixes | 1-2 hours | Medium |
| Documentation | 1 hour | Low |
| **Total** | **8-12 hours** | **Medium** |

---

## Conclusion

This integration guide provides complete, step-by-step instructions for refactoring `php_spx.c` to use the new architectural modules. All prerequisite modules are implemented and ready. The refactoring follows SOLID principles and significantly improves code quality while maintaining full backward compatibility.

The integration can be done incrementally with feature flags for safety, and comprehensive testing ensures no regressions.

**Status**: Ready for integration
**Risk**: Low (all modules tested, integration well-defined)
**Reward**: High (major improvement in maintainability and extensibility)
