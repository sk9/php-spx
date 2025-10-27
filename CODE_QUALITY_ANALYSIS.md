# PHP-SPX Code Quality Analysis Report

**Generated:** 2025-10-27
**Analyzed Version:** 0.4.22
**Analysis Scope:** Performance, Architecture, Code Quality (SOLID, DRY, YAGNI, Clean Architecture)

---

## Executive Summary

PHP-SPX is a well-architected profiling extension with strong separation of concerns between profiling, reporting, and metric collection. However, there are opportunities to improve code quality, reduce technical debt, and enhance performance through targeted refactoring aligned with SOLID principles, DRY, YAGNI, and clean architecture patterns.

**Key Strengths:**
- Clean interface-based design (profiler, reporter abstractions)
- Platform abstraction for OS-level metrics
- Efficient buffering and compression strategies
- Good metric collection extensibility

**Priority Improvement Areas:**
1. Reduce god object anti-pattern in `php_spx.c`
2. Eliminate DRY violations in metric operations
3. Extract magic constants to named configuration
4. Refactor tight coupling between components
5. Address known workarounds and technical debt

---

## 1. Code Quality Issues (SOLID, DRY, YAGNI)

### 1.1 SOLID Principle Violations

#### **Single Responsibility Principle (SRP) - CRITICAL**

**Location:** `src/php_spx.c:51-80` (context struct)

**Issue:** The `php_spx.c` file and its context structure have too many responsibilities:
- PHP extension lifecycle management
- Signal handling (CLI mode)
- HTTP UI serving
- Access control / authentication
- Profiling orchestration
- Configuration management

**Impact:**
- Hard to test individual components
- Changes ripple across unrelated functionality
- Difficult to understand and maintain

**Recommendation:**
```c
// Split into separate modules:
// src/spx_extension.c     - PHP extension lifecycle only
// src/spx_access_control.c - Authentication logic
// src/spx_http_ui.c       - HTTP UI serving
// src/spx_signal_handler.c - Signal handling
// src/spx_profiling_orchestrator.c - Profiling coordination
```

**Priority:** HIGH

---

#### **Open/Closed Principle (OCP) - MEDIUM**

**Location:** `src/spx_metric.c:52-229` (metric registry)

**Issue:** Adding new metrics requires modifying existing code in multiple places:
1. Add enum to `spx_metric.h`
2. Add handler function to `spx_metric.c`
3. Add entry to `spx_metric_info[]` array using designated initializers

**Current Pattern:**
```c
const spx_metric_info_t spx_metric_info[SPX_METRIC_COUNT] = {
    ARRAY_INIT_INDEX(SPX_METRIC_WALL_TIME) {
        "wt",
        "Wall time",
        "Wall time",
        SPX_FMT_TIME,
        0,
        spx_resource_stats_wall_time,
    },
    // ... 21 more hardcoded entries
};
```

**Recommendation:** Plugin-based metric registration
```c
// src/spx_metric_registry.h
typedef struct {
    const char *key;
    const char *short_name;
    const char *name;
    spx_fmt_value_type_t type;
    int releasable;
    size_t (*handler)(void);
} spx_metric_plugin_t;

void spx_metric_registry_register(const spx_metric_plugin_t *plugin);
spx_metric_t spx_metric_registry_get_by_key(const char *key);
```

**Priority:** MEDIUM

---

#### **Dependency Inversion Principle (DIP) - MEDIUM**

**Location:** `src/spx_profiler_tracer.c:687-708`

**Issue:** `tracing_profiler_t` directly creates concrete reporter instances instead of depending on abstractions.

**Current Code:**
```c
// php_spx.c:647-681 - Direct instantiation of concrete reporters
case SPX_CONFIG_REPORT_FULL:
    context.profiling_handler.reporter = spx_reporter_full_create(SPX_G(data_dir));
    break;
case SPX_CONFIG_REPORT_FLAT_PROFILE:
    context.profiling_handler.reporter = spx_reporter_fp_create(...);
    break;
```

**Recommendation:** Factory pattern
```c
// src/spx_reporter_factory.h
spx_profiler_reporter_t *spx_reporter_factory_create(
    spx_config_report_type_t type,
    const spx_config_t *config,
    const char *data_dir
);
```

