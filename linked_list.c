#include <stdio.h>
#include <stdlib.h>

struct Node {
    int val;
    struct Node* next;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Node* head = NULL;
    struct Node* curr = NULL;

    for (int i = 0; i < N; i++) {
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->val = det(i);
        newNode->next = NULL;
        if (!head) {
            head = newNode;
            curr = head;
        } else {
            curr->next = newNode;
            curr = newNode;
        }
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        curr = head;
        while (curr) {
            sink += curr->val;
            curr = curr->next;
        }
    }

    printf("%lld\n", sink);
    
    // Cleanup
    curr = head;
    while (curr) {
        struct Node* temp = curr;
        curr = curr->next;
        free(temp);
    }

    return 0;
}