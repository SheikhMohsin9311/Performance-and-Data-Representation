#!/bin/bash
# runperf.sh — external perf stat sweep with ETA tracking.
# Usage: bash runperf.sh [--dry-run]
#   --dry-run  uses N=65536, REPEATS=3 for a quick sanity check.

set -euo pipefail

# ── Config ──────────────────────────────────────────────────────────────
OUTPUT="scaling_results.csv"
REPEATS=30
SIZES=(262144 524288 1048576 2097152 4194304 8388608 16777216)

if [[ "${1:-}" == "--dry-run" ]]; then
    echo "[DRY RUN] N=65536, repeats=3"
    SIZES=(65536); REPEATS=3; OUTPUT="dry_run.csv"
fi

EVENTS="cycles,ref-cycles,instructions,\
L1-dcache-loads,L1-dcache-load-misses,\
LLC-loads,LLC-load-misses,\
dTLB-loads,dTLB-load-misses,\
branches,branch-misses"

declare -a ENTRIES=(
    "BST:second"           "SortedArray:third"      "LinkedList:linked_list"
    "Heap:heap"            "HashMap:hash_map"        "Vector:vector_ds"
    "Deque:deque_ds"       "RBTree:rb_tree"          "AoS:aos"
    "SoA:soa"              "SkipList:skip_list"       "BTree:btree"
    "VEBTree:veb_tree"     "Trie:trie"                "SlabList:slab_list"
    "CircularBuffer:circular_buffer"
)

# ── Pre-flight ───────────────────────────────────────────────────────────
paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)
if [ "$paranoid" -gt 1 ]; then
    echo "ERROR: perf_event_paranoid=$paranoid. Run:"
    echo "  sudo sysctl kernel.perf_event_paranoid=1"
    exit 1
fi

missing=0
for entry in "${ENTRIES[@]}"; do
    bin="${entry##*:}"
    if [ ! -f "./$bin" ]; then echo "[MISSING] $bin"; missing=1; fi
done
[ "$missing" -eq 1 ] && echo "Run 'make' first." && exit 1

# ── ETA bookkeeping ──────────────────────────────────────────────────────
total_runs=$(( ${#ENTRIES[@]} * ${#SIZES[@]} ))
runs_done=0
start_epoch=$(date +%s)

eta_str() {
    local secs=$1
    printf "%dm%02ds" $(( secs / 60 )) $(( secs % 60 ))
}

# ── CSV header ───────────────────────────────────────────────────────────
echo "ds,N,\
cycles,ref_cycles,instructions,\
l1_loads,l1_misses,\
llc_loads,llc_misses,\
dtlb_loads,dtlb_misses,\
branches,branch_misses,\
ipc,cycles_per_elem,\
l1_miss_pct,llc_miss_pct,dtlb_miss_pct,branch_miss_pct" > "$OUTPUT"

# ── Parser ───────────────────────────────────────────────────────────────
# parse_event RAW EVENT_KEYWORD → mean integer count (commas stripped)
parse_event() {
    echo "$1" | awk -v kw="$2" '
    { for (i=1;i<=NF;i++) if($i==kw){ gsub(/,/,"",$1); print $1+0; exit } }'
}

# ── Main loop ────────────────────────────────────────────────────────────
for entry in "${ENTRIES[@]}"; do
    label="${entry%%:*}"; binary="${entry##*:}"

    for N in "${SIZES[@]}"; do
        # ── ETA ─────────────────────────────────────────────────────────
        now=$(date +%s)
        elapsed=$(( now - start_epoch ))
        if [ "$runs_done" -gt 0 ]; then
            avg=$(( elapsed / runs_done ))
            remain=$(( total_runs - runs_done ))
            eta=$(eta_str $(( avg * remain )))
        else
            eta="--:--"
        fi
        pct=$(( runs_done * 100 / total_runs ))

        printf "[%3d%%] %-18s N=%-10s ETA %-8s  " \
               "$pct" "[$label]" "$N" "$eta"

        # ── perf stat ───────────────────────────────────────────────────
        raw=$(taskset -c 0 perf stat -r "$REPEATS" -e "$EVENTS" \
              "./$binary" "$N" 2>&1)

        # Raw event means
        cycles=$(parse_event     "$raw" "cycles")
        ref_cyc=$(parse_event    "$raw" "ref-cycles")
        instrs=$(parse_event     "$raw" "instructions")
        l1_ld=$(parse_event      "$raw" "L1-dcache-loads")
        l1_ms=$(parse_event      "$raw" "L1-dcache-load-misses")
        llc_ld=$(parse_event     "$raw" "LLC-loads")
        llc_ms=$(parse_event     "$raw" "LLC-load-misses")
        dtlb_ld=$(parse_event    "$raw" "dTLB-loads")
        dtlb_ms=$(parse_event    "$raw" "dTLB-load-misses")
        branches=$(parse_event   "$raw" "branches")
        br_ms=$(parse_event      "$raw" "branch-misses")

        # Derived (safe div-by-zero throughout)
        ipc=$(awk       "BEGIN { printf \"%.4f\", ($cycles  >0)?$instrs/$cycles    :0 }")
        cpe=$(awk       "BEGIN { printf \"%.2f\",  ($N      >0)?$cycles/$N         :0 }")
        l1p=$(awk       "BEGIN { printf \"%.3f\",  ($l1_ld  >0)?$l1_ms/$l1_ld*100 :0 }")
        llcp=$(awk      "BEGIN { printf \"%.3f\",  ($llc_ld >0)?$llc_ms/$llc_ld*100:0 }")
        dtlbp=$(awk     "BEGIN { printf \"%.3f\",  ($dtlb_ld>0)?$dtlb_ms/$dtlb_ld*100:0 }")
        brp=$(awk       "BEGIN { printf \"%.3f\",  ($branches>0)?$br_ms/$branches*100:0 }")

        echo "$label,$N,$cycles,$ref_cyc,$instrs,\
$l1_ld,$l1_ms,$llc_ld,$llc_ms,$dtlb_ld,$dtlb_ms,$branches,$br_ms,\
$ipc,$cpe,$l1p,$llcp,$dtlbp,$brp" >> "$OUTPUT"

        echo "ipc=${ipc}  llc=${llcp}%  br=${brp}%"
        runs_done=$(( runs_done + 1 ))
    done
done

# ── Summary ──────────────────────────────────────────────────────────────
total_elapsed=$(( $(date +%s) - start_epoch ))
echo ""
echo "=== Done in $(eta_str $total_elapsed) — $OUTPUT ==="
echo "Rows: $(( $(wc -l < "$OUTPUT") - 1 )) data rows, 19 columns"
echo ""
column -t -s ',' "$OUTPUT"