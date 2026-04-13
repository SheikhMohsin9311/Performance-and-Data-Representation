#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int compare_ints(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    int* arr = (int*)malloc(N * sizeof(int));
    if (!arr) return 1;

    for (int i = 0; i < N; i++) arr[i] = det(i);

    // Sorting: This is part of SETUP, should not be measured
    qsort(arr, N, sizeof(int), compare_ints);

    
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
    for (int i = 0; i < N; i++) sink += arr[i];
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    perf_metrics_t m;
    start_counters(fds, 7);
    
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += arr[i];
    }
    
    stop_counters(fds, 7, &m);

    printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);
    
    if (sink == 0xDEADBEEF) printf("Impossible\n");

    free(arr);
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}