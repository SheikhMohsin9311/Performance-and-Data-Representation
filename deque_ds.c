#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

// Simple block-based Deque simulation
#define BLOCK_SIZE 1024

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    int num_blocks = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int** blocks = (int**)malloc(num_blocks * sizeof(int*));
    for (int i = 0; i < num_blocks; i++) {
        blocks[i] = (int*)malloc(BLOCK_SIZE * sizeof(int));
        for (int j = 0; j < BLOCK_SIZE; j++) {
            int idx = i * BLOCK_SIZE + j;
            if (idx < N) blocks[i][j] = det(idx);
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
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (i * BLOCK_SIZE + j < N) sink += blocks[i][j];
        }
    }
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    perf_metrics_t m;
    start_counters(fds, 7);
    
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < num_blocks; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (i * BLOCK_SIZE + j < N) sink += blocks[i][j];
            }
        }
    }
    
    stop_counters(fds, 7, &m);

    printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);
    
    if (sink == 0xDEADBEEF) printf("Impossible\n");

    for (int i = 0; i < num_blocks; i++) free(blocks[i]);
    free(blocks);
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}