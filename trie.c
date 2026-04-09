#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct TrieNode {
    struct TrieNode* children[16];
    int is_end;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

struct TrieNode* create_node() {
    struct TrieNode* node = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    for (int i = 0; i < 16; i++) node->children[i] = NULL;
    node->is_end = 0;
    return node;
}

void insert(struct TrieNode* root, uint32_t val) {
    struct TrieNode* curr = root;
    for (int i = 0; i < 8; i++) {
        int idx = (val >> (i * 4)) & 0xF;
        if (!curr->children[idx]) curr->children[idx] = create_node();
        curr = curr->children[idx];
    }
    curr->is_end = 1;
}

void traverse(struct TrieNode* node, volatile long long* sink) {
    if (!node) return;
    if (node->is_end) *sink += 1;
    for (int i = 0; i < 16; i++) {
        traverse(node->children[i], sink);
    }
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 100000;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct TrieNode* root = create_node();
    for (int i = 0; i < N; i++) {
        insert(root, (uint32_t)det(i));
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        traverse(root, &sink);
    }

    printf("%lld\n", sink);
    return 0;
}
