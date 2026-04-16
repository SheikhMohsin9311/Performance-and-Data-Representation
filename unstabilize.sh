#!/bin/bash
# unstabilize.sh — Restore system defaults after benchmarking.
# Run: sudo bash unstabilize.sh

set -euo pipefail

if [ "$EUID" -ne 0 ]; then
    echo "ERROR: Run as root: sudo bash unstabilize.sh"
    exit 1
fi

echo "=== Restoring system defaults ==="

# 1. Restore CPU governor to powersave
echo "  [1/4] Restoring CPU governor to 'powersave'..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo powersave > "$gov" 2>/dev/null || true
done

# 2. Restore ASLR
echo "  [2/4] Restoring ASLR (randomize_va_space = 2)..."
echo 2 > /proc/sys/kernel/randomize_va_space

# 3. Restore perf_event_paranoid
echo "  [3/4] Restoring perf_event_paranoid = 4..."
echo 4 > /proc/sys/kernel/perf_event_paranoid

# 4. Re-enable CPU idle states
echo "  [4/4] Re-enabling CPU C-states..."
for state in /sys/devices/system/cpu/cpu*/cpuidle/state*/disable; do
    echo 0 > "$state" 2>/dev/null || true
done

echo "=== Defaults restored ==="
