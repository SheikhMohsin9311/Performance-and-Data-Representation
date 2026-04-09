#include <stdio.h>
#include <stdlib.h>

struct Node {
    int val;
    struct Node *left, *right;
};

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

struct Node* insert(struct Node* node, int val) {
    if (node == NULL) {
        struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
        temp->val = val;
        temp->left = temp->right = NULL;
        return temp;
    }
    if (val < node->val) node->left = insert(node->left, val);
    else node->right = insert(node->right, val);
    return node;
}

void traverse(struct Node* root, volatile long long* sink) {
    if (root) {
        traverse(root->left, sink);
        *sink += root->val;
        traverse(root->right, sink);
    }
}

void cleanup(struct Node* root) {
    if (root) {
        cleanup(root->left);
        cleanup(root->right);
        free(root);
    }
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;

    struct Node* root = NULL;
    for (int i = 0; i < N; i++) root = insert(root, det(i));

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    for (int r = 0; r < runs; r++) {
        sink = 0;
        traverse(root, &sink);
    }

    printf("%lld\n", sink);
    cleanup(root);
    return 0;
}