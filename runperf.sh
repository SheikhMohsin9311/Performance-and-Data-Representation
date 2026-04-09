#!/bin/bash

# 1. Initialize the built-in Bash timer
SECONDS=0

OUTPUT="deep_scaling_results.csv"

# ==========================================
# CONFIGURATION: REPEAT COUNTS (THE DROWNING LOOP)
# ==========================================
# Change these numbers to whatever you want to test!
RUNS_LARGE_DATA=1000    # Used when N is 10,000,000 or more
RUNS_MEDIUM_DATA=10000    # Used when N is 1,000,000
RUNS_SMALL_DATA=1000000   # Used for N < 1,000,000 
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

        # Run perf, capture raw CSV output. 
        # -x ',' forces comma-separation. 
        # > /dev/null silences the C++ program's standard output.
        raw_perf=$(taskset -c 0 perf stat -x ',' -e "$EVENTS" "./$binary" "$N" "$RUNS" 2>&1 > /dev/null)

        # Parse the specific metrics from the perf CSV output with awk for precision.
        # This prevents newline bleeding and field contamination.
        cycles=$(echo "$raw_perf" | awk -F',' '$3 == "cycles" { gsub(/ /,""); print $1 }')
        instrs=$(echo "$raw_perf" | awk -F',' '$3 == "instructions" { gsub(/ /,""); print $1 }')
        misses=$(echo "$raw_perf" | awk -F',' '$3 == "cache-misses" { gsub(/ /,""); print $1 }')
        brnch=$(echo "$raw_perf" | awk -F',' '$3 == "branches" { gsub(/ /,""); print $1 }')
        brms=$(echo "$raw_perf" | awk -F',' '$3 == "branch-misses" { gsub(/ /,""); print $1 }')
        l1ms=$(echo "$raw_perf" | awk -F',' '$3 == "L1-dcache-load-misses" { gsub(/ /,""); print $1 }')
        tlbms=$(echo "$raw_perf" | awk -F',' '$3 == "dTLB-load-misses" { gsub(/ /,""); print $1 }')

        # Calculate IPC (force float division with awk)
        ipc=$(awk "BEGIN { if (\"$cycles\" != \"\" && \"$cycles\" != \"0\") printf \"%.2f\", $instrs / $cycles; else print \"0.00\" }")

        # Ensure all variables are non-empty
        cycles=${cycles:-0}; instrs=${instrs:-0}; misses=${misses:-0}
        brnch=${brnch:-0}; brms=${brms:-0}; l1ms=${l1ms:-0}; tlbms=${tlbms:-0}

        # Append to your final deep dive CSV (all values on ONE physical line)
        printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
               "$label" "$N" "$RUNS" "$cycles" "$instrs" "$ipc" "$misses" "$brnch" "$brms" "$l1ms" "$tlbms" >> "$OUTPUT"
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