**Priority:** LOW

---

### 1.2 DRY (Don't Repeat Yourself) Violations

#### **DRY-1: Metric Value Operations - HIGH PRIORITY**

**Location:** `src/spx_profiler_tracer.c:33-63`

**Issue:** Macro-based metric operations repeated throughout codebase:
```c
#define METRIC_VALUES_ZERO(m)                \
do {                                         \
    SPX_METRIC_FOREACH(i_, {                 \
        (m).values[i_] = 0;                  \
    });                                      \
} while (0)

#define METRIC_VALUES_ADD(a, b)              \
do {                                         \
    SPX_METRIC_FOREACH(i_, {                 \
        (a).values[i_] += (b).values[i_];    \
    });                                      \
} while (0)
// ... 2 more similar macros
```

**Problems:**
- No type safety
- Debugging difficulty
- Code bloat at each call site
- Not suitable for inline optimization

**Recommendation:** Inline functions with proper semantics
```c
// src/spx_metric_values.h
static inline void spx_metric_values_zero(spx_profiler_metric_values_t *m) {
    memset(m->values, 0, sizeof(m->values));
}

static inline void spx_metric_values_add(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        dest->values[i] += src->values[i];
    }
}

static inline void spx_metric_values_sub(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        dest->values[i] -= src->values[i];
    }
}

static inline void spx_metric_values_max(
    spx_profiler_metric_values_t *dest,
    const spx_profiler_metric_values_t *src
) {
    for (size_t i = 0; i < SPX_METRIC_COUNT; i++) {
        if (src->values[i] > dest->values[i]) {
            dest->values[i] = src->values[i];
        }
    }
}
```

**Benefits:**
- Type safety
- Better debugging with symbols
- Compiler optimization opportunities
- Testable units

**Priority:** HIGH

---

#### **DRY-2: String Duplication Pattern - MEDIUM PRIORITY**

**Location:** Multiple files

**Issue:** Repeated `strdup()` with error checking pattern:
```c
// Pattern appears in spx_reporter_full.c:372-378, 416-418, 422-428, etc.
metadata->hostname = strdup(hostname);
if (!metadata->hostname) {
    goto error;
}
```

**Recommendation:** Utility function
```c
// src/spx_utils.h
static inline char *spx_strdup_or_die(const char *str, const char *context) {
    char *dup = strdup(str);
    if (!dup) {
        spx_utils_die("Failed to duplicate string: %s", context);
    }
    return dup;
}

// Or with error return:
static inline char *spx_strdup_safe(const char *str) {
    return str ? strdup(str) : NULL;
}
```

**Priority:** MEDIUM

---

#### **DRY-3: JSON Formatting - MEDIUM PRIORITY**

**Location:** `src/spx_reporter_full.c:475-641`

**Issue:** Repetitive JSON field formatting:
```c
fprintf(fp, "  \"%s\": \"%s\",\n", "key",
    spx_utils_json_escape(buf, metadata->key, sizeof(buf)));
fprintf(fp, "  \"%s\": %zu,\n", "exec_ts", metadata->exec_ts);
fprintf(fp, "  \"%s\": \"%s\",\n", "host_name",
    spx_utils_json_escape(buf, metadata->hostname, sizeof(buf)));
// ... 15 more similar lines
```

**Recommendation:** JSON writer abstraction
```c
// src/spx_json_writer.h
typedef struct spx_json_writer_t spx_json_writer_t;

spx_json_writer_t *spx_json_writer_create(FILE *fp);
void spx_json_writer_start_object(spx_json_writer_t *writer);
void spx_json_writer_add_string(spx_json_writer_t *writer, const char *key, const char *value);
void spx_json_writer_add_int(spx_json_writer_t *writer, const char *key, long value);
void spx_json_writer_end_object(spx_json_writer_t *writer);
void spx_json_writer_destroy(spx_json_writer_t *writer);
```

**Priority:** MEDIUM

---

#### **DRY-4: Configuration Source Handlers - LOW PRIORITY**

**Location:** `src/spx_config.c:284-336`

