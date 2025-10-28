# SPX PHP Extension - Code Coverage Report

**Date**: October 28, 2025
**PHP Version**: 8.3.6
**Build**: With gcov/lcov instrumentation

## Executive Summary

- **Current Line Coverage**: 43.4% (1,059 of 2,441 lines)
- **Current Function Coverage**: 49.4% (114 of 231 functions)
- **Test Suite**: 60 tests total (5 passing, 28 failing, 27 skipped for CGI)
- **Coverage Goal**: 80% line coverage

## Coverage by Module

### Critical Path Modules (Low Coverage)

| Module | Lines | Coverage | Priority |
|--------|-------|----------|----------|
| `spx_profiler_tracer.c` | 217 | 4.6% | **HIGH** - Core profiling logic |
| `spx_reporter_trace.c` | 74 | 8.1% | **HIGH** - Trace output |
| `spx_config.c` | 96 | 8.3% | **MEDIUM** - Configuration parsing |
| `spx_reporter_full.c` | 223 | 0.0% | **HIGH** - Full report generation |
| `spx_reporter_fp.c` | ? | 0.0% | **MEDIUM** - Flat profile reporter |
| `spx_profiler_sampler.c` | ? | 0.0% | **MEDIUM** - Sampling profiler |

### Support Modules (Moderate Coverage)

| Module | Lines | Coverage | Status |
|--------|-------|----------|--------|
| `spx_limits.c` | 18 | 38.9% | Good unit test coverage |
| `spx_call_stack.c` | 33 | 33.3% | Partially tested |
| `spx_output_stream.c` | 40 | 47.5% | Moderate coverage |
| `spx_metric.c` | 31 | 29.0% | Needs improvement |
| `spx_resource_stats-linux.c` | 20 | 30.0% | Platform-specific |

### Utility Modules (Variable Coverage)

| Module | Lines | Coverage | Notes |
|--------|-------|----------|-------|
| `spx_string_pool.c` | 28 | 14.3% | Low coverage for critical component |
| `spx_fmt.c` | 82 | 14.6% | Formatting utilities |
| `spx_hmap.c` | 57 | 17.5% | Hash map implementation |
| `spx_utils.c` | 5 | 120%* | *Anomaly - needs investigation |

### Main Entry Points

| Module | Lines | Coverage | Notes |
|--------|-------|----------|-------|
| `php_spx.c` | 121 | 20.7% | Main extension entry point |
| `spx_php.c` | 220 | 25.0% | PHP integration layer |
| `spx_profiler.c` | 5 | 20.0% | Profiler interface |

## Test Coverage Analysis

### Passing Tests (5/33 non-skipped)

1. `spx_004.phpt` - Disabled by default ✓
2. `spx_005.phpt` - Explicitly disabled ✓
3. `spx_auto_start_001.phpt` - Auto start disabled ✓
4. `spx_ini_params_no_effect_in_cli.phpt` - INI params in CLI ✓
5. `spx_userland_api.phpt` - Userland API (NEW) ✓

### New Tests Added (6 total)

Created to improve coverage, currently failing:
- `spx_profiler_basic.phpt` - Basic profiler functionality
- `spx_multiple_metrics.phpt` - Multiple metrics tracking
- `spx_sampler_mode.phpt` - Sampling profiler mode
- `spx_trace_output.phpt` - Trace report generation
- `spx_deep_recursion.phpt` - Deep call stack handling
- `spx_builtins_traced.phpt` - Built-in function tracing

### Test Failure Patterns

Most failures are due to:
1. **Report format mismatches** - Expected output doesn't match actual
2. **Missing profiler features** - Some modes not fully integrated
3. **Environment differences** - Tests expect specific PHP/SPX behavior

## Path to 80% Coverage

### Estimated Effort

To reach 80% line coverage (~1,953 lines), we need to cover an additional **894 lines** (currently at 1,059).

### Recommended Actions

#### 1. Fix Existing Tests (Short Term)
- Debug why 28 tests are failing
- Adjust test expectations to match actual behavior
- This could add 15-20% coverage immediately

#### 2. Core Profiling Path (Medium Priority)
Focus on `spx_profiler_tracer.c` (217 lines at 4.6%):
- Test basic profiling lifecycle
- Test metric collection
- Test function table management
- Test stack handling
- **Potential gain**: +10-15% coverage

#### 3. Reporter Modules (High Impact)
Improve coverage of reporter modules (297+ lines at ~0-8%):
- `spx_reporter_full.c` (223 lines, 0%)
- `spx_reporter_fp.c` (unknown, 0%)
- `spx_reporter_trace.c` (74 lines, 8.1%)
- **Potential gain**: +12-18% coverage

#### 4. Configuration & Entry Points (Medium Priority)
- `spx_config.c` (96 lines, 8.3%)
- `php_spx.c` (121 lines, 20.7%)
- `spx_php.c` (220 lines, 25.0%)
- **Potential gain**: +8-12% coverage

#### 5. Unit Tests for Utilities (Lower Priority)
- String pool (`spx_string_pool.c`)
- Hash map (`spx_hmap.c`)
- Formatters (`spx_fmt.c`)
- **Potential gain**: +3-5% coverage

## Coverage Infrastructure

### Tools Configured
- ✅ **gcov**: GNU coverage tool installed
- ✅ **lcov**: HTML report generation
- ✅ **Build script**: `build_coverage.sh` for instrumented builds
- ✅ **Coverage workflow**: `make test` → `lcov` → HTML report

### HTML Reports
Coverage reports are generated in `coverage_html/` directory:
```bash
./build_coverage.sh
make install
make test
lcov --capture --directory . --output-file coverage.info
lcov --extract coverage.info "*/src/*" --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_html
```

View at: `coverage_html/index.html`

## Recommendations for Maintainers

1. **Prioritize fixing existing tests** - Many tests are close to passing
2. **Focus on critical path coverage** - Tracer and reporters are most important
3. **Consider integration tests** - PHP-level tests provide better overall coverage than unit tests
4. **Add coverage to CI/CD** - Automated coverage reporting on each commit
5. **Set incremental goals** - Target 60% first, then 70%, then 80%

## Files Added/Modified

### New Test Files
- `tests/spx_profiler_basic.phpt`
- `tests/spx_multiple_metrics.phpt`
- `tests/spx_sampler_mode.phpt`
- `tests/spx_trace_output.phpt`
- `tests/spx_deep_recursion.phpt`
- `tests/spx_builtins_traced.phpt`
- `tests/spx_userland_api.phpt` (passing)

### New Infrastructure
- `build_coverage.sh` - Build with coverage instrumentation
- `COVERAGE_REPORT.md` - This document
- Coverage data files (`.gcda`, `.gcno`) - gitignored

### Coverage Artifacts
- `coverage.info` - Raw coverage data
- `coverage_filtered.info` - Filtered to src/ only
- `coverage_html/` - HTML coverage browser

## Next Steps

1. ✅ Coverage infrastructure set up
2. ✅ Baseline measurement complete (43.4%)
3. ⚠️ Test suite expanded (7 new tests, 1 passing)
4. ⏸️ **Blocked on**: Debugging test failures to improve coverage
5. 📋 **TODO**: Fix existing tests to exercise more code paths
6. 📋 **TODO**: Write targeted tests for uncovered modules
7. 📋 **TODO**: Integrate coverage reporting into CI

---

*Report generated by Claude Code coverage analysis*
