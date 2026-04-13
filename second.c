#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

struct Node {
    int val;
    struct Node *left, *right;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

void traverse_bst(struct Node* root, volatile long long* sink) {
    if (!root) return;
    traverse_bst(root->left, sink);
    *sink += root->val;
    traverse_bst(root->right, sink);
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Node* root = NULL;
    for (int i = 0; i < N; i++) {
        struct Node* n = (struct Node*)malloc(sizeof(struct Node));
        n->val = det(i);
        n->left = n->right = NULL;
        if (!root) root = n;
        else {
            struct Node* curr = root;
            while (1) {
                if (n->val < curr->val) {
                    if (curr->left) curr = curr->left;
                    else { curr->left = n; break; }
                } else {
                    if (curr->right) curr = curr->right;
                    else { curr->right = n; break; }
                }
            }
        }
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
    traverse_bst(root, &sink);
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    perf_metrics_t m;
    start_counters(fds, 7);
    for (int r = 0; r < runs; r++) {
        sink = 0;
        traverse_bst(root, &sink);
    }
    stop_counters(fds, 7, &m);

    printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);
    
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}