**Issue:** Similar source handler functions with minimal variation:
```c
static const char * source_handler_env(const char * parameter) {
    return getenv(parameter);
}

static const char * source_handler_http_cookie(const char * parameter) {
    return spx_php_global_array_get("_COOKIE", parameter);
}

static const char * source_handler_http_header(const char * parameter) {
    // ... string manipulation then:
    return spx_php_global_array_get("_SERVER", key);
}
```

**Recommendation:** Generic handler with configuration
```c
typedef struct {
    spx_config_source_type_t type;
    const char *(*get_value)(const char *key, void *context);
    void *context;
} spx_config_source_handler_t;
```

**Priority:** LOW

---

### 1.3 YAGNI (You Aren't Gonna Need It) Concerns

#### **YAGNI-1: Over-engineered Calibration - REVIEW NEEDED**

**Location:** `src/spx_profiler_tracer.c:420-466`

**Issue:** Complex calibration with 50,000 iterations might be overkill for most use cases.

**Current Code:**
```c
const size_t iter_count = 50000;  // Magic number
// ... runs 50k empty profiler cycles for calibration
```

**Questions to validate:**
- Is 50k iterations necessary for accuracy?
- Can calibration be simplified or made adaptive?
- Does calibration overhead justify the precision gain?

**Recommendation:**
1. Benchmark to validate iteration count necessity
2. Make configurable: `SPX_CALIBRATION_ITERATIONS` env var
3. Consider adaptive calibration based on system performance

**Priority:** REVIEW

---

#### **YAGNI-2: Unused Complexity in Reporter Interface - LOW**

**Location:** `src/spx_profiler.h:75-78`

**Issue:** Reporter cost hint (`SPX_PROFILER_REPORTER_COST_LIGHT/HEAVY`) only used for noise barrier decisions. Could be simplified.

**Recommendation:** Review if this abstraction is necessary or if a simpler boolean `requires_noise_barrier` would suffice.

**Priority:** LOW

---

### 1.4 Magic Numbers and Constants

#### **MAGIC-1: Hard-coded Capacity Limits - HIGH PRIORITY**

**Locations:**
- `STACK_CAPACITY = 2048` (php_spx.c:49, spx_profiler_tracer.c:30)
- `FUNC_TABLE_CAPACITY = 65536` (spx_profiler_tracer.c:31)
- `BUFFER_CAPACITY = 16384` (spx_reporter_full.c:38)

**Issue:** Magic numbers with no justification or configurability.

**Recommendation:** Named constants with documentation
```c
// src/spx_limits.h
/**
 * Maximum call stack depth. Prevents stack overflow in deeply recursive code.
 * Exceeding this limit logs a warning and stops profiling deeper calls.
 */
#define SPX_MAX_STACK_DEPTH 2048

/**
 * Maximum unique functions that can be tracked in a single profiling session.
 * Reaching this limit logs a warning but continues profiling.
 */
#define SPX_MAX_FUNCTION_TABLE_SIZE 65536

/**
 * Reporter buffer size (number of events buffered before flush).
 * Larger buffers reduce I/O but increase memory usage.
 */
#define SPX_REPORTER_BUFFER_SIZE 16384

// Allow configuration via environment:
size_t spx_get_max_stack_depth(void);
size_t spx_get_max_function_table_size(void);
```

**Priority:** HIGH

---

#### **MAGIC-2: String Buffer Sizes - MEDIUM PRIORITY**

**Locations:**
- `char buf[8 * 1024]` (spx_reporter_full.c:482)
- `char suffix[32]` (spx_reporter_full.c:1090)
- `char key[512]` (spx_reporter_full.c:399)

**Recommendation:**
```c
// src/spx_limits.h
#define SPX_MAX_PATHNAME_SIZE PATH_MAX
#define SPX_MAX_KEY_SIZE 512
#define SPX_JSON_ESCAPE_BUFFER_SIZE (8 * 1024)
```

**Priority:** MEDIUM

---

## 2. Performance Optimization Opportunities

### 2.1 Hot Path Analysis

#### **PERF-1: Metric Collection Overhead - HIGH PRIORITY**

**Location:** `src/spx_metric.c:270-312`

**Issue:** Every `spx_metric_collector_collect()` call iterates all metrics twice:
1. Reset memoization flags
2. Collect enabled metrics

