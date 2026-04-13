# Performance & Data Representation

This suite measures the **Memory Wall**. We analyze how 16 different data structures collide with hardware reality: $L1/L2/L3$ caches, the hardware prefetcher, and the CPU pipeline. 

## The Mission
Modern CPUs are fast, but RAM is slow. If your data layout forces the CPU to wait for main memory, your $O(\log n)$ algorithm will lose to a cache-friendly $O(n)$ scan every time. We use `perf` and raw hardware counters to quantify these **stalls**.

---

## Quick Start
### 1. Fix hardware permissions
The Linux kernel restricts raw hardware counter access by default. You need to lower the paranoia level:
```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### 2. Run the benchmarking engine
The `runperf.sh` script orchestrates the entire suite. It pins execution to a single core to eliminate OS noise and sweeps through sizes from $10^3$ to $10^7$ with randomized jitter.
```bash
./runperf.sh
```
*Note: You can change `CORE_ID` at the top of the script if you want to pin to a different physical core.*

---

## The 16 Contenders: Hardware Deep-Dive

### Contiguous Power (Prefetcher Dominance)
These structures are stored in sequential memory blocks. They maximize **Spatial Locality**.
- **Array / Vector**: The gold standard. Data is perfectly packed. Every cache-line fetch (64 bytes) pulls in 16 integers. The **Hardware Prefetcher** detects the stride-1 pattern and fetches the next lines before the CPU even requests them. Result: **IPC > 2.0**.
- **AoS (Array of Structs)**: Good locality if you need all fields of an object at once. However, if you only need one field, you waste cache bandwidth on "garbage" fields in the same line.
- **SoA (Struct of Arrays)**: The "Data-Oriented Design" choice. Fields are separated into different arrays. This maximizes bandwidth for field-specific scans and is highly vectorized.
- **Heap**: A binary tree flattened into an array index-logic. Despite the tree semantics, the memory is contiguous. It shares the same prefetcher benefits as the array.
- **CircularBuffer**: Efficient contiguous queue. Minimizes allocation overhead and maintains spatial locality for producers and consumers.

### Pointer-Chasing (The Pipeline Killers)
These structures scatter nodes across the heap. They suffer from **Dependency Chains**.
- **LinkedList**: Every node is an individual heap allocation. To find node $i+1$, the CPU *must* wait for the `next` pointer of node $i$ to return from memory. This is a serial dependency that prevents out-of-order execution. Stalls are ~200 cycles per hop if the list exceeds $L3$ size.
- **BST / RBTree**: Classic search trees. Beyond pointer chasing, these add the cost of **Branch Mispredictions**. Every node is a potential 15-cycle penalty if the branch predictor gets the `left/right` choice wrong.
- **SkipList**: Probabilistic layering. Very heavy on memory because every node carries multiple pointers (4-8 on average), quickly exhausting cache capacity.
- **SlabList**: A optimized linked list that allocates nodes in contiguous "slabs". This improves locality within the slab but still hits pointer-chasing stalls at slab boundaries.
- **Trie**: Character-level prefixing. Extremely deep dependency chains. Small nodes are often padded by the allocator, wasting 50%+ of cache capacity on overhead.

### Hybrid & Segmented Layouts
- **Deque**: Elements in contiguous chunks (blocks) linked by a map. Excellent locality within a block (usually 128 items), but hits a cache miss when it jumps to the next heap-allocated block.
- **HashMap**: Contiguous bucket array provides a good base, but "Separate Chaining" (linked lists for collisions) degrades performance toward $O(n)$ pointer-chasing as the load factor grows.
- **BTree**: Designed for the memory hierarchy. High branching factor ($B=16+$) means one node fills exactly one or more cache lines. You stay on the same line longer before chasing the next pointer.
- **vEBTree (van Emde Boas)**: A recursive, "cache-oblivious" layout. It maps a tree into an array such that subtrees occupy contiguous blocks. This keeps related nodes on the same cache line regardless of cache size.

---

## The CSV Output: Metric Guide
Every row in your results represents one benchmark point. Here is what the numbers actually mean:

- **data_structure**: The name of the implementation being tested.
- **N**: The number of elements in the structure ($10^3$ to $10^7$).
- **runs**: The count of "Drowning Loop" iterations. We traverse the entire structure this many times to mask measurement overhead.
- **cycles**: The absolute number of CPU clock cycles elapsed. This is your primary "time" metric.
- **instructions**: Total "Retired Instructions". This is the actual work the CPU completed.
- **ipc (Instructions Per Cycle)**: The efficiency of your pipeline. 
    - **IPC > 1.5**: Your code is flying. The CPU is effectively parallelizing work.
    - **IPC < 0.5**: Your CPU is mostly doing nothing, waiting on RAM.
- **cache_misses**: Specifically **L3 Cache Misses**. These are the "expensive" misses that force the CPU to go all the way to Main Memory (DRAM), costing ~200+ cycles each.
- **branches**: The number of conditional jumps executed (if/else, loops).
- **branch_misses**: How many times the CPU's branch predictor guessed wrong. Each miss flushes the entire pipeline, costing ~15-20 cycles.
- **L1_misses**: Misses in the fastest, smallest cache ($L1$). High L1 misses mean your "working set" is larger than ~32KB.
- **TLB_misses**: Translation Lookaside Buffer misses. This happens when your data is spread across so many virtual memory pages that the CPU loses track of the physical addresses, forcing a "Page Table Walk".

---

## Benchmarking Engine
The `runperf.sh` suite is designed for **High-Density Performance Analysis**:
- **Dynamic Scaling**: We sweep from $10^3$ to $10^7$ with $\pm10\%$ jitter to ensure architectural transition points (L1 $\to$ L2 $\to$ L3 $\to$ RAM) are accurately captured.
- **Drowning Loops**: Every benchmark wraps its traversal in a high-iteration loop to ensure the CPU hits steady-state frequency and the measurements aren't dominated by process startup noise.
- **Hardware Metrics**: We track **Cycles**, **Instructions**, **IPC**, **Cache-Misses**, **Branch-Misses**, and **TLB-Stalls**.

## Analysis Pipeline
Data is saved to timestamped CSVs (`deep_scaling_results_*.csv`). Use `analysis.ipynb` to visualize:
- **Point Clouds**: See the performance "cloud" that reveals variance and hardware noise.
- **Memory Wall Heatmaps**: Observe the exact point where cycles-per-element spikes as $N$ exceeds specific cache capacities.
- **Stall Correlation**: Directly correlate L3 misses with IPC drops to prove where the CPU is waiting.

---
*Results vary by architecture. This suite has been validated on modern x86/ARM systems.*