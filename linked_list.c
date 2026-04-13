#include <stdio.h>
#include <stdlib.h>
#include "perf_helper.h"

struct Node {
    int val;
    struct Node* next;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

void shuffle(struct Node** array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        struct Node* temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    // Allocate all nodes first
    struct Node** nodes = (struct Node**)malloc(N * sizeof(struct Node*));
    for (int i = 0; i < N; i++) {
        nodes[i] = (struct Node*)malloc(sizeof(struct Node));
        nodes[i]->val = det(i);
        nodes[i]->next = NULL;
    }

    // Memory Shake: Shuffle nodes to destroy spatial locality
    shuffle(nodes, N);

    // Link the shuffled nodes
    struct Node* head = nodes[0];
    for (int i = 0; i < N - 1; i++) {
        nodes[i]->next = nodes[i + 1];
    }
    nodes[N - 1]->next = NULL;

    free(nodes); // We don't need the pointer array anymore, just the linked nodes

    // Hardware counter descriptors
    
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
    struct Node* curr = head;
    while (curr) {
        sink += curr->val;
        curr = curr->next;
    }
    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    perf_metrics_t m;
    start_counters(fds, 7);
    
    for (int r = 0; r < runs; r++) {
        sink = 0;
        curr = head;
        while (curr) {
            sink += curr->val;
            curr = curr->next;
        }
    }
    
    stop_counters(fds, 7, &m);

    // Print machine-readable metrics
    printf("METRICS,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", m.cycles, m.instructions, m.cache_misses, m.branches, m.branch_misses, m.l1_misses, m.tlb_misses);
    
    if (sink == 0xDEADBEEF) printf("Impossible\n");

    // Cleanup
    curr = head;
    while (curr) {
        struct Node* temp = curr;
        curr = curr->next;
        free(temp);
    }
    
    for(int i=0; i<7; i++) if(fds[i]!=-1) close(fds[i]);
    return 0;
}