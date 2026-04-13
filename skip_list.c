#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

#define MAX_LEVEL 8

struct SkipNode {
    int val;
    struct SkipNode* next[MAX_LEVEL];
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct SkipNode* head = (struct SkipNode*)malloc(sizeof(struct SkipNode));
    for (int i = 0; i < MAX_LEVEL; i++) head->next[i] = NULL;

    for (int i = 0; i < N; i++) {
        struct SkipNode* n = (struct SkipNode*)malloc(sizeof(struct SkipNode));
        n->val = det(i);
        int node_level = 1;
        while (node_level < MAX_LEVEL && (rand() % 100) < 50) node_level++;
        
        for (int l = 0; l < node_level; l++) {
            n->next[l] = head->next[l];
            head->next[l] = n;
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
    struct SkipNode* curr = head->next[0];
    while (curr) { sink += curr->val; curr = curr->next[0]; }
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    perf_metrics_t m;
    start_counters(fds, 7);
    for (int r = 0; r < runs; r++) {
        sink = 0;
        curr = head->next[0];
        while (curr) { sink += curr->val; curr = curr->next[0]; }
    }
    stop_counters(fds, 7, &m);

    printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);
    
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}
