#include <stdio.h>
#include <stdlib.h>

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;
    int* buf = (int*)malloc(N * sizeof(int));
    if (!buf) return 1;

    for (int i = 0; i < N; i++) buf[i] = det(i);

    __asm__ __volatile__("" ::: "memory");

    volatile long long sink = 0;
    // Warm up
    for (int i = 0; i < 2 * N; i++) sink += buf[i % N];

    __asm__ __volatile__("" ::: "memory");
    
    // Measured drowning loop
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < 2 * N; i++) sink += buf[i % N];
    }

    printf("%lld\n", sink);
    free(buf);
    return 0;
}
