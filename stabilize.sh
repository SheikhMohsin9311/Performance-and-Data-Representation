#!/bin/bash
# stabilize.sh — High-fidelity performance stabilization

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run with sudo."
    exit 1
fi

echo "=== Performance Stabilization Starting ==="

# 1. Set CPU Governor to 'performance' for all cores
echo "Setting CPU governor to 'performance'..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo "performance" > "$gov"
done

# 2. Unlock hardware performance counters
echo "Unlocking perf counters (paranoid=-1)..."
sysctl -w kernel.perf_event_paranoid=-1

# 3. Disable NMI watchdog (reduces background jitter)
echo "Disabling NMI watchdog..."
echo "0" > /proc/sys/kernel/nmi_watchdog

# 4. Kernel Address Space Layout Randomization (ASLR)
echo "0" > /proc/sys/kernel/randomize_va_space

echo "=== System Stabilized for Benchmarking ==="
