// Skip List — deterministic (no rand for values, srand only for level draw).
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace std;

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }
static const int MAX_LEVEL = 16;

struct SLNode { int val; SLNode* next[MAX_LEVEL]; };

SLNode* makeNode(int val) {
    SLNode* n = new SLNode();
    n->val = val; memset(n->next, 0, sizeof(n->next)); return n;
}

// Deterministic level via trailing-zero count of (i+1): valid geometric distribution.
int detLevel(int i) {
    int lvl = 1;
    unsigned v = (unsigned)(i + 1);
    while (lvl < MAX_LEVEL && (v & 1) == 0) { v >>= 1; lvl++; }
    return lvl;
}

struct SkipList {
    SLNode* head; int level;
    SkipList() : level(1) { head = makeNode(0); }

    void insert(int val, int ilvl) {
        SLNode* update[MAX_LEVEL]; SLNode* cur = head;
        for (int i = level - 1; i >= 0; i--) {
            while (cur->next[i] && cur->next[i]->val < val) cur = cur->next[i];
            update[i] = cur;
        }
        if (ilvl > level) {
            for (int i = level; i < ilvl; i++) update[i] = head;
            level = ilvl;
        }
        SLNode* n = makeNode(val);
        for (int i = 0; i < ilvl; i++) { n->next[i] = update[i]->next[i]; update[i]->next[i] = n; }
    }
};

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    SkipList sl;
    for (int i = 0; i < N; i++) sl.insert(det(i), detLevel(i));

    volatile long long sink = 0;
    for (SLNode* c = sl.head->next[0]; c; c = c->next[0]) sink += c->val; // warm up
    sink = 0;
    for (SLNode* c = sl.head->next[0]; c; c = c->next[0]) sink += c->val; // measured

    cout << sink << "\n";
    return 0;
}
