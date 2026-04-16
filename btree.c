#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

#define B 16

struct BNode {
    int keys[B];
    struct BNode* children[B+1];
    int n;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

void traverse_btree(struct BNode* node, volatile long long* sink) {
    if (!node) return;
    for (int i = 0; i < node->n; i++) {
        traverse_btree(node->children[i], sink);
        *sink += node->keys[i];
    }
    traverse_btree(node->children[node->n], sink);
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct BNode* root = (struct BNode*)calloc(1, sizeof(struct BNode));
    root->n = 0;

    for (int i = 0; i < N; i++) {
        struct BNode* curr = root;
        int key = det(i);
        // Better branching to ensure tree depth/width scales with N
        while (curr->n >= B) {
            int branch = (key ^ (key >> 16)) % (B + 1);
            if (!curr->children[branch]) {
                curr->children[branch] = (struct BNode*)calloc(1, sizeof(struct BNode));
            }
            curr = curr->children[branch];
            key = det(key); // Go to next level with "new" key
        }
        curr->keys[curr->n++] = key;
    }

    
    int fds[7];
    fds[0] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    fds[1] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    fds[2] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    fds[3] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
    fds[4] = open_perf_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
    fds[5] = open_perf_counter(PERF_TYPE_HW_CACHE, (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)));
    fds[6] = open_perf_counter(PERF_TYPE_HW_CACHE, (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)));

    __asm__ __volatile__("" ::: "memory");


    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;
    
    // Warm up
    // 3 warmup passes to prime cache and branch predictor
    for (int _wu = 0; _wu < 3; _wu++) {
    traverse_btree(root, &sink);
    } /* end warmup */
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    /* Per-iteration sample collection */
    perf_sample_t* _samples = (perf_sample_t*)malloc(runs * sizeof(perf_sample_t));
    perf_stats_t   _stats;
    for (int _r = 0; _r < runs; _r++) {
        start_counters(fds, 7);
            sink = 0;
            traverse_btree(root, &sink);
        stop_counters(fds, 7, &_samples[_r]);
    } /* end per-iteration measurement */

    compute_perf_stats(_samples, runs, &_stats);
    double _ipc = (_stats.median_cycles > 0)
        ? (double)_stats.median_instructions / _stats.median_cycles : 0.0;
    PRINT_METRICS3(N, _stats, _ipc);
    
    free(_samples);
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}
