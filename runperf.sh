#!/bin/bash

# 1. Initialize the built-in Bash timer
SECONDS=0

OUTPUT="deep_scaling_results.csv"

# ==========================================
# CONFIGURATION: REPEAT COUNTS (THE DROWNING LOOP)
# ==========================================
# Change these numbers to whatever you want to test!
RUNS_LARGE_DATA=10      # Used when N is 10,000,000 or more
RUNS_MEDIUM_DATA=100    # Used when N is 1,000,000
RUNS_SMALL_DATA=1000    # Used for N < 1,000,000
# ==========================================

# Request deep hardware events directly from the CPU
EVENTS="cycles,instructions,cache-misses,branches,branch-misses,L1-dcache-load-misses,dTLB-load-misses"

echo "data_structure,N,runs,cycles,instructions,ipc,cache_misses,branches,branch_misses,L1_misses,TLB_misses" > "$OUTPUT"

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

SIZES=(1000 10000 100000 1000000 10000000)

for entry in "${ENTRIES[@]}"; do
    binary="${entry%%:*}"
    label="${entry##*:}"

    if [ ! -f "./$binary" ]; then
        echo "Skipping $label (binary not found. Did you run 'make'?)"
        continue
    fi

    for N in "${SIZES[@]}"; do
        
        # Apply your custom repeat counts based on the size of N
        if [ "$N" -ge 10000000 ]; then
            RUNS=$RUNS_LARGE_DATA
        elif [ "$N" -ge 1000000 ]; then
            RUNS=$RUNS_MEDIUM_DATA
        else
            RUNS=$RUNS_SMALL_DATA
        fi

        echo -n "Running $label with N=$N (Runs=$RUNS) ... "
        
        # Run perf, capture raw CSV output. 
        # -x ',' forces comma-separation. 
        # > /dev/null silences the C++ program's standard output.
        raw_perf=$(taskset -c 0 perf stat -x ',' -e "$EVENTS" "./$binary" "$N" "$RUNS" 2>&1 > /dev/null)

        # Parse the specific metrics from the perf CSV output
        cycles=$(echo "$raw_perf" | grep "cycles" | cut -d',' -f1)
        instructions=$(echo "$raw_perf" | grep "instructions" | cut -d',' -f1)
        cache_misses=$(echo "$raw_perf" | grep "cache-misses" | cut -d',' -f1)
        branches=$(echo "$raw_perf" | grep "branches" | cut -d',' -f1)
        branch_misses=$(echo "$raw_perf" | grep "branch-misses" | cut -d',' -f1)
        l1_misses=$(echo "$raw_perf" | grep "L1-dcache-load-misses" | cut -d',' -f1)
        tlb_misses=$(echo "$raw_perf" | grep "dTLB-load-misses" | cut -d',' -f1)

        # Calculate IPC (and prevent division by zero)
        if [ -z "$cycles" ] || [ "$cycles" -eq 0 ]; then
            ipc="0.00"
            echo -n "(Warning: 0 cycles detected) "
        else
            ipc=$(awk "BEGIN {printf \"%.2f\", $instructions / $cycles}")
        fi

        # Append to your final deep dive CSV
        echo "$label,$N,$RUNS,$cycles,$instructions,$ipc,$cache_misses,$branches,$branch_misses,$l1_misses,$tlb_misses" >> "$OUTPUT"
        echo "done"
    done
done

# 2. Calculate and format the total runtime
duration=$SECONDS
minutes=$((duration / 60))
seconds=$((duration % 60))

echo ""
echo "=================================================="
echo "Deep analysis ready in $OUTPUT!"
echo "Total benchmark suite runtime: ${minutes}m ${seconds}s"
echo "=================================================="