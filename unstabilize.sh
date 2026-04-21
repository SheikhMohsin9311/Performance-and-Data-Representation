#!/bin/bash
# unstabilize.sh — Restore system to normal defaults

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run with sudo."
    exit 1
fi

echo "=== Restoring System Defaults ==="

# 1. Reset CPU Governor to 'powersave' (standard for most Linux distros)
echo "Resetting CPU governor to 'powersave'..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo "powersave" > "$gov" 2>/dev/null || echo "ondemand" > "$gov"
done

# 2. Re-lock hardware performance counters (default is usually 3 or 4)
echo "Restoring perf counter paranoid level (default=3)..."
sysctl -w kernel.perf_event_paranoid=3

# 3. Re-enable NMI watchdog
echo "Re-enabling NMI watchdog..."
echo "1" > /proc/sys/kernel/nmi_watchdog

# 4. Re-enable Address Space Layout Randomization (ASLR)
echo "Re-enabling ASLR..."
echo "2" > /proc/sys/kernel/randomize_va_space

echo "=== System Restored to Normal State ==="
