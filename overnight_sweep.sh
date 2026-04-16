#!/bin/bash
# overnight_sweep.sh — Persistent wrapper for massive data collection

TARGET_ROWS=100000
ARCHIVE="masterresultsOvernight.csv"

echo "=== Overnight Massive Sweep Starting ==="
echo "Targeting $TARGET_ROWS rows in $ARCHIVE"

# 1. Initial Disk Check
FREE_MB=$(df -m . | tail -1 | awk '{print $4}')
if [ "$FREE_MB" -lt 500 ]; then
    echo "Error: Less than 500MB free disk space. Aborting."
    exit 1
fi

# 2. Infinite Loop until stopped or target met
while true; do
    CURRENT_ROWS=$(wc -l < "$ARCHIVE" 2>/dev/null || echo 0)
    echo "Current archive size: $CURRENT_ROWS rows"
    
    if [ "$CURRENT_ROWS" -ge "$TARGET_ROWS" ]; then
        echo "Target of $TARGET_ROWS reached. Sweep complete."
        break
    fi

    echo "Launching runperf.sh sweep..."
    export MASTER_ARCHIVE="$ARCHIVE"
    bash runperf.sh
    
    echo "Sweep pass finished. Cooling down for 10 seconds..."
    sleep 10
done

echo "=== Massive Sweep Finished ==="
