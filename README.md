Measuring Clock Cycles of Data Structure Traversals in C++ Using Linux perf

## A Complete, Line-by-Line Reference
## Table of Contents

1. [What Is a Clock Cycle](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#1-what-is-a-clock-cycle)
2. [The CPU Pipeline and Why Cycles Are Not Instructions](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#2-the-cpu-pipeline-and-why-cycles-are-not-instructions)
3. [Hardware Performance Counters](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#3-hardware-performance-counters)
4. [The Linux perf Subsystem](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#4-the-linux-perf-subsystem)
5. [perf_event_paranoid and Security](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#5-perf_event_paranoid-and-security)
6. [The perf_event_open Syscall](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#6-the-perf_event_open-syscall)
7. [The PerfCounter Header — Every Line Explained](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#7-the-perfcounter-header--every-line-explained)
8. [Why We Do Not Use perf stat for This](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#8-why-we-do-not-use-perf-stat-for-this)
9. [Memory and Cache Hierarchy](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#9-memory-and-cache-hierarchy)
10. [Why Data Structure Layout Determines Performance](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#10-why-data-structure-layout-determines-performance)
11. [Data Structure 1 — Binary Search Tree (second.cpp)
12. [Data Structure 2 — Sorted Array (third.cpp)](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#12-data-structure-2--sorted-array-thirdcpp)
13. [Data Structure 3 — Singly Linked List (linked_list.cpp)](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#13-data-structure-3--singly-linked-list-linked_listcpp)
14. [Data Structure 4 — Binary Heap (heap.cpp)](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#14-data-structure-4--binary-heap-heapcpp)
15. [Data Structure 5 — Hash Map (hash_map.cpp)
16. [Data Structure 6 — Vector (vector_ds.cpp)](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#16-data-structure-6--vector-vector_dscpp)
17. [Data Structure 7 — Deque (deque_ds.cpp)](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#17-data-structure-7--deque-deque_dscpp)
18. [Data Structure 8 — Red-Black Tree (rb_tree.cpp)](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#18-data-structure-8--red-black-tree-rb_treecpp)
19. [The Makefile — Every Directive Explained](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#19-the-makefile--every-directive-explained)
20. [The Shell Script — Every Line Explained](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#20-the-shell-script--every-line-explained)
21. [Reading the CSV Output](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#21-reading-the-csv-output)
22. [The Volatile Keyword — Why It Is Not Optional](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#22-the-volatile-keyword--why-it-is-not-optional)
23. [Compiler Optimization and What -O2 Actually Does](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#23-compiler-optimization-and-what--o2-actually-does)
24. [Warmup Runs — Why They Exist](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#24-warmup-runs--why-they-exist)
25. [Statistical Validity of a Single Cycle Count](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#25-statistical-validity-of-a-single-cycle-count)
26. [Interpreting Results — What the Numbers Mean](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#26-interpreting-results--what-the-numbers-mean)
27. [Academic Papers and Further Reading](https://claude.ai/chat/cf56a02a-6ca1-4b55-849d-2cc999d27e5c#27-academic-papers-and-further-reading)

---

## 1. What Is a Clock Cycle

A clock cycle is the fundamental unit of time inside a CPU. Every CPU contains a crystal oscillator — a small piece of quartz that vibrates at a fixed, precise frequency when an electric current is passed through it. These vibrations generate a regular electrical pulse called the clock signal. On each rising edge of that pulse, the CPU advances its internal state machines by one step.

If your CPU runs at 3 GHz, the oscillator vibrates 3,000,000,000 times per second. Each one of those vibrations is one clock cycle. The duration of a single clock cycle at 3 GHz is therefore:

```
1 / 3,000,000,000 = 0.000000000333... seconds = 0.333 nanoseconds
```

So one clock cycle lasts about a third of a nanosecond. When you measure 129,000,000 cycles for a tree traversal, you are measuring:

```
129,000,000 × 0.333 ns ≈ 43 milliseconds
```

This matches the `0.079 seconds time elapsed` figure from `perf stat` — the rest of the time is spent on tree construction, memory allocation, and process startup overhead, which is exactly why we isolate just the traversal using `perf_event_open`.

### Why clock cycles are a better unit than time

Time in seconds is affected by:

- Other processes running on the same CPU core
- The operating system scheduler preempting your process
- CPU frequency scaling (turbo boost, power saving states)
- Temperature-induced throttling

Clock cycles are affected only by the actual work your code performs plus the stalls caused by cache misses and data dependencies. They are reproducible across identical runs on the same machine in a way that wall-clock time is not. This is why performance engineers always prefer cycles as the primary metric.

---

## 2. The CPU Pipeline and Why Cycles Are Not Instructions

Modern CPUs do not execute one instruction per cycle. They use a technique called **pipelining** — overlapping the execution of multiple instructions simultaneously by breaking each instruction into stages.

A simplified 5-stage pipeline looks like this:

```
Stage 1: Fetch     — Read the instruction bytes from memory (or instruction cache)
Stage 2: Decode    — Interpret the opcode and identify operands
Stage 3: Execute   — Perform the arithmetic or logic operation
Stage 4: Memory    — Read or write data memory if needed
Stage 5: Writeback — Store the result back into a register
```

At any moment, five different instructions can be in five different stages simultaneously. This means the CPU can theoretically complete one instruction per cycle — an IPC (Instructions Per Cycle) of 1.0.

Modern superscalar CPUs go further and have multiple execution units running in parallel. A processor like Intel's Skylake or AMD's Zen 3 can retire up to 4–6 instructions per cycle under ideal conditions. The theoretical peak IPC is therefore 4–6.

When you see IPC = 0.60 in your BST traversal, it means the CPU is spending 40% of its cycles doing nothing — waiting. It is stalled. The reason it is stalled is almost always **memory latency**: it has issued a load instruction (`mov rax, [rbx]` — load the value at the address stored in rbx) and it cannot proceed until that load returns from memory. If the address is not in cache, it has to wait for RAM, which takes 60–80 nanoseconds — roughly 180–240 cycles at 3 GHz. During those 240 cycles, the pipeline is starved.

This is the central story of your benchmark. Every data structure is fundamentally a question of: **how much does traversal force the CPU to wait for memory?**

---

## 3. Hardware Performance Counters

The CPU's Performance Monitoring Unit (PMU) is a set of special-purpose hardware registers built into the processor die. These registers sit alongside the execution units and increment atomically every time a specific hardware event occurs.

The events tracked include:

|Event Name|What Increments the Counter|
|---|---|
|`PERF_COUNT_HW_CPU_CYCLES`|Every clock cycle the core is not halted|
|`PERF_COUNT_HW_INSTRUCTIONS`|Every instruction retired (completed, not speculated away)|
|`PERF_COUNT_HW_CACHE_MISSES`|Every LLC (Last Level Cache) miss|
|`PERF_COUNT_HW_CACHE_REFERENCES`|Every LLC access|
|`PERF_COUNT_HW_BRANCH_MISSES`|Every branch the predictor got wrong|
|`PERF_COUNT_HW_STALLED_CYCLES_BACKEND`|Cycles where the back-end execution units were stalled|

These counters are hardware — they cost zero overhead to increment. They are not implemented in software. The PMU registers are 64-bit on modern CPUs, so they will not overflow during a short benchmark.

Intel CPUs since Sandy Bridge (2011) typically have 4 "programmable" counter registers and 3 "fixed" counter registers per core. AMD CPUs have a similar structure. The "fixed" counters always track cycles, instructions, and reference cycles. The "programmable" counters can be configured to track any event the CPU supports.

The full list of events supported by your specific CPU is documented in the **Intel Software Developer's Manual, Volume 3B** (Performance Monitoring chapter) or AMD's equivalent **BKDG (BIOS and Kernel Developer's Guide)**. The Linux kernel abstracts all of this behind the `perf_event_open` syscall.

---

## 4. The Linux perf Subsystem

Linux exposes hardware performance counters to userspace through the `perf_event` subsystem, introduced in kernel 2.6.31 (2009). This subsystem:

1. Accepts a configuration structure describing which event you want to count
2. Opens a file descriptor that represents that counter on a specific CPU core for a specific process
3. Lets you start, stop, reset, and read the counter through `ioctl` calls on that file descriptor
4. Allows you to read the count using the standard `read()` syscall

The `perf` command-line tool (`perf stat`, `perf record`) is a userspace frontend to this kernel subsystem. It calls `perf_event_open` internally, the same syscall we use directly in our code.

The advantage of calling `perf_event_open` ourselves is surgical precision — we open and close the counter around exactly the code we want to measure, rather than letting `perf stat` measure the entire process lifetime including startup, memory allocation, and teardown.

---

## 5. perf_event_paranoid and Security

The file `/proc/sys/kernel/perf_event_paranoid` controls which users can access hardware performance counters. The kernel introduced this restriction because hardware counters can be used as side-channel attack vectors — by measuring cache behavior precisely, a process can infer what another process is doing (this is the foundation of cache-timing attacks like Flush+Reload).

The paranoia levels are:

|Value|Effect|
|---|---|
|`-1`|All users can use all events including raw hardware events|
|`0`|Users can use CPU events (hardware counters) for their own processes|
|`1`|Users can use CPU events but cannot profile the kernel|
|`2`|Only root can use CPU events; users are limited to software counters|
|`3` or `4`|Hardware counters completely disabled for non-root users|

Your system had `perf_event_paranoid = 4`, which is typical on Ubuntu 22.04+ where Canonical tightened the defaults. Setting it to `1` is the minimal change needed:

```bash
sudo sysctl kernel.perf_event_paranoid=1
```

This lets your process count hardware events for itself (not for the kernel, not for other processes), which is all we need.

To make it permanent across reboots:

```bash
echo 'kernel.perf_event_paranoid = 1' | sudo tee -a /etc/sysctl.conf
```

---

## 6. The perf_event_open Syscall

The syscall signature is:

```c
int perf_event_open(
    struct perf_event_attr *attr,   // Configuration structure
    pid_t pid,                      // Which process to monitor
    int cpu,                        // Which CPU core to monitor
    int group_fd,                   // Group leader fd (for event groups)
    unsigned long flags             // Flags
);
```

The return value is a file descriptor, or `-1` on failure.

**Arguments in our usage:**

```c
syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0)
```

- `&pe` — pointer to the configured `perf_event_attr` structure
- `0` — `pid = 0` means "monitor the calling process"
- `-1` — `cpu = -1` means "on any CPU core" (follow the process as the scheduler moves it)
- `-1` — `group_fd = -1` means this counter is not part of a group; it stands alone
- `0` — no special flags

### The perf_event_attr structure — every field we set

```c
struct perf_event_attr pe;
```

`perf_event_attr` is a large structure (128 bytes). We zero-initialize it with `memset` so all fields we do not explicitly set default to 0.

```c
memset(&pe, 0, sizeof(pe));
```

This is critical. The kernel rejects the syscall if any reserved field is non-zero, treating it as an unknown request from a newer API version. Zero-initializing ensures we do not pass garbage from the stack.

```c
pe.type = PERF_TYPE_HARDWARE;
```

`type` tells the kernel which kind of event we want. `PERF_TYPE_HARDWARE` means a hardware PMU event. Other options include `PERF_TYPE_SOFTWARE` (kernel software counters like page faults), `PERF_TYPE_TRACEPOINT` (kernel trace events), and `PERF_TYPE_RAW` (raw CPU-specific event codes from the Intel manual).

```c
pe.config = PERF_COUNT_HW_CPU_CYCLES;
```

`config` specifies which hardware event within the given type. `PERF_COUNT_HW_CPU_CYCLES` is the total cycle counter — it maps to the hardware "unhalted core cycles" event (Intel event code `0x003C`). The kernel translates this generic name to the correct CPU-specific event code at runtime, so the same code works on Intel and AMD CPUs.

```c
pe.disabled = 1;
```

`disabled = 1` means the counter starts in a paused state. It will not begin counting until we explicitly call `ioctl(fd, PERF_EVENT_IOC_ENABLE, 0)`. Without this, the counter would start counting from the moment `perf_event_open` returns — counting cycles spent in the warmup run, memory allocation, and any code before we want to measure.

```c
pe.exclude_kernel = 1;
```

`exclude_kernel = 1` tells the PMU to stop incrementing the counter whenever the CPU is executing in kernel mode (ring 0). Without this, every system call, page fault handler, and interrupt handler invoked during our traversal would add cycles to our count. Since we are measuring userspace data structure traversal, kernel cycles are noise. This flag removes them.

```c
pe.exclude_hv = 1;
```

`exclude_hv = 1` excludes cycles spent in the hypervisor (ring -1, used in virtualized environments). On bare metal this has no effect but it is good practice to set it for portability.

---

## 7. The PerfCounter Header — Every Line Explained

```cpp
#pragma once
```

A non-standard but universally supported preprocessor directive that tells the compiler to include this file only once per translation unit, regardless of how many times it is `#include`d. It is equivalent to a traditional include guard (`#ifndef PERF_UTIL_H` / `#define` / `#endif`) but cleaner.

```cpp
#include <cstdint>
```

Provides fixed-width integer types. We use `uint64_t` to hold the cycle count — a 64-bit unsigned integer that will not overflow for any reasonable measurement (at 3 GHz, it would take ~194 years to overflow a 64-bit counter).

```cpp
#include <cstring>
```

Provides `memset`, used to zero-initialize the `perf_event_attr` structure.

```cpp
#include <unistd.h>
```

Provides `read()` and `close()` — POSIX system call wrappers. We use `read()` to extract the counter value from the file descriptor, and `close()` in the destructor to release the kernel resource.

```cpp
#include <sys/ioctl.h>
```

Provides `ioctl()` — Input/Output Control. `ioctl` is a generic mechanism to send control commands to file descriptors. We use three commands on our perf file descriptor:

- `PERF_EVENT_IOC_RESET` — zero the counter
- `PERF_EVENT_IOC_ENABLE` — start counting
- `PERF_EVENT_IOC_DISABLE` — stop counting

```cpp
#include <sys/syscall.h>
```

Provides `SYS_perf_event_open` — the syscall number for `perf_event_open`. There is no C library wrapper function for this syscall (it was intentionally omitted from glibc), so we must invoke it through the raw `syscall()` function.

```cpp
#include <linux/perf_event.h>
```

Provides the `perf_event_attr` structure definition, the `PERF_TYPE_*` constants, the `PERF_COUNT_HW_*` constants, and the `PERF_EVENT_IOC_*` ioctl codes. This is a Linux kernel header installed in `/usr/include/linux/`.

```cpp
struct PerfCounter {
    int fd;
```

`fd` is the file descriptor returned by `perf_event_open`. It is the kernel's handle to our configured performance counter. All subsequent operations (enable, disable, read, close) are performed through this integer handle. File descriptors are small non-negative integers; `0`, `1`, `2` are stdin, stdout, stderr, and subsequent `open()`/`socket()`/`perf_event_open()` calls get the next available integer, typically starting from `3`.

```cpp
    PerfCounter() {
        perf_event_attr pe;
        memset(&pe, 0, sizeof(pe));
```

The constructor. `perf_event_attr pe` allocates the structure on the stack (not the heap). `sizeof(pe)` is 128 bytes on current kernel versions, so this is a 128-byte stack allocation. `memset` fills all 128 bytes with zero. This is mandatory — the kernel checks the `size` field and any reserved fields for non-zero values.

```cpp
        pe.type           = PERF_TYPE_HARDWARE;
        pe.config         = PERF_COUNT_HW_CPU_CYCLES;
        pe.disabled       = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv     = 1;
```

As explained in Section 6.

```cpp
        fd = syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0);
    }
```

The raw syscall. `SYS_perf_event_open` is the integer syscall number — on x86-64 Linux this is `298`. The return value is stored in `fd`. A production implementation would check `fd == -1` and call `perror()` to print the error, but we omit that for clarity.

```cpp
    void start() {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
```

`start()` first resets the counter to zero, then enables it. We always reset before enabling because the counter may have been used before (or left in an indeterminate state). The third argument to `ioctl` here is `0` — this is the argument passed to the ioctl command. For `PERF_EVENT_IOC_RESET` and `PERF_EVENT_IOC_ENABLE`, the argument is a flags bitmask where `0` means "apply to this counter only, not the entire group." After `start()` returns, the hardware PMU register is incrementing on every clock cycle.

```cpp
    uint64_t stop() {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        uint64_t count = 0;
        read(fd, &count, sizeof(count));
        return count;
    }
```

`stop()` first disables the counter — the PMU register stops incrementing. Then `read(fd, &count, sizeof(count))` reads the counter value from the kernel into our `count` variable. The kernel copies the current 64-bit counter value into the 8 bytes pointed to by `&count`. The `read` call is not expensive — it is a fast path in the kernel that copies 8 bytes from a kernel structure into userspace. We return the count, which represents the number of clock cycles elapsed between `start()` and `stop()`.

```cpp
    ~PerfCounter() { close(fd); }
};
```

The destructor closes the file descriptor, releasing the kernel resource. If we omitted this, the file descriptor would leak until the process exits. For a short benchmark program this is harmless, but it is good practice.

---

## 8. Why We Do Not Use perf stat for This

`perf stat ./second` measures from `execve()` (process start) to `exit()` (process end). This includes:

1. Dynamic linker loading the C++ runtime (`ld-linux.so`)
2. C++ runtime initialization (global constructors, `atexit` handlers)
3. `main()` entry
4. `srand(42)` — seeding the random number generator
5. The loop allocating and inserting 100,000 nodes (`new Node()` × 100,000 — this involves `malloc` internally, which calls `brk()` or `mmap()` system calls to request memory pages from the OS)
6. The warmup traversal
7. **The measured traversal** ← this is all we actually want
8. `freeTree()` — another 100,000 `delete` calls
9. C++ runtime teardown

Steps 1–6 and 8–9 are noise relative to step 7. The tree construction alone involves 100,000 `new` calls and 100,000 comparisons plus pointer assignments. This can easily cost more cycles than the traversal itself.

By using `perf_event_open` directly, we start the counter on the line immediately before `inorder(root)` and stop it on the line immediately after. The counter is cold (disabled) during everything else.

---

## 9. Memory and Cache Hierarchy

This section is essential for interpreting any of the benchmark results. The CPU does not read from RAM directly — it uses a hierarchy of caches, each faster and smaller than the one below it.

A typical modern system looks like this:

```
 Registers       ~0 cycles      ~1 KB per core
     |
 L1 Cache        ~4 cycles      32–64 KB per core (split: 32KB data, 32KB instruction)
     |
 L2 Cache        ~12 cycles     256 KB – 1 MB per core
     |
 L3 Cache        ~40 cycles     8–32 MB shared across all cores
     |
 RAM (DRAM)      ~200 cycles    8–64 GB
     |
 SSD/HDD         ~millions      Terabytes
```

When the CPU executes a load instruction (e.g., `mov rax, [rbx]` — load the 8-byte value at the address in `rbx` into `rax`), it:

1. Checks the L1 data cache. If the data is there (L1 hit), it takes ~4 cycles.
2. If not in L1, checks L2. If there (L2 hit), ~12 cycles.
3. If not in L2, checks L3. If there (L3 hit), ~40 cycles.
4. If not in L3 (L3 miss / cache miss), goes to RAM. ~200 cycles.

Caches work on **cache lines** — not individual bytes. A cache line is typically 64 bytes. When any byte of a 64-byte block is loaded, the entire 64-byte block is fetched and stored in the cache. This means that if your data is laid out contiguously in memory (like an array), loading one element pulls in ~7 adjacent elements "for free." The next 7 accesses cost ~4 cycles each instead of ~200 cycles each.

This spatial locality benefit is the entire reason arrays and vectors beat linked lists and trees in traversal benchmarks. It is not a software phenomenon — it is a consequence of how memory hardware works at the physics level.

---

## 10. Why Data Structure Layout Determines Performance

When you call `new Node()` in C++, the allocator (`malloc` underneath) returns a pointer to some free region of the heap. The heap is not organized — successive calls to `new` return addresses that are scattered across the virtual address space, determined by the allocator's internal free list and the history of previous allocations and deallocations.

For a BST built by inserting 100,000 randomly valued elements, each node is allocated separately, at an address determined by the allocator. The left and right pointers in each node point to addresses that are typically hundreds or thousands of bytes apart in memory, often on different cache lines, often in different pages of physical RAM.

When the inorder traversal does `inorder(root->left)`, it dereferences `root->left` — a pointer load. The CPU does not know in advance what value that pointer holds. It must wait for the load to complete before it knows which address to fetch next. This is called a **pointer-chasing dependency chain** — each load depends on the result of the previous load, so they cannot be parallelized or reordered. The CPU's prefetcher cannot predict the next address because pointer values are data, not patterns.

An array, by contrast, stores elements at addresses `arr`, `arr+4`, `arr+8`, `arr+12`, ... The pattern is completely regular. The hardware prefetcher recognizes sequential access patterns and begins fetching cache lines before the CPU even requests them. By the time the loop iteration reaches `arr[i]`, the data is already sitting in L1 cache.

This is why cache miss counts and IPC tell the story of every data structure's memory behavior.

---

## 11. Data Structure 1 — Binary Search Tree (second.cpp)

### The Node Structure

```cpp
struct Node {
    int data;
    Node* left;
    Node* right;
};
```

`struct Node` occupies `sizeof(int) + sizeof(Node*) + sizeof(Node*)` bytes. On a 64-bit system, `int` is 4 bytes, `Node*` is 8 bytes each, so: `4 + 4 (padding) + 8 + 8 = 24 bytes`. The 4-byte padding is inserted by the compiler between `data` and `left` because pointers must be aligned to 8-byte boundaries on x86-64. Each `Node` therefore occupies exactly one-third of a 64-byte cache line, meaning each cache line holds at most 2 complete nodes (2 × 24 = 48 bytes, with 16 bytes unused).

Since nodes are allocated individually with `new Node()`, adjacent nodes in the tree (parent and child) are almost certainly on different cache lines — they may even be on different physical memory pages (4 KB apart). This maximizes cache miss frequency during traversal.

### newNode

```cpp
Node* newNode(int val) {
    Node* n = new Node();
```

`new Node()` calls the global allocation function `operator new(sizeof(Node))`, which internally calls `malloc(24)`. `malloc` manages a heap free list — it either carves 24 bytes from an existing free block or calls `brk()`/`mmap()` to request a new page from the OS. The returned pointer `n` is the address of the allocated 24-byte region.

```cpp
    n->data = val;
    n->left = nullptr;
    n->right = nullptr;
    return n;
}
```

The `->` operator dereferences `n` (treats `n` as the base address) and adds the field offset (0 bytes for `data`, 8 bytes for `left`, 16 bytes for `right`) to form the effective address for each store instruction. `nullptr` is the C++11 keyword for the null pointer — a pointer with value 0.

### insert

```cpp
Node* insert(Node* root, int val) {
    if (!root) return newNode(val);
```

The base case — if `root` is null (we have reached a leaf's child slot), allocate a new node here.

```cpp
    if (val < root->data)
        root->left = insert(root->left, val);
    else
        root->right = insert(root->right, val);
    return root;
}
```

This is a recursive BST insertion. `root->data` is a load from the current node's memory address. `root->left` and `root->right` are further pointer loads. For a tree of 100,000 nodes, the recursion depth is expected O(log N) ≈ 17 levels on average for a balanced tree, but with random insertion order, the expected height is approximately 2.5 × ln(N) ≈ 29 levels.

### inorder

```cpp
void inorder(Node* root) {
    if (!root) return;
    inorder(root->left);
    volatile int x = root->data;
    inorder(root->right);
}
```

The inorder traversal visits nodes in sorted order: left subtree, current node, right subtree. Each recursive call pushes a stack frame containing the return address and saved registers (at least 8 bytes, typically 16–32 bytes due to alignment). For 100,000 nodes with a recursion depth of ~29, the maximum stack depth at any point is 29 frames × ~32 bytes = ~928 bytes — well within the default 8 MB stack limit.

`volatile int x = root->data` — the `volatile` qualifier tells the compiler that this variable may be observed by external agents (hardware, other threads, signal handlers) and that its reads and writes must not be optimized away. Without `volatile`, the compiler would see that `x` is written but never read by any subsequent code and would eliminate the assignment entirely, potentially eliminating the `root->data` load, which could allow it to eliminate entire subtree traversals as dead code. With `volatile`, every node's `data` field is actually loaded from memory, ensuring the traversal is real work.

### main

```cpp
const int N = 100000;
```

`const int N` is a compile-time constant. The compiler substitutes this value inline wherever `N` is used — `N` itself may not occupy any memory in the executable.

```cpp
srand(42);
```

Seeds the C standard library pseudo-random number generator with the value `42`. This makes all subsequent `rand()` calls produce the same sequence every run, ensuring deterministic tree structure across all benchmark runs. Without this, the tree shape (and therefore cache behavior) would vary between runs.

```cpp
for (int i = 0; i < N; i++)
    root = insert(root, rand());
```

`rand()` returns a pseudo-random integer between 0 and `RAND_MAX` (at least 32767, typically 2,147,483,647 on Linux). The loop inserts 100,000 such values into the BST.

```cpp
inorder(root);  // warm up
inorder(root);  // measured
```

The first call brings tree nodes into L3 cache (or L2 if the tree fits). The second call is the one we measure. We do this because the first traversal's cache miss pattern is dominated by compulsory misses (cold cache) — every node is a miss simply because it has never been accessed before. By the second traversal, nodes that fit in L3 will be cached. This gives us a more stable and representative measurement of the data structure's steady-state cache behavior.

---

## 12. Data Structure 2 — Sorted Array (third.cpp)

```cpp
int arr[N];
```

`int arr[N]` with `N = 100000` declares a stack-allocated array of 100,000 integers. Total size: `100,000 × 4 bytes = 400,000 bytes = 400 KB`. This is larger than the L1 cache (typically 32 KB) and L2 cache (typically 256 KB) on most systems, so the entire array does not fit in L2. It does fit entirely in L3 cache (typically 8–32 MB).

```cpp
srand(42);
for (int i = 0; i < N; i++) arr[i] = rand();
std::sort(arr, arr + N);
```

We fill with the same random seed (42) as the BST so both contain the same set of values, then sort so traversal visits values in ascending order — matching what inorder BST traversal produces.

`std::sort` uses introsort (a hybrid of quicksort, heapsort, and insertion sort) and runs in O(N log N) time. This is not measured.

```cpp
for (int i = 0; i < N; i++) sum += arr[i];
```

The traversal. `arr[i]` compiles to a load from address `arr + 4*i`. The address of the next element is always `current address + 4`. This is a perfectly regular stride-1 (in units of `int` size) sequential access pattern. The L1 hardware prefetcher is designed specifically to detect this pattern and will prefetch cache lines ahead of the current access. By the time the loop reaches `arr[i]`, the cache line containing `arr[i]` (which also contains `arr[i+1]` through `arr[i+15]`) has already been fetched into L1. The effective cost of each access approaches ~4 cycles rather than ~200 cycles.

---

## 13. Data Structure 3 — Singly Linked List (linked_list.cpp)

```cpp
struct Node {
    int data;
    Node* next;
};
```

Each node is `4 + 4 (padding) + 8 = 16 bytes`. Same caveats as BST nodes — individually heap-allocated, scattered across memory.

```cpp
Node* head = nullptr;
Node* tail = nullptr;
```

`head` tracks the first node (for traversal). `tail` tracks the last node (for O(1) append). Without `tail`, appending would require walking the entire list each time — O(N²) total for building the list.

```cpp
if (!head) head = tail = n;
else { tail->next = n; tail = n; }
```

First element: both `head` and `tail` point to it. Subsequent elements: the current tail's `next` pointer is updated to point to the new node, then `tail` advances to the new node. This gives O(1) append.

### Traversal

```cpp
Node* cur = head;
while (cur) { sum += cur->data; cur = cur->next; }
```

`cur->next` is a pointer load. The address loaded into `cur` on each iteration is the value stored in `cur->next`, which was stored there at allocation time — a heap address determined by `malloc`. There is no pattern to these addresses. The hardware prefetcher cannot predict them. Every node access is potentially an L3 miss.

The linked list and BST share the same fundamental pathology — pointer chasing — but the linked list is even simpler (no branching, purely sequential pointer following), so it provides a clean baseline for the pointer-chasing cost.

---

## 14. Data Structure 4 — Binary Heap (heap.cpp)

```cpp
int arr[N];
```

The key insight about a heap: it is stored as a flat array. A node at index `i` has:

- Left child at index `2*i + 1`
- Right child at index `2*i + 2`
- Parent at index `(i-1) / 2`

This implicit structure (no pointers at all) means the heap is stored identically to a plain array in memory. All 100,000 integers are contiguous. The traversal is indistinguishable from the array traversal from a memory access perspective.

```cpp
void heapifyDown(int* arr, int n, int i) {
    int smallest = i;
    int l = 2*i+1, r = 2*i+2;
    if (l < n && arr[l] < arr[smallest]) smallest = l;
    if (r < n && arr[r] < arr[smallest]) smallest = r;
    if (smallest != i) {
        std::swap(arr[i], arr[smallest]);
        heapifyDown(arr, n, smallest);
    }
}
```

`heapifyDown` enforces the min-heap property: every parent is smaller than its children. It compares the element at `i` with its children and swaps with the smallest if necessary, then recurses on the swapped position. This is O(log N) per call.

```cpp
for (int i = N/2 - 1; i >= 0; i--)
    heapifyDown(arr, N, i);
```

Floyd's algorithm — build a valid heap from an unordered array in O(N) time by heapifying from the last internal node down to the root. Nodes at indices `N/2` through `N-1` are leaf nodes (they have no children) and already satisfy the heap property trivially.

The traversal itself is identical to the array traversal — sequential scan of `arr[0]` through `arr[N-1]`. Cache behavior will be essentially identical to the array.

---

## 15. Data Structure 5 — Hash Map (hash_map.cpp)

```cpp
std::unordered_map<int,int> mp;
mp.reserve(N * 2);
```

`std::unordered_map` is C++'s standard hash table. It uses **separate chaining**: an array of "buckets" where each bucket is a linked list of entries that hash to the same index.

`mp.reserve(N * 2)` pre-allocates at least `2N` buckets. The default max load factor is 1.0 (one entry per bucket on average). By reserving `2N` buckets for `N` entries, we ensure a load factor of 0.5, which minimizes collision chains and improves traversal locality slightly.

Even with pre-allocation, `unordered_map` stores each key-value pair as a heap-allocated node (in most standard library implementations, GCC's libstdc++ included). This means each `insert` results in a `new` allocation, and iteration (the traversal) follows pointers from each bucket's linked list — pointer chasing, similar to the linked list.

The memory layout during traversal is: scan the bucket array (contiguous, good), then for each non-empty bucket follow its linked list pointer (heap-allocated, scattered, bad). The mix of these two patterns makes the hash map fall somewhere between the array and the linked list in practice.

```cpp
for (auto& kv : mp) sum += kv.second;
```

`kv` is a `std::pair<const int, int>`. `kv.second` is the value (the insertion index). The range-based for loop calls `mp.begin()` and `mp.end()` to get iterators, then calls `++` on the iterator which advances through the bucket array and linked list chains.

---

## 16. Data Structure 6 — Vector (vector_ds.cpp)

```cpp
std::vector<int> v;
v.reserve(N);
```

`std::vector<int>` is a dynamically-sized array. Internally it stores:

- `T* _data` — pointer to heap-allocated contiguous storage
- `size_t _size` — current number of elements
- `size_t _capacity` — current allocated storage capacity

`v.reserve(N)` calls `operator new(N * sizeof(int))` = `new int[100000]` = 400 KB contiguous allocation. All 100,000 integers will be stored adjacent in memory. Without `reserve`, the vector would start small and double its capacity every time it fills up (geometric growth), resulting in up to log₂(100000) ≈ 17 reallocations during the build phase. We `reserve` to avoid this and ensure the storage is contiguous from the start.

```cpp
v.push_back(rand());
```

`push_back` checks if `_size < _capacity`. Since we reserved N and we call push_back N times, `_size` never reaches `_capacity`, so no reallocation occurs. The element is stored at `_data[_size]` and `_size` is incremented.

The traversal is semantically identical to the array traversal. `std::vector` is designed to have zero overhead over a raw array for element access.

---

## 17. Data Structure 7 — Deque (deque_ds.cpp)

```cpp
std::deque<int> dq;
```

`std::deque` (double-ended queue) is the most complex standard container in terms of memory layout. It does **not** store elements in a single contiguous block. Instead it uses a **map** — an array of pointers, each pointing to a fixed-size **chunk** (also called a block or segment) of contiguous elements.

In libstdc++ (GCC's standard library), the chunk size for `int` is 512 bytes, holding `512 / 4 = 128` integers per chunk. So 100,000 integers require `ceil(100000 / 128) = 782` chunks, each separately heap-allocated.

The memory layout looks like:

```
map: [ptr0] [ptr1] [ptr2] ... [ptr781]
       |       |       |
    [128 ints][128 ints][128 ints]...
```

During traversal, the iterator walks sequentially through each chunk (good locality within a chunk — 128 sequential ints), then follows a pointer to the next chunk (a potential cache miss at each chunk boundary).

This gives the deque intermediate cache behavior: better than linked lists (128 sequential accesses between pointer hops), but worse than a vector (no pointer hops at all).

---

## 18. Data Structure 8 — Red-Black Tree (rb_tree.cpp)

```cpp
std::set<int> s;
```

`std::set<int>` in libstdc++ is implemented as a **red-black tree** — a self-balancing BST that guarantees O(log N) insert, delete, and search. Each node in the internal tree stores:

- The value (`int`)
- Color bit (red or black)
- Parent pointer
- Left child pointer
- Right child pointer

The internal node type in libstdc++ (`_Rb_tree_node`) is approximately 40 bytes on a 64-bit system. Like our hand-written BST, each node is individually heap-allocated.

The difference from our BST is that the red-black tree rebalances on insertion (via rotations and recoloring), keeping height bounded at 2 × log₂(N+1) ≈ 34 for N = 100,000. Our random-insertion BST has expected height ~29 and worst-case N = 100,000 (degenerate case of sorted insertion). In practice for random input the heights are similar, and both have the same pathological pointer-chasing cache behavior.

```cpp
for (int x : s) sum += x;
```

`std::set`'s iterator does an inorder traversal of the red-black tree — visiting nodes in sorted order by following parent and child pointers. This is the same pointer-chasing access pattern as our hand-written `inorder()`.

---

## 19. The Makefile — Every Directive Explained

```makefile
CXX = g++
```

Defines a Makefile variable named `CXX`. The convention is that `CXX` holds the C++ compiler command. This is referenced as `$(CXX)` in rules. Defining it as a variable means we can change the compiler for all targets by editing one line.

```makefile
CXXFLAGS = -O2
```

`CXXFLAGS` holds compiler flags applied to all compilation commands. `-O2` enables optimization level 2 — a middle ground between compile time and runtime performance. It enables: inlining of small functions, loop unrolling, instruction scheduling, common subexpression elimination, and dead code elimination, without the aggressive transformations (like auto-vectorization at full width) that `-O3` enables. We use `-O2` because it reflects realistic production build settings.

```makefile
TARGETS = second third linked_list heap hash_map vector_ds deque_ds rb_tree
```

A space-separated list of all binary targets. Referenced in the `all` and `clean` rules.

```makefile
all: $(TARGETS)
```

The first target in a Makefile is the default — running `make` with no arguments builds this target. `all` depends on `$(TARGETS)`, so Make will build all binaries.

```makefile
second: second.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
```

A pattern rule. The target is `second`, the prerequisite is `second.cpp`. The recipe (the indented line — **this indentation must be a tab character, not spaces**) runs when `second` does not exist or when `second.cpp` has been modified more recently than `second`.

`$@` is an automatic variable meaning "the target name" — expands to `second`. `$<` is an automatic variable meaning "the first prerequisite" — expands to `second.cpp`.

So the full command is: `g++ -O2 -o second second.cpp`

```makefile
clean:
	rm -f $(TARGETS)
```

The `clean` target removes all compiled binaries. `-f` means "force" — do not error if the files do not exist. Running `make clean` followed by `make` gives a guaranteed fresh build.

---

## 20. The Shell Script — Every Line Explained

```bash
#!/bin/bash
```

The shebang line. When the OS executes this file, it reads the first two bytes — `#!` — as the "interpreter magic number" and uses the rest of the line (`/bin/bash`) as the interpreter to invoke. So the OS runs `/bin/bash run_perf.sh`, which starts the Bash shell and passes the script as input.

```bash
OUTPUT="results.csv"
RUNS=1000
```

Shell variables. In Bash, variable assignment has no spaces around `=`. `OUTPUT` holds the filename for our CSV. `RUNS` holds the number of repetitions to pass to `perf stat -r`.

```bash
echo "data_structure,cycles,instructions,ipc,cache_misses,time_elapsed_sec" > "$OUTPUT"
```

`echo` prints the string to stdout. `>` redirects stdout to the file named `$OUTPUT`, creating it if it does not exist or **truncating it if it does**. This writes the CSV header row. Quoting `"$OUTPUT"` protects against filenames containing spaces.

```bash
declare -a ENTRIES=(
    "second:BST"
    ...
)
```

`declare -a` declares an indexed array. Each element is a string in the format `binary:label`. The colon is our delimiter — chosen because it cannot appear in a C++ binary filename or a reasonable label.

```bash
for entry in "${ENTRIES[@]}"; do
```

Iterates over all elements of the `ENTRIES` array. `"${ENTRIES[@]}"` expands to all elements, each quoted separately, so elements with spaces would be handled correctly (none do here, but the quoting is best practice).

```bash
    binary="${entry%%:*}"
    label="${entry##*:}"
```

Parameter expansion operators:

- `${entry%%:*}` — remove the longest match of `:*` from the **end** of `entry`. Since `entry = "second:BST"`, `:*` matches `:BST`, so the result is `second`.
- `${entry##*:}` — remove the longest match of `*:` from the **beginning** of `entry`. `*:` matches `second:`, so the result is `BST`.

This splits `second:BST` into `binary=second` and `label=BST` without calling `cut`, `awk`, or `sed` — pure Bash, no subprocess overhead.

```bash
    if [ ! -f "./$binary" ]; then
        echo "[$label] binary not found, skipping"
        continue
    fi
```

`[ ! -f "./$binary" ]` tests whether the file does not exist (`!`) as a regular file (`-f`). If the binary was not compiled (e.g., `make` failed for that target), we skip it gracefully rather than letting `perf stat` produce a confusing error.

```bash
    raw=$(perf stat -r "$RUNS" -e cycles,instructions,cache-misses "./$binary" 2>&1)
```

`$( ... )` is command substitution — the command inside runs in a subshell and its stdout is captured into the variable `raw`. `perf stat` writes its output to **stderr**, not stdout. `2>&1` redirects file descriptor 2 (stderr) to file descriptor 1 (stdout), so the command substitution captures it. Without `2>&1`, `raw` would be empty and the perf output would print directly to the terminal unprocessed.

```bash
    cycles=$(echo "$raw" | awk '/^[[:space:]]+[0-9,]+[[:space:]]+cycles/{gsub(/,/,"",$1); print $1; exit}')
```

Parses the cycle count from the perf output. Breaking this down:

- `echo "$raw"` — print the captured perf output
- `|` — pipe to awk
- `/^[[:space:]]+[0-9,]+[[:space:]]+cycles/` — awk pattern: match lines that start with whitespace, then digits-and-commas, then whitespace, then the word `cycles`. The `^` anchors to line start.
- `{gsub(/,/,"",$1); print $1; exit}` — awk action: `gsub(/,/,"",$1)` removes all commas from field 1 (the number), `print $1` prints it, `exit` stops processing after the first match (we use `exit` because "stalled-cycles-backend" lines also contain the word "cycles" and we want only the first match).

```bash
    ipc=$(echo "$raw" | grep -oP '[0-9]+\.[0-9]+(?=\s+insn per cycle)' | head -1)
```

`grep -oP` enables Perl-compatible regex and prints only the matching portion (`-o`). The pattern `[0-9]+\.[0-9]+(?=\s+insn per cycle)` uses a **lookahead** `(?=...)` — match a decimal number only if it is followed by whitespace and `insn per cycle`. This extracts the IPC value without capturing the surrounding text. `head -1` takes the first match in case the pattern appears multiple times.

```bash
    echo "$label,$cycles,$instructions,$ipc,$cache_misses,$time_elapsed" >> "$OUTPUT"
```

`>>` appends to the file (does not truncate, unlike `>`). This adds one CSV row per data structure.

```bash
column -t -s ',' "$OUTPUT"
```

`column` is a utility that formats text into aligned columns. `-t` enables table mode (auto-detect column widths), `-s ','` sets the delimiter to comma. This prints a human-readable table to the terminal after all runs complete. The CSV file itself remains unmodified.

---

## 21. Reading the CSV Output

The CSV has these columns:

|Column|Meaning|
|---|---|
|`data_structure`|Name of the data structure|
|`cycles`|Clock cycles for the traversal only (when using `perf_event_open` instrumentation)|
|`instructions`|CPU instructions retired during the traversal|
|`ipc`|Instructions per cycle = `instructions / cycles`|
|`cache_misses`|LLC (L3) cache misses during the traversal|
|`time_elapsed_sec`|Wall-clock time (includes build, warmup, and teardown)|

### Expected ordering (fastest to slowest in cycles):

1. **Vector** — contiguous, compiler vectorizes the loop with SIMD
2. **Array** — identical to vector
3. **Heap** — identical to array (same contiguous layout)
4. **Deque** — mostly contiguous with periodic pointer hops
5. **HashMap** — bucket array plus chained nodes
6. **Linked List** — pure pointer chasing, no structure
7. **BST** — pointer chasing plus branching overhead
8. **RB Tree** — pointer chasing plus larger node size plus inorder iterator overhead

---

## 22. The Volatile Keyword — Why It Is Not Optional

The C++ compiler's optimizer performs **dead code elimination**: if a variable is written but never read by any code path that produces an observable side effect, the write (and any computation that feeds into it) is eliminated.

Consider this code without `volatile`:

```cpp
void inorder(Node* root) {
    if (!root) return;
    inorder(root->left);
    int x = root->data;   // written, never read — compiler eliminates this
    inorder(root->right);
}
```

With `-O2`, the compiler performs whole-function analysis. It sees that `x` is never used after assignment. It therefore eliminates the assignment. Then it sees that `root->data` is now not needed (nothing reads it). It eliminates the load. Then, in some compilers, it may determine that the recursive calls produce no side effects and eliminate the entire function body, reducing `inorder` to a single `ret` instruction.

You would then benchmark a function that returns immediately, measuring 0 clock cycles for the traversal.

With `volatile int x`:

```cpp
volatile int x = root->data;
```

The compiler is required by the C++ standard to perform this load and this store, regardless of whether `x` is subsequently read. The `volatile` qualifier tells the compiler that the variable has side effects observable outside the normal program flow (hardware registers, signal handlers, etc.) and must not be optimized away. This forces every node's `data` field to be actually loaded from memory during traversal, making the benchmark measure real cache behavior.

An alternative is to accumulate into a `volatile` variable outside the traversal:

```cpp
volatile long long sink = 0;
// ...
void inorder(Node* root) {
    if (!root) return;
    inorder(root->left);
    sink += root->data;  // sink is volatile — load of root->data is real
    inorder(root->right);
}
```

This is slightly less intrusive but the effect is the same.

---

## 23. Compiler Optimization and What -O2 Actually Does

`-O0` (no optimization): The compiler generates code that closely mirrors the source — every variable lives in a stack slot, every expression is computed and stored, no instructions are reordered. This is easy to debug but slow. Function calls are not inlined. Loop variables are loaded from stack on every iteration.

`-O1`: Enables basic optimizations — dead store elimination, constant folding, simple inlining of trivially small functions, basic register allocation (variables go into registers instead of stack slots where possible).

`-O2`: Enables all `-O1` optimizations plus: loop-invariant code motion (hoist computations out of loops), inlining of small-to-medium functions, instruction scheduling (reorder instructions to avoid pipeline stalls), tail call optimization, branch prediction hints, CSE (common subexpression elimination). This is the standard production optimization level.

`-O3`: Enables all `-O2` plus: aggressive vectorization (using SIMD instructions like SSE, AVX), loop unrolling (duplicate loop body to reduce branch overhead), function cloning for constant propagation. Can make code larger.

For our benchmarks, `-O2` is appropriate because:

1. It reflects real-world build settings
2. It removes artificial bottlenecks from unoptimized code (stack spills, redundant loads) without changing the fundamental memory access pattern we are trying to measure

---

## 24. Warmup Runs — Why They Exist

When a program first accesses a memory region, several things happen that do not happen on subsequent accesses:

**Page faults**: Virtual memory pages are not mapped to physical RAM until first access. The first access triggers a **minor page fault** — the kernel maps a physical page to the virtual address. This is a kernel operation costing thousands of cycles per page. Subsequent accesses to the same virtual address have no page fault cost.

**Cold cache (compulsory misses)**: Every cache line that has never been loaded will be a cache miss on first access, regardless of the data structure's layout quality. These are called compulsory misses — they are unavoidable on first touch. The warmup run pays this cost so the measured run can show the steady-state behavior.

**Branch predictor training**: The CPU's branch predictor learns patterns over time. The first traversal of a BST teaches the predictor that `if (!root) return` is usually not taken (since most nodes exist). On subsequent runs, the predictor is pre-trained.

**TLB warming**: The Translation Lookaside Buffer (TLB) caches virtual-to-physical address mappings. On first access, TLB misses occur (requiring a page table walk costing ~30–100 cycles each). After the warmup, frequently-accessed pages are in the TLB.

By doing one warmup traversal before measuring, we ensure we are measuring the data structure's behavior under realistic operating conditions — the same conditions a long-running server process would experience — rather than the one-time startup penalty.

---

## 25. Statistical Validity of a Single Cycle Count

A single run of `perf_event_open` measurement gives you one data point. This is subject to:

- **OS preemption**: The scheduler may preempt your process mid-traversal and run another process for a few milliseconds. The counter continues counting during preemption if `pe.exclude_kernel = 0`, or pauses if `= 1` (which we set). Even with `exclude_kernel`, returning from preemption causes cache eviction — the scheduler's code and other processes' data displace our tree nodes from cache.
    
- **Interrupt handling**: Hardware interrupts (timer, network, disk) invoke kernel handlers that briefly interrupt execution. With `exclude_kernel = 1`, these cycles are excluded from the count, but the cache disruption is not undone.
    
- **Thermal throttling**: If the CPU is hot, it reduces its frequency. With `cycles` (not `ref-cycles`), the count would be lower even for the same amount of work.
    

For a presentation, running the binary multiple times and taking the minimum (not the average) is often preferred in systems benchmarking — the minimum represents the closest approximation to "no interference" because interference can only add time, not remove it. The `perf stat -r N` approach gives you the mean ± standard deviation, which is suitable for characterizing typical behavior.

---

## 26. Interpreting Results — What the Numbers Mean

### IPC as the primary diagnostic

|IPC Range|Interpretation|
|---|---|
|> 2.0|Excellent — SIMD or very cache-friendly sequential code|
|1.5 – 2.0|Good — mostly in cache, some stalls|
|1.0 – 1.5|Moderate — occasional cache misses|
|0.5 – 1.0|Poor — significant memory stalls (BST, linked list territory)|
|< 0.5|Very poor — memory-bound, pipeline almost always stalled|

### Cache misses as the explanation

Cache misses directly explain low IPC. Each L3 miss costs ~200 cycles during which the CPU cannot retire instructions. If you have 455,010 cache misses (as in the BST run) and each costs 200 cycles, that accounts for `455,010 × 200 = 91,000,000` stall cycles — which is in the right ballpark for a total of 129,000,000 cycles.

### The efficiency ratio

A useful derived metric is **cycles per element**:

```
cycles_per_element = total_cycles / N
```

For the BST with 129,000,000 cycles and 100,000 nodes: `129,000,000 / 100,000 = 1,290 cycles per node`. For an array with (expected) ~2,000,000 cycles: `2,000,000 / 100,000 = 20 cycles per element`. The BST costs 64× more cycles per element than the array — this is the cost of pointer chasing made quantitative.

---

## 27. Academic Papers and Further Reading

### On measurement methodology

**Mytkowicz, T., Diwan, A., Hauswirth, M., & Sweeney, P. F. (2009)** _Producing Wrong Data Without Doing Anything Obviously Wrong_ ASPLOS 2009. Shows that environmental factors — the length of environment variable strings, the alignment of the stack — can change measured performance by up to 300% without any code changes. The core argument: measuring an entire process is measuring your measurement setup as much as your code. Directly motivates using `perf_event_open` to isolate regions.

**Georges, A., Buytaert, D., & Eeckhout, L. (2007)** _Statistically Rigorous Java Performance Evaluation_ OOPSLA 2007. Establishes statistical frameworks for when a benchmark result is meaningful — warmup iterations, confidence intervals, sample sizes. Essential reading for anyone claiming "X is 2× faster than Y."

**Weaver, V. M., & McKee, S. A. (2008)** _Can Hardware Performance Counters Be Trusted?_ ISPASS 2008. Documents non-determinism in PMU counters: interrupts can cause counters to be off by thousands of counts, multiplexing introduces sampling error when more events are requested than counter registers exist. Explains why you see ±1–2% variance even over 1000 runs.

### On cache-oblivious data structures

**Frigo, M., Leiserson, C. E., Prokop, H., & Ramachandran, S. (1999)** _Cache-Oblivious Algorithms_ FOCS 1999. Introduces algorithms that are optimal for any cache hierarchy without knowing cache sizes at compile time. Van Emde Boas tree layout for BSTs is the canonical example — it achieves O(log_B N) cache misses where B is the cache line size, compared to O(log N) for a standard BST.

**Bender, M. A., Demaine, E. D., & Farach-Colton, M. (2000)** _Cache-Oblivious B-Trees_ FOCS 2000. Applies cache-oblivious techniques specifically to search trees, directly relevant to why BST performs worse than array in your benchmark.

### On hardware performance counter internals

**Intel Corporation** _Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3B, Chapter 19_ Available at intel.com. The authoritative reference for Intel PMU events, counter registers, and the encoding of hardware events. The `PERF_COUNT_HW_CPU_CYCLES` constant in Linux maps to the `UnhaltedCoreCycles` event documented here.

**AMD Corporation** _BIOS and Kernel Developer's Guide (BKDG) for AMD Family 17h_ The AMD equivalent of the Intel SDM for Zen architecture CPUs.

### On data structure performance in practice

**Stroustrup, B. (2012)** _Why You Should Avoid Linked Lists_ GoingNative 2012, Microsoft. A talk (with slides) where Bjarne Stroustrup — the creator of C++ — demonstrates that `std::vector` outperforms `std::list` even for insertion in the middle for sizes up to 500,000 elements, because of cache effects. Precisely the phenomenon our benchmark quantifies.

**Drepper, U. (2007)** _What Every Programmer Should Know About Memory_ Red Hat. Available freely online. A 114-page technical document covering the entire memory hierarchy — DRAM internals, cache organization, TLB, NUMA — in exhaustive detail. The most comprehensive free resource on this topic.

---

_This document covers every variable, every syscall argument, every compiler flag, and every shell construct used in the benchmark suite. The central theme is that performance is not primarily a property of algorithms in the abstract — it is a property of how algorithms interact with the memory hierarchy of real hardware. Clock cycles are the language that hardware speaks, and `perf_event_open` is the most precise way to listen._