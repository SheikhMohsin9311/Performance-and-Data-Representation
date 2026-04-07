#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <deque>

int main() {
    const int N = 100000;
    std::deque<int> dq;

    srand(42);
    for (int i = 0; i < N; i++) dq.push_back(rand());

    // Warm up
    volatile long long sum = 0;
    for (int x : dq) sum += x;

    // Measured run
    sum = 0;
    for (int x : dq) sum += x;

    return 0;
}
