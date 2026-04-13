# Performance & Data Representation

This suite measures the **Memory Wall**. We analyze how 16 different data structures collide with hardware reality: $L1/L2/L3$ caches, the hardware prefetcher, and the CPU pipeline. 

## The Mission
Modern CPUs are fast, but RAM is slow. If your data layout forces the CPU to wait for main memory, your $O(\log n)$ algorithm will lose to a cache-friendly $O(n)$ scan every time. We use `perf` and raw hardware counters to quantify these **stalls**.

---

## Technical Stack & Organization
- **Core language**: C11
- **Optimization**: `-O3 -march=native -flto`
- **Isolation**: Workloads pinned to `CORE_ID=1` to avoid OS noise.
- **Organization**: All compiled binaries are stored in the `./bin/` directory.

---

## 🛠️ Build System
The project features a professional-grade `Makefile` with support for different build modes.

### 1. Standard build (Release)
Optimized for peak performance benchmarking.
```bash
make clean && make
```
### 2. Debug build
Equipped with `-O0` and debug symbols for troubleshooting or profiling.
```bash
make MODE=debug
```
### 3. Cleanup
```bash
make clean
```

---

## 🚀 Benchmarking Engine
The `runperf.sh` script orchestrates the entire high-density suite.

### 1. Fix hardware permissions
The Linux kernel restricts raw hardware counter access by default. Lower the paranoia level:
```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### 2. Run the engine
The engine sweeps through sizes from $10^3$ to $10^7$ with randomized jitter and pins to independent CPU cores.
```bash
./runperf.sh
```

---

## 💎 High-Fidelity Measurement Architecture
To capture raw hardware impact without noise, we use two advanced techniques:

### 1. Internal Loop Instrumentation
We no longer measure the *entire* process (which captures initialization noise). Instead, we use `perf_helper.h` to open raw hardware counters via `perf_event_open`.
*   **Measurement Boundary**: We wrap ONLY the "Drowning Loop" in start/stop calls.
*   **Isolated Scorecard**: We capture 7 simultaneous metrics (Cycles, IPC, L3 Misses, Branches, Branch Misses, L1 Misses, TLB Stalls) per measurement point.

### 2. Memory Layout Randomization
To defeat the "Allocator Bias" (where sequential `malloc` helps the prefetcher), we implement a **Memory Shake** for all pointer-based structures:
*   Before measurement, we shuffle the node links to create true randomized pointer-chasing. 
*   **Result**: Linked structure IPC drops from ~0.9 (legacy/polluted) to **~0.03** (realistic 'Memory Wall').

---

## The 16 Contenders: Hardware Deep-Dive

### Contiguous Power (Prefetcher Dominance)
These structures maximize **Spatial Locality**.
- **Array / Vector**: Perfectly packed. Result: **IPC > 2.0**.
- **AoS / SoA**: Contrast between Object-Oriented and Data-Oriented designs.
- **Heap**: Binary tree flattened into array logic for prefetcher friendliness.
- **CircularBuffer**: Efficient contiguous queue with minimal allocation overhead.

### Pointer-Chasing (The Pipeline Killers)
These structures scatter nodes, creating **Dependency Chains**.
- **LinkedList**: Every hop costs cycles (~200 if list hits RAM).
- **BST / RBTree**: Search trees suffering from **Branch Mispredictions**.
- **SkipList**: Probabilistic layering with massive pointer overhead.
- **SlabList**: Optimized list that improves locality via contiguous "slabs".
- **Trie**: Deep dependency chains for prefix matching.

### Hybrid & Segmented Layouts
- **Deque**: Contiguous chunks linked by a map; hits stalls at chunk boundaries.
- **HashMap**: Base contiguous array vs. "Separate Chaining" overhead.
- **BTree**: Cache-line optimized trees with high branching factors ($B=16+$).
- **vEBTree**: Recursive, cache-oblivious layout for optimal locality.

---

## The CSV Output: Metric Guide
- **cycles**: Primary "time" metric (absolute clock cycles).
- **ipc**: Efficiency of the pipeline (**IPC > 1.5** is flying; **IPC < 0.5** is stalling).
- **cache_misses**: Specifically **L3 Cache Misses** (the 200+ cycle killers).
- **branch_misses**: Penalties from the branch predictor guessing wrong.
- **TLB_misses**: Stalls caused by virtual memory "Page Table Walks".

---

## Analysis Pipeline
Integrated data analysis using `analysis.ipynb`:
- **Concurrency Guard**: The notebook automatically skips active CSV files being written to by the benchmark script.
- **Universal Aggregation**: Aggregates all `*results*.csv` files, including legacy and multi-session data.
- **Visualization**: Generates optimized point clouds, Memory Wall heatmaps, and stall correlation graphs.

---
*Results vary by architecture. This suite has been validated on modern x86/ARM systems.*