#!/bin/bash

# =============================================================================
# runperf.sh — High-fidelity benchmark runner
# =============================================================================
# Usage: bash runperf.sh
# Prerequisites: run `sudo bash stabilize.sh` first for reproducible results.
# =============================================================================

set -euo pipefail

# 1. Initialize the built-in Bash timer
SECONDS=0

# Dynamic output to allow data accumulation
OUTPUT="deep_scaling_results_$(date +%s).csv"

# ==========================================
# CONFIGURATION: REPEAT COUNTS & CORE PINNING
# ==========================================
CORE_ID=1               # Pin all benchmarks to this CPU core
RUNS_LARGE=3            # N >= 10M
RUNS_MEDIUM=20          # N >= 1M
RUNS_SMALL=200          # N >= 100K
RUNS_TINY=1000          # N < 100K — many reps to overcome os noise at tiny sizes
# Rows where median_cycles < this will be flagged (unreliable measurement)
MIN_CYCLES_WARN=50000
# ==========================================

# ── Pre-flight checks ─────────────────────────────────────────────────────────
echo "=== Pre-flight Checks ==="

# Check CPU governor
GOV=$(cat /sys/devices/system/cpu/cpu${CORE_ID}/cpufreq/scaling_governor 2>/dev/null || echo "unknown")
if [ "$GOV" != "performance" ]; then
    echo "  [WARNING] CPU governor is '${GOV}' (not 'performance')."
    echo "            For reproducible results, run: sudo bash stabilize.sh"
fi

# Check perf_event_paranoid
PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "unknown")
if [ "$PARANOID" != "-1" ] && [ "$PARANOID" != "0" ]; then
    echo "  [WARNING] perf_event_paranoid=${PARANOID} — hardware counters may be blocked."
    echo "            For counter access, run: sudo bash stabilize.sh"
fi

echo "  Output file : $OUTPUT"
echo "  CPU core    : $CORE_ID"
echo "  RUNS (S/M/L): ${RUNS_SMALL}/${RUNS_MEDIUM}/${RUNS_LARGE}"
echo "=========================="

# Write CSV header
echo "data_structure,N,runs,median_cycles,median_instructions,ipc,median_cache_misses,median_branches,median_branch_misses,median_l1_misses,median_tlb_misses,cv_pct" > "$OUTPUT"

# All 16 Data Structures
declare -a ENTRIES=(
    "array:Array"
    "linked_list:LinkedList"
    "aos:AoS"
    "soa:SoA"
    "bst:BST"
    "vector_ds:Vector"
    "deque_ds:Deque"
    "heap:Heap"
    "hash_map:HashMap"
    "rb_tree:RBTree"
    "btree:BTree"
    "circular_buffer:CircularBuffer"
    "skip_list:SkipList"
    "slab_list:SlabList"
    "trie:Trie"
    "veb_tree:vEBTree"
)

# Fixed canonical sizes: 4 steps per decade from 10^3 to 10^7
# (no jitter — identical workloads every run for reproducibility)
SIZES=(
    1000 2500 5000 7500
    10000 25000 50000 75000
    100000 250000 500000 750000
    1000000 2500000 5000000 7500000
    10000000
)

# Verify binaries are present
if [ ! -d "bin" ] || [ -z "$(ls -A bin 2>/dev/null)" ]; then
    echo "Error: bin/ directory is empty. Run 'make' first!"
    exit 1
fi

for entry in "${ENTRIES[@]}"; do
    binary="${entry%%:*}"
    label="${entry##*:}"

    if [ ! -f "./bin/$binary" ]; then
        echo "Skipping $label (binary not found in ./bin/)"
        continue
    fi

    echo -n "Benchmarking $label ... "
    for N in "${SIZES[@]}"; do

        # Deterministic RUNS based on N tier
        if [ "$N" -ge 10000000 ]; then
            RUNS=$RUNS_LARGE
        elif [ "$N" -ge 1000000 ]; then
            RUNS=$RUNS_MEDIUM
        elif [ "$N" -ge 100000 ]; then
            RUNS=$RUNS_SMALL
        else
            RUNS=$RUNS_TINY
        fi

        # Execute binary with crash detection
        if ! exec_output=$(taskset -c $CORE_ID "./bin/$binary" "$N" "$RUNS" 2>&1); then
            echo ""
            echo "  [ERROR] $label crashed at N=$N — skipping"
            continue
        fi

        # Parse METRICS2 line: median_cycles,median_instructions,ipc,cache_misses,
        #                       branches,branch_misses,l1_misses,tlb_misses,cv_pct
        metrics_line=$(echo "$exec_output" | grep "^METRICS2" | tail -1)
        if [ -z "$metrics_line" ]; then
            echo ""
            echo "  [WARNING] No METRICS2 output for $label at N=$N"
            continue
        fi

        # Strip "METRICS2," prefix
        perf_data="${metrics_line#METRICS2,}"

        IFS=',' read -r med_cyc med_ins ipc med_cm med_br med_brm med_l1 med_tlb cv_pct <<< "$perf_data"

        # Warn if the measurement is too short to be meaningful
        if [ "${med_cyc:-0}" -lt "$MIN_CYCLES_WARN" ] 2>/dev/null; then
            echo ""
            echo "  [NOISE] $label N=$N: only ${med_cyc} median cycles — cv_pct unreliable (increase RUNS_TINY)"
        fi

        printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
            "$label" "$N" "$RUNS" \
            "${med_cyc:-0}" "${med_ins:-0}" "${ipc:-0.00}" \
            "${med_cm:-0}"  "${med_br:-0}"  "${med_brm:-0}" \
            "${med_l1:-0}"  "${med_tlb:-0}" "${cv_pct:-0.0}" >> "$OUTPUT"

        echo -n "."
    done
    echo " done"
    sleep 1  # Thermal cooldown between structures
done

# Summary
duration=$SECONDS
minutes=$((duration / 60))
seconds=$((duration % 60))

echo ""
echo "=================================================="
echo "Results written to: $OUTPUT"
echo "Total runtime: ${minutes}m ${seconds}s"
echo "=================================================="