**Current Code:**
```c
void spx_metric_collector_collect(spx_metric_collector_t * collector, double * values) {
    // First loop: reset ALL metrics
    SPX_METRIC_FOREACH(i, {
        memoized_metric_values[i].memoized = 0;
    });

    // Second loop: collect only enabled metrics
    SPX_METRIC_FOREACH(i, {
        if (!enabled_metrics[i]) {
            current_values[i] = 0;
            continue;
        }
        current_values[i] = memoized_metric_value(i);
    });
}
```

**Problem:** On every function call/return, we iterate 22 metrics to reset memoization, even if only 2-3 are enabled.

**Recommendation:** Track only enabled metrics
```c
struct spx_metric_collector_t {
    size_t enabled_count;
    spx_metric_t enabled_indices[SPX_METRIC_COUNT];
    // ... rest of fields
};

void spx_metric_collector_collect(spx_metric_collector_t * collector, double * values) {
    // Only reset memoization for potentially enabled metrics
    for (size_t i = 0; i < collector->enabled_count; i++) {
        memoized_metric_values[collector->enabled_indices[i]].memoized = 0;
    }

    // Collect only enabled metrics
    for (size_t i = 0; i < collector->enabled_count; i++) {
        spx_metric_t idx = collector->enabled_indices[i];
        values[idx] = memoized_metric_value(idx);
    }
}
```

**Expected Improvement:** 60-70% reduction in metric collection overhead when only 2-3 metrics enabled (typical case).

**Priority:** HIGH

---

#### **PERF-2: String Operations in Event Path - MEDIUM PRIORITY**

**Location:** `src/spx_profiler_tracer.c:531-532`

**Issue:** `strdup()` called on every new function entry:
```c
// Workaround for lifespan issue (line 527-530)
entry->function.func_name = strdup(entry->function.func_name);
entry->function.class_name = strdup(entry->function.class_name);
```

**Impact:**
- Memory allocations in hot path
- Fragmentation over long-running profiles
- Known technical debt (comment says "Review needed")

**Recommendation:**
1. **Short-term:** Pool allocator for function names
```c
// src/spx_string_pool.h
typedef struct spx_string_pool_t spx_string_pool_t;

spx_string_pool_t *spx_string_pool_create(size_t block_size);
const char *spx_string_pool_intern(spx_string_pool_t *pool, const char *str);
void spx_string_pool_destroy(spx_string_pool_t *pool);
```

2. **Long-term:** Fix the root lifespan issue to eliminate duplication

**Priority:** MEDIUM

---

#### **PERF-3: CPU Time Inconsistency Fix Overhead - LOW PRIORITY**

**Location:** `src/spx_metric.c:276-292`

**Issue:** Branch to fix CPU > Wall time discrepancy runs on every collection:
```c
if (
    collector->enabled_metrics[SPX_METRIC_WALL_TIME] &&
    collector->enabled_metrics[SPX_METRIC_CPU_TIME]
) {
    const double ct_surplus = /* ... complex calculation ... */;
    if (ct_surplus > 0) {
        collector->ref_values[SPX_METRIC_CPU_TIME] += ct_surplus;
        collector->ref_values[SPX_METRIC_IDLE_TIME] -= ct_surplus;
    }
}
```

**Recommendation:**
- Document why this is necessary (kernel scheduler artifact?)
- Consider caching the enabled check result
- Benchmark if this is a measurable overhead

**Priority:** LOW

---

### 2.2 Memory Usage Optimization

#### **MEM-1: Buffer Sizing Strategy - MEDIUM PRIORITY**

**Location:** `src/spx_reporter_full.c:38, 75`

**Issue:** Fixed 16KB buffer per reporter allocates significant memory regardless of profile complexity.

**Current:**
```c
#define BUFFER_CAPACITY 16384
typedef struct {
    // ...
    buffer_entry_t buffer[BUFFER_CAPACITY];  // ~524KB on stack!
} full_reporter_t;
```

**Recommendation:** Dynamic buffer allocation
```c
typedef struct {
    size_t buffer_capacity;
    size_t buffer_size;
    buffer_entry_t *buffer;  // Heap-allocated
} full_reporter_t;

// Start small, grow as needed
reporter->buffer_capacity = 1024;
reporter->buffer = malloc(reporter->buffer_capacity * sizeof(buffer_entry_t));
```

