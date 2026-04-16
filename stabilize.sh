#!/bin/bash
# stabilize.sh — One-shot system tuning for reproducible benchmarks.
# Run ONCE before starting runperf.sh. Requires sudo.
# Restore defaults with: sudo bash unstabilize.sh

set -euo pipefail

if [ "$EUID" -ne 0 ]; then
    echo "ERROR: Run as root: sudo bash stabilize.sh"
    exit 1
fi

NCPU=$(nproc)

echo "=== Stabilizing system for benchmarking ==="

# 1. CPU frequency governor → performance (max clock, no throttling)
echo "  [1/5] Setting CPU governor to 'performance'..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo performance > "$gov"
done

# 2. Disable ASLR (address-space layout randomisation)
#    This makes allocator placement deterministic across runs.
echo "  [2/5] Disabling ASLR..."
echo 0 > /proc/sys/kernel/randomize_va_space

# 3. Lower perf_event_paranoid to allow user-space hardware counters
echo "  [3/5] Setting perf_event_paranoid = -1..."
echo -1 > /proc/sys/kernel/perf_event_paranoid

# 4. Disable CPU idle states (C-states) to prevent deep-sleep latency spikes
echo "  [4/5] Disabling CPU C-states (idle)..."
for state in /sys/devices/system/cpu/cpu*/cpuidle/state*/disable; do
    echo 1 > "$state" 2>/dev/null || true
done

# 5. Set process niceness hint for the benchmark runner
#    (runperf.sh uses nice -n -10 internally now)
echo "  [5/5] Done. Launch benchmarks with: bash runperf.sh"

echo ""
echo "=== System stabilized ==="
echo "  Governor:    performance"
echo "  ASLR:        disabled (0)"
echo "  Paranoid:    -1 (full hw counter access)"
echo "  C-states:    disabled"
echo ""
echo "Restore defaults when done: sudo bash unstabilize.sh"
