#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <vector>

int main() {
    const int N = 100000;
    std::vector<int> v;
    v.reserve(N);

    srand(42);
    for (int i = 0; i < N; i++) v.push_back(rand());

    // Warm up
    volatile long long sum = 0;
    for (int x : v) sum += x;

    // Measured run
    sum = 0;
    for (int x : v) sum += x;

    return 0;
}
