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
    // 3 warmup passes to prime cache and branch predictor
    for (int _wu = 0; _wu < 3; _wu++) {
    for (int i = 0; i < N; i++) sink += arr[i];
    } /* end warmup */
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    /* Per-iteration sample collection */
    perf_sample_t* _samples = (perf_sample_t*)malloc(runs * sizeof(perf_sample_t));
    perf_stats_t   _stats;
    for (int _r = 0; _r < runs; _r++) {
        start_counters(fds, 7);
            sink = 0;
            for (int i = 0; i < N; i++) sink += arr[i];
        stop_counters(fds, 7, &_samples[_r]);
    } /* end per-iteration measurement */
    

    compute_perf_stats(_samples, runs, &_stats);
    double _ipc = (_stats.median_cycles > 0)
        ? (double)_stats.median_instructions / _stats.median_cycles : 0.0;
    printf("METRICS2,%lu,%lu,%.2f,%lu,%lu,%lu,%lu,%lu,%.1f\n",
           _stats.median_cycles, _stats.median_instructions, _ipc,
           _stats.median_cache_misses, _stats.median_branches,
           _stats.median_branch_misses, _stats.median_l1_misses,
           _stats.median_tlb_misses, _stats.cv_pct);
    
    if (sink == 0xDEADBEEF) printf("Impossible\n");

    free(arr);
    free(_samples);
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}