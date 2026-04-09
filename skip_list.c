#include <stdio.h>
#include <stdlib.h>

#define MAX_LEVEL 16

struct Node {
    int val;
    struct Node* next[MAX_LEVEL];
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }
static inline int detLevel(int i) {
    unsigned h = (unsigned)i * 1300000077u;
    int level = 1;
    while (level < MAX_LEVEL && (h & 1)) { h >>= 1; level++; }
    return level;
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Node* head = (struct Node*)malloc(sizeof(struct Node));
    head->val = -1;
    for (int i = 0; i < MAX_LEVEL; i++) head->next[i] = NULL;

    for (int i = 0; i < N; i++) {
        int val = det(i);
        int lvl = detLevel(i);
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->val = val;
        
        struct Node* curr = head;
        for (int l = MAX_LEVEL - 1; l >= 0; l--) {
            while (curr->next[l] && curr->next[l]->val < val) {
                curr = curr->next[l];
            }
            if (l < lvl) {
                newNode->next[l] = curr->next[l];
                curr->next[l] = newNode;
            } else {
                newNode->next[l] = NULL; // not used
            }
        }
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        struct Node* curr = head->next[0];
        while (curr) {
            sink += curr->val;
            curr = curr->next[0];
        }
    }

    printf("%lld\n", sink);
    return 0; // Skip cleanup for speed as per benchmarking tradition
}