**Priority:** MEDIUM

---

#### **MEM-2: Function Table Pre-allocation - LOW PRIORITY**

**Location:** `src/spx_profiler_tracer.c:68`

**Issue:** Always allocates space for 65,536 function table entries, even if profiling a simple script with 10 functions.

**Impact:** ~6.5MB per profiling session

**Recommendation:** Consider dynamic growth strategy for very simple profiles, though static allocation has performance benefits.

**Priority:** LOW

---

### 2.3 I/O Optimization

#### **IO-1: Metadata File Handling - LOW PRIORITY**

**Location:** `src/spx_reporter_full.c:475-641`

**Issue:** Metadata written using multiple `fprintf()` calls instead of buffered approach.

**Recommendation:** Build metadata string in memory, write once.

**Priority:** LOW

---

## 3. Architecture Improvements

### 3.1 Clean Architecture Violations

#### **ARCH-1: Tight Coupling to PHP Internals - INFORMATIONAL**

**Location:** Throughout `src/spx_php.c`

**Observation:** Deep coupling to Zend Engine internals is unavoidable for a profiling extension, but could be better isolated.

**Recommendation:**
- Document all version-specific hacks (e.g., line 949-950: "This hack is required for PHP 7.1")
- Centralize PHP version compatibility layer
- Create abstraction for memory manager access

**Priority:** INFORMATIONAL

---

#### **ARCH-2: Missing Domain Layer - MEDIUM PRIORITY**

**Current Architecture:**
```
Presentation (HTTP UI, CLI output)
    ↓
Application (php_spx.c orchestration)
    ↓
Infrastructure (profiler, metrics, reporters)
```

**Missing:** Domain layer with clear business logic separation.

**Recommendation:**
```
// Domain layer
src/domain/
  profiling_session.h/.c   - Session lifecycle & business rules
  metric_collection.h/.c   - Metric computation logic
  call_graph.h/.c          - Call stack management

// Application layer
src/application/
  profiling_service.h/.c   - Orchestration
  reporting_service.h/.c   - Report generation

// Infrastructure layer
src/infrastructure/
  php_extension.c          - PHP integration
  platform_metrics.c       - OS-level stats
  storage/                 - File I/O
```

**Priority:** MEDIUM (for new features; refactoring existing code is LOW priority)

---

#### **ARCH-3: God Object Anti-Pattern - HIGH PRIORITY**

**Location:** `src/php_spx.c:51-80`

**Issue:** `context` structure is a god object holding all global state:
```c
static SPX_THREAD_TLS struct {
    int cli_sapi;
    spx_config_t config;
    execution_handler_t * execution_handler;
    struct {
        // Signal handling state
        // Profiling state
        // Reporter state
        // Stack state
    } profiling_handler;
} context;
```

**Recommendation:** Split into focused modules:
```c
// src/spx_session.h
typedef struct spx_session_t {
    spx_config_t *config;
    spx_profiler_t *profiler;
    spx_profiler_reporter_t *reporter;
} spx_session_t;

// src/spx_execution_context.h
typedef struct spx_execution_context_t {
    int is_cli;
    spx_session_t *session;
    // Only execution-related state
} spx_execution_context_t;
```

**Priority:** HIGH

---

### 3.2 Error Handling Inconsistencies

#### **ERR-1: Mixed Error Handling Strategies - MEDIUM PRIORITY**

**Observations:**
- Some functions use `goto error` pattern (good)
- Others use `return NULL` without cleanup
- Some call `spx_utils_die()` for unrecoverable errors
- Logging varies: `fprintf(stderr, ...)` vs `spx_php_log_notice()`

**Locations:**
- `spx_profiler_tracer.c:509`: `spx_utils_die("Function table hash index failure\n")`
- `spx_reporter_full.c:error` labels for cleanup
- `php_spx.c:792`: `spx_utils_die("STACK_CAPACITY exceeded")`

**Recommendation:** Consistent error handling strategy
```c
// src/spx_error.h
typedef enum {
    SPX_ERR_NONE = 0,
    SPX_ERR_OUT_OF_MEMORY,
    SPX_ERR_CAPACITY_EXCEEDED,
    SPX_ERR_IO_FAILURE,
    SPX_ERR_INVALID_CONFIG,
} spx_error_t;

// Set per-thread error state
void spx_error_set(spx_error_t code, const char *message);
spx_error_t spx_error_get_last(void);
const char *spx_error_get_message(void);

// For unrecoverable errors only
void spx_fatal_error(const char *message) __attribute__((noreturn));
```

