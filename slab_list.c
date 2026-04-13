#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

#define SLAB_SIZE 1024

struct Slab {
    int data[SLAB_SIZE];
    struct Slab* next;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Slab* head = (struct Slab*)malloc(sizeof(struct Slab));
    struct Slab* curr_slab = head;
    for (int i = 0; i < N; i++) {
        int idx = i % SLAB_SIZE;
        if (i > 0 && idx == 0) {
            curr_slab->next = (struct Slab*)malloc(sizeof(struct Slab));
            curr_slab = curr_slab->next;
        }
        curr_slab->data[idx] = det(i);
    }
    curr_slab->next = NULL;

    
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
    curr_slab = head;
    while (curr_slab) {
        for (int j = 0; j < SLAB_SIZE; j++) sink += curr_slab->data[j];
        curr_slab = curr_slab->next;
    }
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    perf_metrics_t m;
    start_counters(fds, 7);
    for (int r = 0; r < runs; r++) {
        sink = 0;
        curr_slab = head;
        while (curr_slab) {
            for (int j = 0; j < SLAB_SIZE; j++) sink += curr_slab->data[j];
            curr_slab = curr_slab->next;
        }
    }
    stop_counters(fds, 7, &m);

    printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);
    
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}
