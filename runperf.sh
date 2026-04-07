#!/bin/bash

OUTPUT="results.csv"
RUNS=1000

echo "data_structure,cycles,instructions,ipc,cache_misses,time_elapsed_sec" > "$OUTPUT"

# Format: "binary:label"
declare -a ENTRIES=(
    "second:BST"
    "third:Array"
    "linked_list:LinkedList"
    "heap:Heap"
    "hash_map:HashMap"
    "vector_ds:Vector"
    "deque_ds:Deque"
    "rb_tree:RBTree"
)

for entry in "${ENTRIES[@]}"; do
    binary="${entry%%:*}"
    label="${entry##*:}"

    if [ ! -f "./$binary" ]; then
        echo "[$label] binary not found, skipping"
        continue
    fi

    echo -n "Running $label ... "

    raw=$(perf stat -r "$RUNS" -e cycles,instructions,cache-misses "./$binary" 2>&1)

    # Extract cycles (first match — avoid matching 'stalled-cycles' lines)
    cycles=$(echo "$raw" | awk '/^[[:space:]]+[0-9,]+[[:space:]]+cycles/{gsub(/,/,"",$1); print $1; exit}')

    # Extract instructions and IPC
    instructions=$(echo "$raw" | awk '/instructions/{gsub(/,/,"",$1); print $1; exit}')
    ipc=$(echo "$raw" | grep -oP '[0-9]+\.[0-9]+(?=\s+insn per cycle)' | head -1)

    # Extract cache-misses
    cache_misses=$(echo "$raw" | awk '/cache-misses/{gsub(/,/,"",$1); print $1; exit}')

    # Extract elapsed time
    time_elapsed=$(echo "$raw" | grep -oP '^[[:space:]]*[0-9]+\.[0-9]+' | tail -1 | tr -d ' ')

    echo "$label,$cycles,$instructions,$ipc,$cache_misses,$time_elapsed" >> "$OUTPUT"
    echo "done"
done

echo ""
echo "Results written to $OUTPUT"
column -t -s ',' "$OUTPUT"