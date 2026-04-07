// B-Tree order 16 — deterministic, no rand.
#include <iostream>
#include <cstring>
#include <string>
using namespace std;

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }
static const int ORDER = 16;
static const int MAX_KEYS = ORDER - 1;

struct BNode {
    int keys[MAX_KEYS]; BNode* children[ORDER]; int n; bool leaf;
    BNode(bool isLeaf) : n(0), leaf(isLeaf) { memset(children, 0, sizeof(children)); }
};

struct BTree {
    BNode* root;
    BTree() { root = new BNode(true); }

    void splitChild(BNode* parent, int idx) {
        BNode* full = parent->children[idx];
        BNode* sib = new BNode(full->leaf);
        int mid = MAX_KEYS / 2;
        sib->n = MAX_KEYS - mid - 1;
        for (int i = 0; i < sib->n; i++) sib->keys[i] = full->keys[mid + 1 + i];
        if (!full->leaf) for (int i = 0; i <= sib->n; i++) sib->children[i] = full->children[mid + 1 + i];
        full->n = mid;
        for (int i = parent->n; i > idx; i--) parent->children[i+1] = parent->children[i];
        parent->children[idx+1] = sib;
        for (int i = parent->n - 1; i >= idx; i--) parent->keys[i+1] = parent->keys[i];
        parent->keys[idx] = full->keys[mid];
        parent->n++;
    }

    void insertNonFull(BNode* node, int k) {
        int i = node->n - 1;
        if (node->leaf) {
            while (i >= 0 && node->keys[i] > k) { node->keys[i+1] = node->keys[i]; i--; }
            node->keys[i+1] = k; node->n++;
        } else {
            while (i >= 0 && node->keys[i] > k) i--;
            i++;
            if (node->children[i]->n == MAX_KEYS) { splitChild(node, i); if (node->keys[i] < k) i++; }
            insertNonFull(node->children[i], k);
        }
    }

    void insert(int k) {
        if (root->n == MAX_KEYS) {
            BNode* r = new BNode(false);
            r->children[0] = root; splitChild(r, 0); root = r;
        }
        insertNonFull(root, k);
    }
};

volatile long long sink = 0;
void inorder(BNode* node) {
    if (!node) return;
    for (int i = 0; i < node->n; i++) {
        if (!node->leaf) inorder(node->children[i]);
        sink += node->keys[i];
    }
    if (!node->leaf) inorder(node->children[node->n]);
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    BTree bt;
    for (int i = 0; i < N; i++) bt.insert(det(i));

    inorder(bt.root); // warm up
    sink = 0;
    inorder(bt.root); // measured

    cout << sink << "\n";
    return 0;
}
