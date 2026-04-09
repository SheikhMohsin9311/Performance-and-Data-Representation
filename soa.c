#include <stdio.h>
#include <stdlib.h>

struct SoAData {
    int* a;
    int* b;
    long long* c;
    double* d;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct SoAData data;
    data.a = (int*)malloc(N * sizeof(int));
    data.b = (int*)malloc(N * sizeof(int));
    data.c = (long long*)malloc(N * sizeof(long long));
    data.d = (double*)malloc(N * sizeof(double));

    if (!data.a || !data.b || !data.c || !data.d) return 1;

    for (int i = 0; i < N; i++) {
        data.a[i] = det(i);
        data.b[i] = i;
        data.c[i] = (long long)i * i;
        data.d[i] = (double)i / N;
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += data.a[i];
    }

    printf("%lld\n", sink);
    free(data.a); free(data.b); free(data.c); free(data.d);
    return 0;
}