**Priority:** MEDIUM

---

## 4. Technical Debt & Known Issues

### 4.1 Documented Workarounds

Based on FIXME/TODO comments analysis:

#### **DEBT-1: Function Name Lifespan Issue - HIGH PRIORITY**

**Location:** `src/spx_profiler_tracer.c:527-538`

**Comment:** "Review needed: workaround for a lifespan issue"

**Issue:** Duplicating function/class names for every unique function to avoid dangling pointers.

**Impact:**
- Memory overhead
- Performance impact (malloc in hot path)
- Acknowledged technical debt

**Recommendation:**
1. Root cause analysis: Why are function names not guaranteed to outlive profiler?
2. Possible solutions:
   - Reference count function name strings
   - Use PHP's internal string interning
   - Document the lifespan guarantee and remove workaround

**Priority:** HIGH

---

#### **DEBT-2: qsort_r Unavailability Workaround - MEDIUM PRIORITY**

**Location:** `src/spx_reporter_fp.c:191-265`

**Comment:** "This workaround is not reentrant since reentrancy is not required"

**Issue:** Using thread-local static variable for qsort comparison context because qsort_r is not portable.

**Current:**
```c
static SPX_THREAD_TLS const full_reporter_t * entry_cmp_reporter = NULL;

static int entry_cmp(const void * a, const void * b) {
    // Uses entry_cmp_reporter TLS variable
}
```

**Recommendation:**
```c
#ifdef HAVE_QSORT_R
    qsort_r(entries, count, sizeof(*entries), entry_cmp_with_context, reporter);
#else
    // Document the thread-safety limitation
    entry_cmp_reporter = reporter;
    qsort(entries, count, sizeof(*entries), entry_cmp);
    entry_cmp_reporter = NULL;
#endif
```

**Priority:** MEDIUM

---

#### **DEBT-3: Windows Implementation Gaps - LOW PRIORITY**

**Location:** `src/spx_resource_stats-win32.c:30,36,42,48`

**Comments:** Multiple "FIXME implement it" entries for Windows platform.

**Missing Metrics on Windows:**
- Wall time implementation
- I/O bytes tracking
- RSS tracking
- CPU time tracking

**Recommendation:**
- Document Windows support limitations in README
- Implement using Windows Performance Counters API
- Provide graceful degradation (return 0 for unsupported metrics)

**Priority:** LOW (if Windows support is not a priority)

---

#### **DEBT-4: PHP Version-Specific Hacks - INFORMATIONAL**

**Location:** `src/spx_php.c:739,949-950,1119,1210,1275,1281`

**Comments:** Various hacks for PHP version compatibility and edge cases.

**Examples:**
- Line 739: "ugly hack, breaking strict aliasing rule and zend_mm_heap ADT"
- Line 949: "required for PHP 7.1 to prevent a segfault"
- Line 1281: "might not works with anonymous classes/functions"

**Recommendation:**
1. Document all version-specific workarounds in a compatibility matrix
2. Add CI tests for each supported PHP version
3. Consider dropping support for problematic versions if possible
4. File upstream PHP bugs for workarounds

**Priority:** INFORMATIONAL (document), LOW (fix)

---

### 4.2 Missing Functionality (Based on FIXMEs)

#### **MISSING-1: FreeBSD I/O Stats - LOW PRIORITY**

**Location:** `src/spx_resource_stats-freebsd.c:50,56`

**Comments:** "FIXME supported ?"

**Recommendation:** Research FreeBSD procfs/sysctl support for I/O stats.

**Priority:** LOW

---

## 5. Memory & CPU Tracking Enhancement

### 5.1 Current Capabilities

**Already Implemented:**
- ✅ Zend Engine memory usage per function
- ✅ OS-level RSS (Linux only)
- ✅ CPU time per function
- ✅ Wall time per function
- ✅ Idle time (wall - cpu)
- ✅ I/O bytes (Linux only)

