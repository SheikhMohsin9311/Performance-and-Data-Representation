// std::set is implemented as a Red-Black Tree internally
#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <set>

int main() {
    const int N = 100000;
    std::set<int> s;

    srand(42);
    for (int i = 0; i < N; i++) s.insert(rand());

    // Warm up
    volatile long long sum = 0;
    for (int x : s) sum += x;

    // Measured run
    sum = 0;
    for (int x : s) sum += x;

    return 0;
}
