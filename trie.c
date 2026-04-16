/* trie.c — Binary Trie Traversal
 *
 * A 32-level binary trie (one bit per level) stores N 32-bit integers.
 * Each insertion consumes exactly 32 edges in the worst case, giving a
 * maximum depth of 32. The actual number of nodes is at most N*32 but
 * shared prefixes reduce this significantly.
 *
 * Traversal uses DFS inorder, visiting every leaf node exactly once.
 * This guarantees that exactly N values are summed during the measurement
 * window and node count scales predictably with N.
 *
 * Memory access pattern: highly disjoint (pointer-chasing at each level),
 * which is the correct pathological case for trie benchmarking.
 */
#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

static inline unsigned int det_u(int i) { return (unsigned)i * 2654435761u; }

#define BITS 32

struct TrieNode {
    struct TrieNode* child[2];
    int val;       /* stored at leaves only; -1 means internal node */
};

static struct TrieNode* new_node(void) {
    struct TrieNode* n = (struct TrieNode*)calloc(1, sizeof(struct TrieNode));
    n->val = -1;
    return n;
}

static void trie_insert(struct TrieNode* root, unsigned int key, int val) {
    struct TrieNode* cur = root;
    for (int b = BITS - 1; b >= 0; b--) {
        int bit = (key >> b) & 1;
        if (!cur->child[bit]) cur->child[bit] = new_node();
        cur = cur->child[bit];
    }
    cur->val = val;
}

static void traverse_trie(struct TrieNode* node, volatile long long* sink) {
    if (!node) return;
    if (node->val != -1) {        /* leaf */
        *sink += node->val;
        return;
    }
    traverse_trie(node->child[0], sink);
    traverse_trie(node->child[1], sink);
}

static void free_trie(struct TrieNode* node) {
    if (!node) return;
    free_trie(node->child[0]);
    free_trie(node->child[1]);
    free(node);
}

int main(int argc, char* argv[]) {
    int N    = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    /* Insert N distinct keys (deterministic hash spread across 32-bit space) */
    struct TrieNode* root = new_node();
    for (int i = 0; i < N; i++) {
        unsigned int key = det_u(i);
        trie_insert(root, key, (int)key);
    }

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

    /* 3 warmup traversals to prime branch predictor */
    volatile long long sink = 0;
    for (int _wu = 0; _wu < 3; _wu++) {
        sink = 0;
        traverse_trie(root, &sink);
    }
    __asm__ __volatile__("" ::: "memory");

    /* Per-run sampling */
    perf_sample_t* _samples = (perf_sample_t*)malloc(runs * sizeof(perf_sample_t));
    perf_stats_t   _stats;
    for (int _r = 0; _r < runs; _r++) {
        start_counters(fds, 7);
            sink = 0;
            traverse_trie(root, &sink);
        stop_counters(fds, 7, &_samples[_r]);
    }

    compute_perf_stats(_samples, runs, &_stats);
    double _ipc = (_stats.median_cycles > 0)
        ? (double)_stats.median_instructions / _stats.median_cycles : 0.0;
    PRINT_METRICS3(N, _stats, _ipc);

    if (sink == 0xDEADBEEF) printf("Impossible\n");

    free_trie(root);
    free(_samples);
    for (int i = 0; i < 7; i++) if (fds[i] != -1) close(fds[i]);
    return 0;
}
