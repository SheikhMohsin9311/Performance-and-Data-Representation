// Slab-allocated linked list — all nodes in one contiguous block.
// No rand needed: sequential values.
#include <iostream>
#include <string>
using namespace std;

struct Node { int data; Node* next; };

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    Node* slab = new Node[N];
    for (int i = 0; i < N; i++) {
        slab[i].data = i;
        slab[i].next = (i + 1 < N) ? &slab[i + 1] : nullptr;
    }
    Node* head = &slab[0];

    volatile long long sink = 0;
    for (Node* c = head; c; c = c->next) sink += c->data; // warm up
    sink = 0;
    for (Node* c = head; c; c = c->next) sink += c->data; // measured

    cout << sink << "\n";
    delete[] slab;
    return 0;
}
