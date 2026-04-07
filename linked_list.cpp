#include <iostream>
#include <string>

using namespace std;

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

struct Node { int data; Node* next; };

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    Node* head = nullptr;
    Node* tail = nullptr;

    for (int i = 0; i < N; i++) {
        Node* n = new Node();
        n->data = det(i); n->next = nullptr;
        if (!head) head = tail = n;
        else { tail->next = n; tail = n; }
    }

    // Warm up
    volatile long long sink = 0;
    for (Node* c = head; c; c = c->next) sink += c->data;

    // Measured
    sink = 0;
    for (Node* c = head; c; c = c->next) sink += c->data;

    cout << sink << "\n";
    for (Node* c = head; c;) { Node* t = c->next; delete c; c = t; }
    return 0;
}