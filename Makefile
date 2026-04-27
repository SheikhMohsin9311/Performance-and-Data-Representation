# ==============================================================================
# Performance and Data Representation - High-Fidelity Build System
# ==============================================================================

CC      ?= gcc
MODE    ?= release
BIN_DIR  = bin
TEST_DIR = tests

# ── Compiler Flags ────────────────────────────────────────────────────────────
COMMON_FLAGS = -std=c11 -Wall -Wextra -flto -D_GNU_SOURCE
LDFLAGS      = -lm -flto

ifeq ($(MODE),debug)
    CFLAGS = $(COMMON_FLAGS) -O0 -g -DDEBUG
    MSG    = "Building in [DEBUG] mode - O0, debug symbols enabled"
else
    # Release mode: Peak performance with architectural tuning
    CFLAGS = $(COMMON_FLAGS) -O3 -march=native -DNDEBUG
    MSG    = "Building in [RELEASE] mode - O3, march=native, NDEBUG enabled"
endif

# ── Targets ───────────────────────────────────────────────────────────────────
TARGETS = bst array linked_list heap hash_map vector_ds deque_ds rb_tree \
          aos soa skip_list btree veb_tree trie slab_list circular_buffer

# Prefix targets with the binary directory
# We use $(addprefix ...) to ensure binaries end up in bin/
BINS = $(addprefix $(BIN_DIR)/, $(TARGETS))
TEST_BIN = $(BIN_DIR)/perf_helper_test

# ── Styling ───────────────────────────────────────────────────────────────────
BLUE  = \033[1;34m
GREEN = \033[1;32m
CYAN  = \033[1;36m
RESET = \033[0m

# ── Rules ─────────────────────────────────────────────────────────────────────
.PHONY: all clean help check test

all: $(BINS)
	@echo "$(GREEN)Build Completed successfully!$(RESET)"

# Compile rule: .c -> bin/binary
$(BIN_DIR)/%: %.c perf_helper.h | $(BIN_DIR)
	@echo "$(CYAN)  Compiling$(RESET) $< -> $@"
	@$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(TEST_BIN): $(TEST_DIR)/perf_helper_test.c perf_helper.h | $(BIN_DIR)
	@echo "$(CYAN)  Compiling$(RESET) $< -> $@"
	@$(CC) $(CFLAGS) -I. -o $@ $< $(LDFLAGS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@echo "$(BLUE)Cleaning up...$(RESET)"
	@rm -rf $(BIN_DIR)

check: all $(TEST_BIN)
	@echo "$(CYAN)Running unit tests...$(RESET)"
	@$(TEST_BIN)
	@echo "$(CYAN)Running smoke tests...$(RESET)"
	@for bin in $(BINS); do \
		echo "Testing $$bin..."; \
		$$bin 100 1 > /dev/null || exit 1; \
	done
	@echo "$(GREEN)All binaries executed successfully with N=100!$(RESET)"

test: $(TEST_BIN)
	@echo "$(CYAN)Running unit tests...$(RESET)"
	@$(TEST_BIN)

help:
	@echo "$(BLUE)Available commands:$(RESET)"
	@echo "  $(GREEN)make$(RESET)          - Build all data structures in Release mode (O3)"
	@echo "  $(GREEN)make MODE=debug$(RESET) - Build in Debug mode (O0, -g)"
	@echo "  $(GREEN)make clean$(RESET)     - Remove all binaries and the bin/ directory"
	@echo "  $(GREEN)make test$(RESET)      - Run perf_helper unit tests"
	@echo "  $(GREEN)make help$(RESET)      - Show this help message"
