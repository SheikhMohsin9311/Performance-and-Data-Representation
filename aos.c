#include <stdio.h>
#include <stdlib.h>

struct Data {
    int a, b;
    long long c;
    double d;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Data* arr = (struct Data*)malloc(N * sizeof(struct Data));
    if (!arr) return 1;

    for (int i = 0; i < N; i++) {
        arr[i].a = det(i);
        arr[i].b = i;
        arr[i].c = (long long)i * i;
        arr[i].d = (double)i / N;
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += arr[i].a;
    }

    printf("%lld\n", sink);
    free(arr);
    return 0;
}