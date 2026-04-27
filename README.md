![The Memory Wall](https://img.shields.io/badge/Research-Systems%20Performance-blue) ![C11](https://img.shields.io/badge/Language-C11-green) ![Hardware](https://img.shields.io/badge/Architecture-Zen%203-orange)

# The Memory Wall: Data Structure scaling vs. Hardware Reality

> "Performance is no longer strictly bound by operation counts; it's heavily dictated by bandwidth efficiency and cache-line utilization."

This project is a high-fidelity research suite designed to quantify the **Memory Wall**—the widening performance gap between CPU speed and DRAM latency. While traditional academic analysis focuses on Big-O notation ($O(1)$, $O(\log N)$), this suite uses internal PMU (Performance Monitoring Unit) instrumentation to demonstrate that **hardware locality acts as a massive constant multiplier on asymptotic complexity**.

---

## 🏛️ Research Premise: The Architecture of Stalls
Modern CPUs consume instructions at a rate of 3–5 per cycle (IPC), but a single DRAM fetch takes $\approx 300$ cycles. When a data structure fails to utilize the cache hierarchy effectively, it triggers a pipeline starvation event.

**Key Latency Targets (Zen 3):**
*   **L1 Cache**: $\approx 1$ns (Top Tier)
*   **L3 Cache**: $\approx 15-20$ns (The Drop-off)
*   **DRAM (Main Memory)**: $100$ns+ (The Wall)

Once a data structure exceeds the 32MB L3 boundary, hardware locality dominates. For operations with identical Big-O bounds, locality dictates the winner. For differing bounds, locality defines the crossover point—the scale at which a structurally "worse" algorithm beats a "better" one due to cache efficiency.

---

## ⚠️ Scope and Limitations: The Traversal Bound
This suite explicitly measures the **Retrieval/Traversal Bound** of these structures by performing $O(N)$ full-scale aggregations. 

While Contiguous structures (like Arrays) win by up to 40x in pure traversal due to spatial locality, it is critical to acknowledge that pointer-based structures (BST, LinkedList) are designed for dynamic $O(\log N)$ searches and $O(1)$ mutations. The massive data-movement penalty ($O(N)$ `memmove`) required to insert into the middle of a contiguous array is not captured in this traversal-focused benchmark. 

Furthermore, we observe significant **statistical deviations between mean and median cycles** at certain scales. This is primarily due to:
*   **Context Switch Noise:** Even with core pinning, OS background tasks and hardware interrupts can induce high-latency outliers.
*   **Cold-start/Thermal Throttling:** Long sweeps can lead to slight frequency fluctuations despite locking the performance governor.
*   **Measurement Jitter:** Hardware PMU counters can exhibit non-deterministic behavior across different runs.
Because of these tail-latency events, we prioritize **Median** metrics for stability, though the Mean often reflects the real-world "jittery" performance an application would experience.


---

## ⚔️ The 16 Evaluated Structures
We evaluate three distinct families of data representation:

| Family | Categorization | Micro-Architectural Behavior |
| :--- | :--- | :--- |
| **Contiguous** | `Array`, `Vector`, `SoA`, `AoS`, `Heap` | **Prefetcher Friendly**. Maximizes spatial locality and IPC. |
| **Pointer-Chasing** | `LinkedList`, `BST`, `RBTree`, `Trie` | **Serial Dependency Chains**. Forces the CPU into idle wait-states. |
| **Hardware-Aware** | `B-Tree`, `vEB Tree`, `HashMap` | **Symmetric Alignment**. Fits data into 64-byte cache lines. |

---

## 🔬 Experimental Methodology
To achieve research-grade fidelity, the suite implements three critical bypass and isolation techniques:

### 1. Instrumentation Bypass (`perf_helper.h`)
We bypass standard timing syscalls (which incur jitter) and interact directly with the hardware performance counters using `perf_event_open`. We measure:
*   **IPC**: Instructions Per Cycle (Pipeline efficiency).
*   **LLC Misses**: Last Level Cache misses (DRAM trips).
*   **TLB Stalls**: Translation Lookaside Buffer exhaustion.

### 2. Allocator De-biasing (Worst-Case Isolation)
To isolate the true cost of pointer chasing, we prevent `malloc` from providing temporal/spatial locality by performing a **Fisher-Yates shuffle** on all pointer links before measurement. While real-world allocators offer some contiguous clustering, this shuffle enforces an absolute worst-case memory fragmentation scenario to fully expose the "Pointer Tax".

### 3. Environmental Isolation
The runner automatically locks the environment to ensure deterministic results:
*   **Core Pinning**: Pinned to `CORE_ID=1`.
*   **Frequency Locking**: Fixed at 3.5 GHz (prevents Turbo Boost jitter).
*   **Thermal Control**: Built-in cool-down periods between sweeps.

---

## 📊 Quickstart: Building & Running

### 🛡️ System Preparation (Linux)
Unlock the `perf` subsystem for unprivileged PMU access:
```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### 🔨 Build System
Our `Makefile` targets the C11 standard with aggressive optimizations:
```bash
# Release Build (-O3 -march=native -flto)
make clean && make

# Debug Build (-O0 -g)
make MODE=debug
```

### ✅ Tests
```bash
# Unit tests for perf_helper utilities
make test

# Unit tests + smoke test sweep for each benchmark binary
make check
```

### 🚀 Execute Sweep
Run the full performance spectrum ($10^3 \rightarrow 10^8$ elements):
```bash
./runperf.sh
```

---

## 📁 Repository Structure
*   `the_memory_wall.tex`: Final research manuscript (LaTeX).
*   `presentation.tex`: Beamer slide deck for academic pitch.
*   `perf_helper.h`: Syscall-based PMU instrumentation engine.
*   `runperf.sh`: Mass data collection orchestrator.
*   `*.c`: Pure C implementations of all 16 data structures.

---

## 📧 Citation & Contact
If you use this suite in your research or systems course, please cite:
**Mohsin, S. (2026). The Memory Wall: A Hardware-First Evaluation of Data Structure Scaling.**

---
*Developed by Sheikh Mohsin (Advanced Systems Performance Group).*
