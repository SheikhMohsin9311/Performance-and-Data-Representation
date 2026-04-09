#include <stdio.h>
#include <stdlib.h>

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    int* arr = (int*)malloc(N * sizeof(int));
    if (!arr) return 1;

    for (int i = 0; i < N; i++) arr[i] = det(i);

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += arr[i];
    }

    printf("%lld\n", sink);
    free(arr);
    return 0;
}