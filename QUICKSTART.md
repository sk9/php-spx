# PHP-SPX Quick Start Guide

**After Recent Refactoring - Updated October 29, 2025**

This guide provides quick commands to get you started with building, testing, and using PHP-SPX after the recent test-debug-refactor-cleanup work.

---

## Quick Build & Test (Copy-Paste Ready)

### Option 1: Build and Test in One Go

```bash
# Build the extension
phpize && \
./configure && \
make clean && make && \
sudo make install

# Enable the extension
echo "extension=spx.so" | sudo tee /etc/php/8.4/cli/conf.d/20-spx.ini

# Verify installation
php -m | grep SPX

# Run all tests
cd tests/unit && make clean && make && make run && cd ../..
export TEST_PHP_EXECUTABLE=$(which php)
php run-tests.php -q tests/*.phpt
```

### Option 2: Step-by-Step

```bash
# 1. Generate build files
phpize

# 2. Configure the build
./configure

# 3. Compile
make clean && make

# 4. Install (optional - skip if testing locally)
sudo make install

# 5. Enable extension (if installed)
echo "extension=spx.so" | sudo tee /etc/php/8.4/cli/conf.d/20-spx.ini

# 6. Verify
php -m | grep SPX
# Should output: SPX

# 7. Build and run unit tests
cd tests/unit
make clean && make
make run
cd ../..

# 8. Run integration tests
export TEST_PHP_EXECUTABLE=$(which php)
php run-tests.php -q tests/*.phpt
```

---

## Expected Results

### Build Output
```
Build complete.
Don't forget to run 'make test'.
```

### Unit Tests
```
All tests passed!
Total: 35 tests
```

### Integration Tests
```
Tests passed: 29/29 (100.0%)
Tests skipped: 34 (require php-cgi)
Tests failed: 0
```

---

## Using SPX

### Command Line Profiling

```bash
# Enable profiling for a script
SPX_ENABLED=1 php your_script.php

# With specific metrics
SPX_ENABLED=1 SPX_METRICS=wt,zm,io php your_script.php

# Live flat profile
SPX_ENABLED=1 SPX_FP_LIVE=1 php your_script.php

# Full report (for web UI analysis)
SPX_ENABLED=1 SPX_REPORT=full php your_script.php
```

### Web Profiling

Add to your PHP configuration (e.g., `/etc/php/8.4/fpm/conf.d/20-spx.ini`):

```ini
extension=spx.so
spx.http_enabled=1
spx.http_key="dev"
spx.http_ip_whitelist="127.0.0.1"
```

Access the control panel:
```
http://localhost/?SPX_KEY=dev&SPX_UI_URI=/
```

---

## Troubleshooting

### Build Fails

**Error:** `control reaches end of non-void function`
**Solution:** Ensure `php_spx.h` is included BEFORE `ext/standard/info.h` in php_spx.c

**Error:** `implicit declaration of function 'snprintf'`
**Solution:** Add `#include <stdio.h>` at the top of the file

**Error:** `for loop initial declarations are only allowed in C99`
**Solution:** Move variable declarations outside the for loop:
```c
// Wrong (C99):
for (size_t i = 0; i < n; i++) { }

// Correct (C90):
size_t i;
for (i = 0; i < n; i++) { }
```

### Tests Fail

**Issue:** Most tests skipped
**Reason:** php-cgi not installed (this is normal)
**Solution:** Only 29 CLI tests will run, which is expected

**Issue:** SPX extension not loaded
**Solution:**
```bash
echo "extension=spx.so" | sudo tee /etc/php/8.4/cli/conf.d/20-spx.ini
php -m | grep SPX
```

### Formatting Issues

**Issue:** clang-format breaks compilation
**Solution:** Ensure `.clang-format` has `SortIncludes: false`

```bash
# Verify setting
grep SortIncludes .clang-format
# Should output: SortIncludes: false
```

---

## Development Workflow

### Making Changes

```bash
# 1. Edit source files
vim src/your_file.c

# 2. Format code
find src -name "*.c" -o -name "*.h" | xargs clang-format -i -style=file

# 3. Rebuild
make clean && make

# 4. Run tests
cd tests/unit && make clean && make && make run && cd ../..
php run-tests.php -q tests/*.phpt

# 5. Verify no regressions
# All unit tests should pass: 35/35
# All integration tests should pass: 29/29
```

### Before Committing

```bash
# 1. Clean build artifacts
make clean
cd tests/unit && make clean && cd ../..

# 2. Format all code
find src -name "*.c" -o -name "*.h" | xargs clang-format -i -style=file

# 3. Verify compilation
make clean && make

# 4. Run all tests
cd tests/unit && make && make run && cd ../..
export TEST_PHP_EXECUTABLE=$(which php)
php run-tests.php -q tests/*.phpt

# 5. Check git status
git status --short
# Should only show intentional changes
```

---

## Key Files Modified (Recent Work)

| File | Purpose | Change |
|------|---------|--------|
| `.clang-format` | Formatting config | Added `SortIncludes: false` |
| `src/php_spx.c` | Main extension | Fixed include order, flag names |
| `src/infrastructure/security/spx_security_validation.c` | Input validation | Added stdio.h, C90 compat |
| `src/infrastructure/security/spx_security_crypto.c` | Crypto functions | Added stdlib.h, C90 compat |

---

## Quick Reference

### Important Paths

- **Extension:** `/usr/lib/php/20240924/spx.so`
- **PHP Config:** `/etc/php/8.4/cli/conf.d/20-spx.ini`
- **Web UI Assets:** `/usr/share/misc/php-spx/assets/web-ui`
- **Data Directory:** `/tmp/spx` (default)

### Key Commands

```bash
# Check if SPX is loaded
php -m | grep SPX

# View SPX configuration
php --ri spx

# Run specific test
php run-tests.php tests/spx_profiler_basic.phpt

# Clean everything
make clean && cd tests/unit && make clean && cd ../..

# Format code
find src -name "*.c" -o -name "*.h" | xargs clang-format -i -style=file
```

---

## Getting Help

1. **Full Documentation:** See `TEST_DEBUG_REFACTOR_CLEANUP_SUMMARY.md`
2. **Test Details:** See `TEST_IMPROVEMENTS.md`
3. **Architecture:** See `ARCHITECTURE_REFACTORING_PLAN.md`
4. **Main README:** See `README.md`
5. **Issues:** https://github.com/NoiseByNorthwest/php-spx/issues

---

## Success Criteria

✅ **Build:** `make` completes with "Build complete."
✅ **Unit Tests:** 35/35 passing
✅ **Integration Tests:** 29/29 passing (34 skipped for CGI)
✅ **SPX Loaded:** `php -m | grep SPX` outputs "SPX"
✅ **Profiling Works:** `SPX_ENABLED=1 php -v` shows profiler output

---

**Last Updated:** October 29, 2025
**Status:** ✅ All systems operational
**Quality:** 100% test pass rate
