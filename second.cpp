#include <iostream>
#include <cstdlib>
#include <string>

using namespace std;

// Deterministic value: Knuth multiplicative hash, no PRNG state.
static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

struct Node {
    int data;
    Node* left;
    Node* right;
};

Node* newNode(int val) {
    Node* n = new Node();
    n->data = val; n->left = nullptr; n->right = nullptr;
    return n;
}

Node* insert(Node* root, int val) {
    if (!root) return newNode(val);
    if (val < root->data) root->left  = insert(root->left,  val);
    else                  root->right = insert(root->right, val);
    return root;
}

volatile long long sink = 0;
void inorder(Node* root) {
    if (!root) return;
    inorder(root->left);
    sink += root->data;
    inorder(root->right);
}

void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->left); freeTree(root->right); delete root;
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    Node* root = nullptr;
    for (int i = 0; i < N; i++) root = insert(root, det(i));

    inorder(root);  // warm up
    sink = 0;
    inorder(root);  // measured

    cout << sink << "\n";
    freeTree(root);
    return 0;
}