#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

struct Node {
    int data;
    Node* left;
    Node* right;
};

Node* newNode(int val) {
    Node* n = new Node();
    n->data = val;
    n->left = nullptr;
    n->right = nullptr;
    return n;
}

Node* insert(Node* root, int val) {
    if (!root) return newNode(val);
    if (val < root->data)
        root->left = insert(root->left, val);
    else
        root->right = insert(root->right, val);
    return root;
}

// Inorder traversal — touches every node once
void inorder(Node* root) {
    if (!root) return;
    inorder(root->left);
    // volatile prevents the compiler from optimizing this away
    volatile int x = root->data;
    inorder(root->right);
}

void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

int main() {
    const int N = 100000;
    Node* root = nullptr;

    srand(42);
    for (int i = 0; i < N; i++)
        root = insert(root, rand());

    // Warm up — first traversal brings tree into cache
    inorder(root);

    // This is the run perf will capture
    inorder(root);

    freeTree(root);
    return 0;
}