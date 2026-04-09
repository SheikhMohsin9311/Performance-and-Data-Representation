#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t key;
    int val;
    int occupied;
} Entry;

static inline uint32_t det(uint32_t i) { return i * 2654435761u; }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    // Linear probing hash table
    int table_size = 1;
    while (table_size < N * 2) table_size <<= 1;
    
    Entry* table = (Entry*)calloc(table_size, sizeof(Entry));
    if (!table) return 1;

    uint32_t mask = table_size - 1;
    for (int i = 0; i < N; i++) {
        uint32_t k = det(i);
        uint32_t h = k & mask;
        while (table[h].occupied && table[h].key != k) {
            h = (h + 1) & mask;
        }
        table[h].key = k;
        table[h].val = i;
        table[h].occupied = 1;
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < table_size; i++) {
            if (table[i].occupied) sink += table[i].val;
        }
    }

    printf("%lld\n", sink);
    free(table);
    return 0;
}