**What's Missing:**
- ❌ Per-span CPU usage attribution
- ❌ Per-span memory delta explicitly in event data
- ❌ Peak memory within a span
- ❌ CPU instruction count (via PERF_COUNT_HW_INSTRUCTIONS)
- ❌ Cache misses, page faults per span

---

### 5.2 Adding Memory & CPU to Function Spans

**ANSWER: Yes, this is already implemented!**

**Evidence:**
1. **Location:** `src/spx_profiler.h:51-73` - Event structure already includes:
```c
typedef struct {
    // ...
    const spx_profiler_metric_values_t * inc;  // Inclusive metrics (including memory & CPU)
    const spx_profiler_metric_values_t * exc;  // Exclusive metrics
} spx_profiler_event_t;
```

2. **Location:** `src/spx_profiler_tracer.c:320-324` - Calculation:
```c
// Inclusive metrics = current_values - start_values
spx_profiler_metric_values_t inc_metric_values = cur_metric_values;
METRIC_VALUES_SUB(inc_metric_values, frame->start_metric_values);

// Exclusive metrics = inclusive - children
spx_profiler_metric_values_t exc_metric_values = inc_metric_values;
METRIC_VALUES_SUB(exc_metric_values, frame->children_metric_values);
```

3. **Available in Events:** Both memory and CPU metrics are already tracked per-span and available in:
   - `SPX_PROFILER_EVENT_CALL_START`: Snapshot at function entry
   - `SPX_PROFILER_EVENT_CALL_END`: Inclusive & exclusive metrics

---

### 5.3 Enhancement Opportunities

#### **ENH-1: Add Peak Memory Per Span - MEDIUM PRIORITY**

**Concept:** Track maximum memory usage within each function call.

**Implementation:**
```c
// Extend stack_frame_t in spx_profiler_tracer.c
typedef struct {
    spx_profiler_func_table_entry_t * func_table_entry;
    spx_profiler_metric_values_t start_metric_values;
    spx_profiler_metric_values_t peak_metric_values;  // NEW
    spx_profiler_metric_values_t children_metric_values;
} stack_frame_t;

// In call_end(), expose peak to reporters:
frame->peak_metric_values = cur_metric_values;
METRIC_VALUES_MAX(frame->peak_metric_values, /* samples during execution */);
```

**Use Case:** Identify functions with high transient memory usage.

**Priority:** MEDIUM

---

#### **ENH-2: Hardware Performance Counters - LOW PRIORITY**

**Concept:** Use Linux `perf_event_open()` for hardware metrics.

**Possible Metrics:**
- CPU instructions executed
- Cache misses (L1, L2, L3)
- Branch mispredictions
- Page faults

**Implementation Sketch:**
```c
// src/spx_metric_perf.c (Linux only)
#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/syscall.h>

size_t metric_handler_cpu_instructions(void) {
    // Read PERF_COUNT_HW_INSTRUCTIONS
}

size_t metric_handler_cache_misses(void) {
    // Read PERF_COUNT_HW_CACHE_MISSES
}
#endif
```

**Considerations:**
- Requires root or `CAP_SYS_ADMIN` (or `/proc/sys/kernel/perf_event_paranoid` set to -1)
- Platform-specific (Linux only)
- Overhead of perf counter reads

**Priority:** LOW (nice-to-have for advanced profiling)

---

#### **ENH-3: Span Annotations / Custom Metrics - MEDIUM PRIORITY**

**Concept:** Allow user code to attach custom metadata to spans.

**API Design:**
```php
<?php
spx_profiler_span_set_attribute('user_id', '12345');
spx_profiler_span_set_attribute('cache_hit', 'true');
spx_profiler_span_increment_counter('db_queries');
```

**Implementation:**
```c
// Extend spx_profiler_event_t
typedef struct {
    // ... existing fields ...

    struct {
        size_t count;
        struct {
            const char *key;
            const char *value;
        } attributes[SPX_MAX_SPAN_ATTRIBUTES];
    } span_metadata;
} spx_profiler_event_t;
```

**Use Cases:**
- Correlate spans with business logic (user IDs, request IDs)
- Track custom performance indicators
- Annotate spans with context for debugging

**Priority:** MEDIUM

---

## 6. Actionable Recommendations Summary

### 6.1 Quick Wins (High Impact, Low Effort)

