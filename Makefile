# ==============================================================================
# Performance and Data Representation - High-Fidelity Build System
# ==============================================================================

CC      ?= gcc
MODE    ?= release
BIN_DIR  = bin

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
TARGETS = second third linked_list heap hash_map vector_ds deque_ds rb_tree \
          aos soa skip_list btree veb_tree trie slab_list circular_buffer

# Prefix targets with the binary directory
# We use $(addprefix ...) to ensure binaries end up in bin/
BINS = $(addprefix $(BIN_DIR)/, $(TARGETS))

# ── Styling ───────────────────────────────────────────────────────────────────
BLUE  = \033[1;34m
GREEN = \033[1;32m
CYAN  = \033[1;36m
RESET = \033[0m

# ── Rules ─────────────────────────────────────────────────────────────────────
.PHONY: all clean help check

all: $(BINS)
	@echo "$(GREEN)Build Completed successfully!$(RESET)"

# Compile rule: .c -> bin/binary
$(BIN_DIR)/%: %.c | $(BIN_DIR)
	@echo "$(CYAN)  Compiling$(RESET) $< -> $@"
	@$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@echo "$(BLUE)Cleaning up...$(RESET)"
	@rm -rf $(BIN_DIR)

help:
	@echo "$(BLUE)Available commands:$(RESET)"
	@echo "  $(GREEN)make$(RESET)          - Build all data structures in Release mode (O3)"
	@echo "  $(GREEN)make MODE=debug$(RESET) - Build in Debug mode (O0, -g)"
	@echo "  $(GREEN)make clean$(RESET)     - Remove all binaries and the bin/ directory"
	@echo "  $(GREEN)make help$(RESET)      - Show this help message"