#include <stdio.h>
#include <stdlib.h>

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;
    int* v = (int*)malloc(N * sizeof(int));
    if (!v) return 1;

    for (int i = 0; i < N; i++) v[i] = det(i);

    __asm__ __volatile__("" ::: "memory");

    volatile long long sink = 0;
    // Warm up
    for (int i = 0; i < N; i++) sink += v[i];

    __asm__ __volatile__("" ::: "memory");
    
    // Measured drowning loop
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += v[i];
    }

    printf("%lld\n", sink);
    free(v);
    return 0;
}