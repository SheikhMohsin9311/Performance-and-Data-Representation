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
RUNS_LARGE=2            # N >= 10M
RUNS_MEDIUM=5          # N >= 1M
RUNS_SMALL=20          # N >= 100K
RUNS_TINY=50          # N < 100K — many reps to overcome os noise at tiny sizes
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
echo "data_structure,N,runs,med_cyc,avg_cyc,med_ins,avg_ins,ipc,med_cm,avg_cm,med_br,avg_br,med_brm,avg_brm,med_l1,avg_l1,med_tlb,avg_tlb,cv_pct,stddev_cycles,ci95_cycles" > "$OUTPUT"

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

# ==========================================
# MASTER ARCHIVE CONFIGURATION
# ==========================================
MASTER_ARCHIVE="${MASTER_ARCHIVE:-masterresultsOvernight.csv}"
CSV_HEADER="data_structure,N,runs,med_cyc,avg_cyc,med_ins,avg_ins,ipc,med_cm,avg_cm,med_br,avg_br,med_brm,avg_brm,med_l1,avg_l1,med_tlb,avg_tlb,cv_pct,stddev_cycles,ci95_cycles"

if [ ! -f "$MASTER_ARCHIVE" ]; then
    echo "$CSV_HEADER" > "$MASTER_ARCHIVE"
fi
# ==========================================

# ── Mixed Round-Robin Strategy ────────────────────────────────────────────────
# We sweep each decade, but within each decade, we iterate through ALL 
# structures for each N-step. This ensures "mixed" data from the start.

echo "Starting Mixed Round-Robin Sweep ($(( ${#ENTRIES[@]} )) structures, N up to 10^8) ..."

# Decade Definitions: min:max:step
DECADES=(
    "1000:10000:30"        # step 30  => ~300 pts/ds
    "10000:100000:300"     # step 300 => ~300 pts/ds
    "100000:1000000:3300"  # step 3.3k => ~270 pts/ds
    "1000000:10000000:33000" # step 33k => ~270 pts/ds
    "10000000:100000000:330000" # step 330k => ~270 pts/ds
)

# ==========================================
# PAUSE/RESUME STATE MANAGEMENT
# ==========================================
STATE_FILE=".benchmark_state"
RESUME_DECADE=0
RESUME_BASE=0

if [ -f "$STATE_FILE" ]; then
    IFS=':' read -r RESUME_DECADE RESUME_BASE < "$STATE_FILE"
    echo "Resuming from Decade Index $RESUME_DECADE, Base $RESUME_BASE"
fi
# ==========================================

for (( d_idx=0; d_idx<${#DECADES[@]}; d_idx++ )); do
    DECADE="${DECADES[$d_idx]}"
    
    # Skip completed decades
    if [ "$d_idx" -lt "$RESUME_DECADE" ]; then
        echo "Skipping already completed Decade $DECADE"
        continue
    fi

    IFS=':' read -r min max step <<< "$DECADE"
    echo "--- Scaling Range: $min to $max ---"
    
    for (( base=$min; base<$max; base+=$step )); do
        # Skip completed bases within the current decade
        if [ "$d_idx" -eq "$RESUME_DECADE" ] && [ "$base" -lt "$RESUME_BASE" ]; then
            continue
        fi

        # Randomize structure order slightly within each N-step
        SHUFFLED_ENTRIES=$(printf "%s\n" "${ENTRIES[@]}" | shuf)
        
        while IFS= read -r entry; do
            binary="${entry%%:*}"
            label="${entry##*:}"

            if [ ! -f "./bin/$binary" ]; then continue; fi

            # Jitter N per structure to cover more ground
            jitter=$(( RANDOM % (step / 2 + 1) ))
            if [ $(( RANDOM % 2 )) -eq 0 ]; then
                N=$(( base + jitter ))
            else
                N=$(( base - jitter ))
            fi
            
            # Clamp and bounds check
            [ "$N" -lt "$min" ] && N=$min
            [ "$N" -gt "$max" ] && N=$max
            [ "$N" -gt 100000000 ] && N=100000000

            # Tailored RUNS for speed/fidelity balance
            if [ "$N" -ge 10000000 ]; then RUNS=$RUNS_LARGE;
            elif [ "$N" -ge 1000000 ]; then RUNS=$RUNS_MEDIUM;
            elif [ "$N" -ge 100000 ]; then RUNS=$RUNS_SMALL;
            else RUNS=$RUNS_TINY; fi

            # Execution
            if ! exec_output=$(taskset -c $CORE_ID "./bin/$binary" "$N" "$RUNS" 2>&1); then
                printf " [FAIL: %s@%d]" "$label" "$N"
                continue
            fi

            # Parse & Log
            metrics_line=$(echo "$exec_output" | grep "^METRICS3" | tail -1)
            [ -z "$metrics_line" ] && continue

            perf_data="${metrics_line#METRICS3,}"
            IFS=',' read -r N_val med_cyc avg_cyc med_ins avg_ins ipc med_cm avg_cm med_br avg_br med_brm avg_brm med_l1 avg_l1 med_tlb avg_tlb cv_pct stddev ci95 <<< "$perf_data"

            row=$(printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s" \
                "$label" "$N" "$RUNS" \
                "${med_cyc:-0}" "${avg_cyc:-0}" \
                "${med_ins:-0}" "${avg_ins:-0}" "${ipc:-0.00}" \
                "${med_cm:-0}"  "${avg_cm:-0}" \
                "${med_br:-0}"  "${avg_br:-0}" \
                "${med_brm:-0}" "${avg_brm:-0}" \
                "${med_l1:-0}"  "${avg_l1:-0}" \
                "${med_tlb:-0}" "${avg_tlb:-0}" \
                "${cv_pct:-0.0}" "${stddev:-0}" "${ci95:-0}")

            echo "$row" >> "$OUTPUT"
            echo "$row" >> "$MASTER_ARCHIVE"
            
        done <<< "$SHUFFLED_ENTRIES"
        
        # Save checkpoint after successful N-step
        echo "$d_idx:$base" > "$STATE_FILE"
        printf "." 
    done
    printf "\n"
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