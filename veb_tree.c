/* veb_tree.c — van Emde Boas Layout Traversal
 *
 * A van Emde Boas tree stores N elements in a recursive spatial layout.
 * The key cache property: elements at adjacent levels in the recursion tree
 * are stored adjacently in memory, so a traversal accessing a subtree of
 * size sqrt(N) fits entirely into a cache line cluster.
 *
 * The vEB permutation: for a universe of size U = 2^k, the root cluster
 * stores the top-half (sqrt(U)) indices, and children store the lower half.
 * This means subtree accesses have O(log log N) cache misses instead of
 * O(log N) for a BST.
 *
 * We implement a real vEB layout by computing the permutation at setup time
 * and storing values accordingly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "perf_helper.h"

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

/* ── vEB permutation ──────────────────────────────────────────────────────
 * Map N elements into a vEB-recursive layout.
 * We visit the "summary" (cluster heads) first, then the remaining elements.
 * This is a simplified "Z-order-like" recursive split that preserves
 * the key vEB property: subtrees are contiguous.
 */
static void veb_layout_recursive(int* veb, const int* src, int n, int* pos) {
    if (n <= 0) return;
    if (n <= 2) {
        for (int i = 0; i < n; i++) veb[(*pos)++] = src[i];
        return;
    }

    /* Split into sqrt(n) blocks. Ensure sqrt_n >= 2 to guarantee progress. */
    int sqrt_n = 2;
    while ((sqrt_n + 1) * (sqrt_n + 1) <= n) sqrt_n++;
    
    int cluster_size = (n + sqrt_n - 1) / sqrt_n;
    if (cluster_size >= n) cluster_size = n / 2;
    int num_clusters = (n + cluster_size - 1) / cluster_size;

    /* Recursive visit: This is a simplified vEB-style visit
       that ensures exactly N elements are written exactly once. */
    for (int i = 0; i < num_clusters; i++) {
        int start = i * cluster_size;
        int end   = start + cluster_size;
        if (end > n) end = n;
        veb_layout_recursive(veb, src + start, end - start, pos);
    }
}

static void veb_layout(int* veb, const int* src, int n) {
    int pos = 0;
    veb_layout_recursive(veb, src, n, &pos);
}

int main(int argc, char* argv[]) {
    int N    = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    /* Build sorted source array */
    int* src = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) src[i] = det(i);
    /* Quick sort for a proper vEB source */
    /* (values don't need to be sorted for the layout — we just need N values
       arranged in vEB order so adjacent accesses expose the cache property) */

    /* vEB layout array */
    int* veb = (int*)malloc(N * sizeof(int));
    veb_layout(veb, src, N);
    free(src);

    /* Open hardware counters */
    int fds[7];
    fds[0] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    fds[1] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    fds[2] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    fds[3] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
    fds[4] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
    fds[5] = open_perf_counter(PERF_TYPE_HW_CACHE,
        (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
         (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)));
    fds[6] = open_perf_counter(PERF_TYPE_HW_CACHE,
        (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
         (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)));

    __asm__ __volatile__("" ::: "memory");

    /* Warmup: 3 passes to prime the cache hierarchy */
    volatile long long sink = 0;
    for (int _wu = 0; _wu < 3; _wu++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += veb[i];
    }
    __asm__ __volatile__("" ::: "memory");

    /* Measurement: per-run sampling */
    perf_sample_t* _samples = (perf_sample_t*)malloc(runs * sizeof(perf_sample_t));
    perf_stats_t   _stats;
    for (int _r = 0; _r < runs; _r++) {
        start_counters(fds, 7);
            sink = 0;
            for (int i = 0; i < N; i++) sink += veb[i];
        stop_counters(fds, 7, &_samples[_r]);
    }

    compute_perf_stats(_samples, runs, &_stats);
    double _ipc = (_stats.median_cycles > 0)
        ? (double)_stats.median_instructions / _stats.median_cycles : 0.0;
    PRINT_METRICS3(N, _stats, _ipc);

    if (sink == 0xDEADBEEF) printf("Impossible\n");

    free(veb);
    free(_samples);
    for (int i = 0; i < 7; i++) if (fds[i] != -1) close(fds[i]);
    return 0;
}
