#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

struct Entry {
    int key;
    int val;
    struct Entry* next;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;
    
    int table_size = N;
    struct Entry** table = (struct Entry**)calloc(table_size, sizeof(struct Entry*));

    for (int i = 0; i < N; i++) {
        int key = det(i);
        int bucket = abs(key) % table_size;
        struct Entry* e = (struct Entry*)malloc(sizeof(struct Entry));
        e->key = key;
        e->val = i;
        e->next = table[bucket];
        table[bucket] = e;
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
    for (int i = 0; i < table_size; i++) {
    struct Entry* e = table[i];
    while (e) {
    sink += e->val;
    e = e->next;
    }
    }
    } /* end warmup */
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    /* Per-iteration sample collection */
    perf_sample_t* _samples = (perf_sample_t*)malloc(runs * sizeof(perf_sample_t));
    perf_stats_t   _stats;
    for (int _r = 0; _r < runs; _r++) {
        start_counters(fds, 7);
            sink = 0;
            for (int i = 0; i < table_size; i++) {
                struct Entry* e = table[i];
                while (e) {
                    sink += e->val;
                    e = e->next;
                }
            }
        stop_counters(fds, 7, &_samples[_r]);
    } /* end per-iteration measurement */
    

    compute_perf_stats(_samples, runs, &_stats);
    double _ipc = (_stats.median_cycles > 0)
        ? (double)_stats.median_instructions / _stats.median_cycles : 0.0;
    PRINT_METRICS3(N, _stats, _ipc);
    
    if (sink == 0xDEADBEEF) printf("Impossible\n");

    for (int i = 0; i < table_size; i++) {
        struct Entry* e = table[i];
        while (e) {
            struct Entry* tmp = e;
            e = e->next;
            free(tmp);
        }
    }
    free(table);
    free(_samples);
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}