#!/bin/bash

# 1. Initialize the built-in Bash timer
SECONDS=0

# Dynamic output to allow data accumulation
OUTPUT="deep_scaling_results_$(date +%s).csv"

# ==========================================
# CONFIGURATION: REPEAT COUNTS & CORE PINNING
# ==========================================
CORE_ID=0              # Pin all benchmarks to this CPU core
BASE_RUNS_LARGE=3      # N >= 10M
BASE_RUNS_MEDIUM=30    # N >= 1M
BASE_RUNS_SMALL=300    # N < 1M
# ==========================================

# Request deep hardware events directly from the CPU
EVENTS="cycles,instructions,cache-misses,branches,branch-misses,L1-dcache-load-misses,dTLB-load-misses"

# Only write header if file is new (though $(date +%s) is likely new)
if [ ! -f "$OUTPUT" ]; then
    echo "data_structure,N,runs,cycles,instructions,ipc,cache_misses,branches,branch_misses,L1_misses,TLB_misses" > "$OUTPUT"
fi

# All 16 of your Data Structures!
declare -a ENTRIES=(
    "third:Array"
    "linked_list:LinkedList"
    "aos:AoS"
    "soa:SoA"
    "second:BST"
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

# REPLACED STATIC SIZES WITH DYNAMIC GENERATOR
# We sweep from 10^3 to 10^7 with multiple steps per decade
GENERATED_SIZES=()
for exp in 3 4 5 6; do
    for multiplier in 10 25 50 75; do
        base=$(( multiplier * 10**(exp-1) ))
        # Add random jitter: +/- 10% of the base
        jitter=$(( RANDOM % (base / 5 + 1) - base / 10 ))
        val=$(( base + jitter ))
        [[ $val -lt 100 ]] && val=100
        GENERATED_SIZES+=($val)
    done
done
GENERATED_SIZES+=(10000000) # Ensure we hit the 10M cap

for entry in "${ENTRIES[@]}"; do
    binary="${entry%%:*}"
    label="${entry##*:}"

    if [ ! -f "./$binary" ]; then
        echo "Skipping $label (binary not found. Did you run 'make'?)"
        continue
    fi

    echo "Benchmarking $label..."
    for N in "${GENERATED_SIZES[@]}"; do
        
        # Apply custom repeat counts with random jitter to explore variance
        if [ "$N" -ge 10000000 ]; then
            RUNS=$(( BASE_RUNS_LARGE + RANDOM % 3 - 1 )) 
        elif [ "$N" -ge 1000000 ]; then
            RUNS=$(( BASE_RUNS_MEDIUM + RANDOM % 10 - 5 ))
        else
            RUNS=$(( BASE_RUNS_SMALL + RANDOM % 60 - 30 ))
        fi
        [[ $RUNS -lt 1 ]] && RUNS=1


        # Run perf, capture raw CSV output. 
        # -x ',' forces comma-separation. 
        # > /dev/null silences the C++ program's standard output.
        raw_perf=$(taskset -c $CORE_ID perf stat -x ',' -e "$EVENTS" "./$binary" "$N" "$RUNS" 2>&1 > /dev/null)

        # Parse with awk - handle potential empty fields gracefully
        cycles=$(echo "$raw_perf" | awk -F',' '$3 == "cycles" { gsub(/[[:space:]]/,""); print $1 }')
        instrs=$(echo "$raw_perf" | awk -F',' '$3 == "instructions" { gsub(/[[:space:]]/,""); print $1 }')
        misses=$(echo "$raw_perf" | awk -F',' '$3 == "cache-misses" { gsub(/[[:space:]]/,""); print $1 }')
        brnch=$(echo "$raw_perf" | awk -F',' '$3 == "branches" { gsub(/[[:space:]]/,""); print $1 }')
        brms=$(echo "$raw_perf" | awk -F',' '$3 == "branch-misses" { gsub(/[[:space:]]/,""); print $1 }')
        l1ms=$(echo "$raw_perf" | awk -F',' '$3 == "L1-dcache-load-misses" { gsub(/[[:space:]]/,""); print $1 }')
        tlbms=$(echo "$raw_perf" | awk -F',' '$3 == "dTLB-load-misses" { gsub(/[[:space:]]/,""); print $1 }')

        # Ensure all variables are non-empty before math
        cycles=${cycles:-0}; instrs=${instrs:-0}; misses=${misses:-0}
        brnch=${brnch:-0}; brms=${brms:-0}; l1ms=${l1ms:-0}; tlbms=${tlbms:-0}

        # Calculate IPC safely using -v to pass variables
        ipc=$(awk -v ins="$instrs" -v cyc="$cycles" 'BEGIN { if (cyc != "" && cyc != "0") printf "%.2f", ins / cyc; else print "0.00" }')

        # Append to your final deep dive CSV
        printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
               "$label" "$N" "$RUNS" "$cycles" "$instrs" "$ipc" "$misses" "$brnch" "$brms" "$l1ms" "$tlbms" >> "$OUTPUT"
        echo " done"
    done
done

# 2. Calculate and format the total runtime
duration=$SECONDS
minutes=$((duration / 60))
seconds=$((duration % 60))

echo ""
echo "=================================================="
echo "High-density analysis ready in $OUTPUT!"
echo "Total benchmark suite runtime: ${minutes}m ${seconds}s"
echo "=================================================="