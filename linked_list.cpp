#include "perfutil.h"
#include <iostream>
#include <cstdlib>

struct Node {
    int data;
    Node* next;
};

int main() {
    const int N = 100000;
    Node* head = nullptr;
    Node* tail = nullptr;

    srand(42);
    for (int i = 0; i < N; i++) {
        Node* n = new Node();
        n->data = rand();
        n->next = nullptr;
        if (!head) head = tail = n;
        else { tail->next = n; tail = n; }
    }

    // Warm up
    volatile long long sum = 0;
    Node* cur = head;
    while (cur) { sum += cur->data; cur = cur->next; }

    // Measured run
    sum = 0;
    cur = head;
    while (cur) { sum += cur->data; cur = cur->next; }

    // Free
    cur = head;
    while (cur) { Node* tmp = cur->next; delete cur; cur = tmp; }

    return 0;
}
