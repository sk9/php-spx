# Unit test Makefile for SPX

CC = gcc
CFLAGS = -Wall -Wextra -I../../src -g -O0 -std=c99
LDFLAGS = -lm

# Source files from main project
SRC_DIR = ../../src
TESTS_DIR = .

# Test executables
TESTS = test_limits \
        test_metric_values \
        test_string_utils \
        test_error_handling \
        test_metric_registry

# Color output
GREEN = \033[0;32m
RED = \033[0;31m
NC = \033[0m

all: $(TESTS)

test_limits: test_limits.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_metric_values: test_metric_values.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_string_utils: test_string_utils.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_error_handling: test_error_handling.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_metric_registry: test_metric_registry.c $(SRC_DIR)/spx_metric_registry.c $(SRC_DIR)/spx_error.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: all
	@echo "\n$(GREEN)Running unit tests...$(NC)\n"
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done
	@echo "\n$(GREEN)All tests passed!$(NC)\n"

clean:
	rm -f $(TESTS) *.o

.PHONY: all run clean
