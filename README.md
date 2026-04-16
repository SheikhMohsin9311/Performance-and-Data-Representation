# Architectural Fidelity
### Data Locality, Memory Latency, and Hardware Determinism

This repository contains a high-fidelity systems research suite designed to quantify the impact of data representation on modern x86 hardware. By bypassing high-level abstractions, we measure the direct collision between 16 primitive data structures and the CPU's memory hierarchy: $L1/L2/L3$ caches, the hardware prefetcher, and the TLB.

## 🏛️ The Spatial Premise
Modern computing is increasingly bottlenecked by the **Memory Wall**. While algorithmic complexity ($O$ notation) dictates operation counts, **Spatial Locality** dictates execution latency. This project provide empirical evidence of how "optimal" algorithms can fail when their memory layouts induce pipeline stalls.

**Standardization**: All measurements utilize a uniform **Full Traversal** baseline, isolating cache-line utilization and pointer-chasing costs from operation-specific logic.

## 💎 High-Fidelity Research Stack

### 1. OS Stabilization Strategy
Benchmarking on a general-purpose OS is inherently noisy. We provide a **Stabilization Layer** to eliminate non-deterministic variance:
*   **`stabilize.sh`**: Locks CPU at peak frequency (`performance` governor), disables ASLR, thwarts C-state deep sleep, and grants unrestricted hardware counter access.
*   **Effect**: Reduces cycle variance from $\pm25\%$ to **$<2\%$**.

### 2. Statistical Remediation (METRICS3)
We utilize a bespoke instrumentation header, `perf_helper.h`, to interface directly with the Linux `perf_event` subsystem.
*   **Sampling Rigor**: We collect per-iteration samples and compute the **Bessel-Corrected Standard Deviation** ($n-1$) to account for small-sample variance.
*   **Confidence Intervals**: We calculate and report the **95% Confidence Interval (CI95)** for cycle counts, visualized as rigorous error bars in our analysis.
*   **Warmup Passes**: Every benchmark executes 3 full warmup traversals to prime the branch predictor and cache hierarchy before measurement.

### 3. Structural Contenders
We analyze 16 distinct implementations with genuine memory layouts:
*   **Contiguous**: Array, Vector, CircularBuffer, AoS, SoA, Heap.
*   **Hybrid / Recursive**: **vEB Tree** (Genuine recursive layout), **BTree** (Clustered nodes).
*   **Pointer-Based**: LinkedList, SlabList, SkipList, **Trie** (32-level binary), BST, RBTree, HashMap, Deque.

## 🚀 Usage Specification

### 1. System Preparation (Root Required)
```bash
sudo bash stabilize.sh
```

### 2. Compiling the Suite
The `Makefile` is optimized for high-performance research (`-O3 -march=native -flto`).
```bash
make clean && make check
```

### 3. Executing the Benchmarks
The `runperf.sh` engine sweeps through working-set sizes from $10^3$ to $10^7$ elements with tier-based iteration counts.
```bash
bash runperf.sh
```

### 4. Analysis & Visualisation
- **High-Density Plots**: `simplified_plots.py` generates 9+ publication-ready figures featuring **CI95 error bars**.
- **The Manuscript**: `presentation.pdf` is an 11-slide Beamer presentation designed in an **Elite Scholar** style, documenting the methodology and cache-pressure transitions.

## 📊 Sample Metrics Captured
| Metric | Significance |
|:---|:---|
| **Cycles/N** | Net architectural cost per element traversal. |
| **CI95** | 95% Confidence interval (Rigorous measurement stability). |
| **IPC** | Pipeline efficiency (Instructions Per Cycle). |
| **L1/L3 Misses** | Direct indicators of memory hierarchy pressure. |
| **TLB Stalls** | Impact of virtual-to-physical address translation. |

## 📜 Intellectual Credit
**Sheikh Mohsin**  
*Systems Research | FLAME University*  
[Portfolio](https://sheikh-mohsin.vercel.app) | [GitHub](https://github.com/SheikhMohsin9311)

*Results are architecture-dependent. Validated on Linux kernel 5.15+ with hardware performance counter support.*