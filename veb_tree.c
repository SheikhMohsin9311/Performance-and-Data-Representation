#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

struct vEB {
    int u;
    int min, max;
    struct vEB* summary;
    struct vEB** cluster;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

static inline int lower_root(int u) {
    int power = (int)log2(u);
    return 1 << (power / 2);
}

static inline int upper_root(int u) {
    int power = (int)log2(u);
    return 1 << (power - power / 2);
}

struct vEB* create_vEB(int u) {
    struct vEB* V = (struct vEB*)malloc(sizeof(struct vEB));
    V->u = u;
    V->min = V->max = -1;
    if (u <= 2) {
        V->summary = NULL;
        V->cluster = NULL;
    } else {
        int r_up = upper_root(u);
        int r_lo = lower_root(u);
        V->summary = create_vEB(r_up);
        V->cluster = (struct vEB**)malloc(r_up * sizeof(struct vEB*));
        for (int i = 0; i < r_up; i++) {
            V->cluster[i] = create_vEB(r_lo);
        }
    }
    return V;
}

void insert(struct vEB* V, int x) {
    if (V->min == -1) {
        V->min = V->max = x;
        return;
    }
    if (x < V->min) {
        int temp = x; x = V->min; V->min = temp;
    }
    if (V->u > 2) {
        int r_lo = lower_root(V->u);
        int high = x / r_lo;
        int low = x % r_lo;
        if (V->cluster[high]->min == -1) {
            insert(V->summary, high);
        }
        insert(V->cluster[high], low);
    }
    if (x > V->max) V->max = x;
}

void traverse(struct vEB* V, volatile long long* sink) {
    if (V->min != -1) {
        *sink += V->min;
        if (V->u > 2) {
            int r_up = upper_root(V->u);
            for (int i = 0; i < r_up; i++) {
                traverse(V->cluster[i], sink);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 16384; 
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    int u = 2;
    while (u < N * 4) u <<= 1;
    
    struct vEB* V = create_vEB(u);
    for (int i = 0; i < N; i++) {
        insert(V, (unsigned)det(i) % u);
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        traverse(V, &sink);
    }

    printf("%lld\n", sink);
    return 0;
}
