# Architectural Fidelity
### Data Locality, Memory Latency, and Hardware Determinism

This repository contains a high-fidelity systems research suite designed to quantify the impact of data representation on modern x86 hardware. By bypassing high-level abstractions, we measure the direct collision between 16 primitive data structures and the CPU's memory hierarchy: $L1/L2/L3$ caches, the hardware prefetcher, and the TLB.

---

## 🏛️ The Spatial Premise
Modern computing is increasingly bottlenecked by the **Memory Wall**. While algorithmic complexity ($O$ notation) dictates operation counts, **Spatial Locality** dictates execution latency. This project provides raw empirical evidence of how "optimal" algorithms can fail when their memory layouts induce pipeline stalls and cache exhaustion.

## 💎 High-Fidelity Research Stack

### 1. OS Stabilization Strategy
Benchmarking on a general-purpose OS is inherently noisy. We provide a **Stabilization Layer** to eliminate non-deterministic variance:
*   **`stabilize.sh`**: Locks CPU at peak frequency (`performance` governor), disables ASLR, thwarts C-state deep sleep, and grants unrestricted hardware counter access.
*   **Effect**: Reduces cycle variance from $\pm25\%$ to **$<2\%$**.

### 2. Discrete Hardware Instrumentation
We utilize a bespoke instrumentation header, `perf_helper.h`, to interface directly with the Linux `perf_event` subsystem.
*   **Internal Sampling**: We wrap only the "Drowning Loop" trajectory, excluding setup/teardown noise.
*   **Statistical Median**: Instead of global averages, we collect individual samples per iteration and compute the **Median** to ignore OS preemption outliers.
*   **Noise Detection**: We report the **Coefficient of Variation (CV%)** for every run; samples exceeding 15% noise are automatically flagged or discarded.

### 3. Structural Contenders
We analyze 16 distinct implementations across three architectural families:
*   **Contiguous**: Array, Vector, CircularBuffer, AoS, SoA, Heap.
*   **Linked**: LinkedList, SlabList, SkipList.
*   **Hybrid / Tree**: BST, RBTree, BTree, vEBTree, HashMap, Trie, Deque.

---

## 🚀 Usage Specification

### 1. System Preparation (Root Required)
```bash
sudo bash stabilize.sh
```

### 2. Compiling the Suite
The `Makefile` is optimized for high-performance research (`-O3 -march=native -flto`).
```bash
make clean && make -j$(nproc)
```

### 3. Executing the Benchmarks
The `runperf.sh` engine sweeps through working-set sizes from $10^3$ to $10^7$ elements.
```bash
./runperf.sh
```

### 4. Analysis & Visualisation
*   **Journal-Grade Analysis**: `analysis.py` processes the high-density dataset and generates 12+ publication-ready figures.
*   **The Manuscript**: `presentation.pdf` contains a boutique, 5-slide academic talk designed in a **Dark Academic** style.

---

## 📊 Sample Metrics Captured
| Metric | Significance |
|:---|:---|
| **Cycles/N** | Net architectural cost per element. |
| **IPC** | Pipeline efficiency (Instructions Per Cycle). |
| **L1/L3 Misses** | Direct indicators of memory hierarchy pressure. |
| **TLB Stalls** | Impact of virtual-to-physical address translation. |
| **CV%** | Metadata for measurement reliability/noise. |

---

## 📜 Intellectual Credit
**Sheikh Mohsin**  
*Systems Research | FLAME University*  
[Portfolio](https://sheikh-mohsin.vercel.app) | [GitHub](https://github.com/SheikhMohsin9311)

---
*Results are architecture-dependent. Validated on Linux kernel 5.15+ with hardware performance counter support.*