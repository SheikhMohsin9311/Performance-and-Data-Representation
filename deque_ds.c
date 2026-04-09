#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE 512

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;
    
    int num_chunks = (N + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int** deque = (int**)malloc(num_chunks * sizeof(int*));
    if (!deque) return 1;

    for (int i = 0; i < num_chunks; i++) {
        deque[i] = (int*)malloc(CHUNK_SIZE * sizeof(int));
    }

    for (int i = 0; i < N; i++) {
        deque[i / CHUNK_SIZE][i % CHUNK_SIZE] = det(i);
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    // Warm up
    for (int i = 0; i < N; i++) {
        sink += deque[i / CHUNK_SIZE][i % CHUNK_SIZE];
    }

    __asm__ __volatile__("" ::: "memory");
    
    // Measured drowning loop
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) {
            sink += deque[i / CHUNK_SIZE][i % CHUNK_SIZE];
        }
    }

    printf("%lld\n", sink);

    for (int i = 0; i < num_chunks; i++) free(deque[i]);
    free(deque);
    return 0;
}