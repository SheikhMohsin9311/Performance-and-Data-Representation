#include <stdio.h>
#include <stdlib.h>

struct Slab {
    int* data;
    struct Slab* next;
};

#define SLAB_SIZE 1024

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Slab* head = NULL;
    struct Slab* curr_slab = NULL;

    for (int i = 0; i < N; i++) {
        if (i % SLAB_SIZE == 0) {
            struct Slab* newSlab = (struct Slab*)malloc(sizeof(struct Slab));
            newSlab->data = (int*)malloc(SLAB_SIZE * sizeof(int));
            newSlab->next = NULL;
            if (!head) head = newSlab;
            if (curr_slab) curr_slab->next = newSlab;
            curr_slab = newSlab;
        }
        curr_slab->data[i % SLAB_SIZE] = det(i);
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        struct Slab* s = head;
        int count = 0;
        while (s) {
            int limit = (N - count < SLAB_SIZE) ? (N - count) : SLAB_SIZE;
            for (int j = 0; j < limit; j++) sink += s->data[j];
            count += limit;
            s = s->next;
        }
    }

    printf("%lld\n", sink);
    return 0;
}