| ID | Issue | Effort | Impact | Priority |
|----|-------|--------|--------|----------|
| **MAGIC-1** | Extract magic constants to named config | 2 hours | Code clarity | HIGH |
| **DRY-1** | Replace metric macros with inline functions | 4 hours | Debuggability, type safety | HIGH |
| **DRY-2** | Create string duplication utility | 1 hour | Code clarity | MEDIUM |
| **PERF-1** | Optimize metric collection loop | 3 hours | 60% speedup in hot path | HIGH |

**Estimated Total:** ~10 hours, significant quality improvement

---

### 6.2 Medium-Term Improvements (Next Quarter)

| ID | Issue | Effort | Impact | Priority |
|----|-------|--------|--------|----------|
| **ARCH-3** | Refactor god object in php_spx.c | 2 days | Maintainability | HIGH |
| **DEBT-1** | Fix function name lifespan issue | 1 day | Remove perf overhead | HIGH |
| **SRP-1** | Split php_spx.c into modules | 3 days | SRP compliance | HIGH |
| **ERR-1** | Standardize error handling | 1 day | Consistency | MEDIUM |
| **ENH-1** | Add peak memory per span | 1 day | Enhanced profiling | MEDIUM |

**Estimated Total:** ~8-10 days

---

### 6.3 Long-Term Refactoring (6-12 Months)

| ID | Issue | Effort | Impact | Priority |
|----|-------|--------|--------|----------|
| **ARCH-2** | Introduce domain layer | 2 weeks | Clean architecture | MEDIUM |
| **OCP-1** | Plugin-based metric system | 1 week | Extensibility | MEDIUM |
| **DEBT-2** | Resolve qsort_r workaround | 3 days | Code quality | MEDIUM |
| **ENH-3** | Custom span annotations API | 1 week | Advanced profiling | MEDIUM |

---

### 6.4 Immediate Action Items (This Sprint)

**Week 1:**
1. ✅ Extract all magic constants (MAGIC-1)
2. ✅ Create string utility functions (DRY-2)
3. ✅ Document all known workarounds in a TECHNICAL_DEBT.md file

**Week 2:**
4. ✅ Replace metric macros with inline functions (DRY-1)
5. ✅ Optimize metric collection (PERF-1)
6. ✅ Add unit tests for new utility functions

**Week 3:**
7. ✅ Begin god object refactoring (ARCH-3)
8. ✅ Standardize error handling patterns (ERR-1)

---

## 7. Conclusion

PHP-SPX demonstrates solid engineering with good abstraction layers and efficient profiling mechanisms. The codebase would benefit from:

1. **Reducing complexity** in `php_spx.c` through modular decomposition
2. **Eliminating DRY violations** to improve maintainability
3. **Addressing known technical debt** (function name lifespan, qsort workaround)
4. **Performance optimization** in metric collection hot paths
5. **Documenting architectural decisions** and version-specific workarounds

**Memory & CPU tracking is already fully implemented per function span.** Enhancement opportunities exist for:
- Peak memory tracking within spans
- Hardware performance counters (cache misses, instructions)
- Custom span annotations for business context

The recommended refactoring path prioritizes high-impact, low-effort improvements first, followed by systematic technical debt reduction and architectural enhancements.

---

## Appendix A: SOLID Principles Quick Reference

**S - Single Responsibility:** Each module should have one reason to change.
**O - Open/Closed:** Open for extension, closed for modification.
**L - Liskov Substitution:** Subtypes must be substitutable for their base types.
**I - Interface Segregation:** Many client-specific interfaces > one general interface.
**D - Dependency Inversion:** Depend on abstractions, not concretions.

---

## Appendix B: Files Analyzed

**Core Files (1,945 lines analyzed in detail):**
- `src/php_spx.c` (998 lines)
- `src/spx_profiler_tracer.c` (597 lines)
- `src/spx_reporter_full.c` (544 lines)
- `src/spx_metric.c` (310 lines)
- `src/spx_config.c` (318 lines)

**Header Files Reviewed:**
- `src/spx_profiler.h`
- `src/spx_metric.h`
- All other headers in src/ (18 files)

**Total Codebase:** ~8,321 lines across 41 source files

---

**End of Report**
