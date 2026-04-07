#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <unordered_map>

int main() {
    const int N = 100000;
    std::unordered_map<int,int> mp;
    mp.reserve(N * 2);

    srand(42);
    for (int i = 0; i < N; i++) mp[rand()] = i;

    // Warm up
    volatile long long sum = 0;
    for (auto& kv : mp) sum += kv.second;

    // Measured run
    sum = 0;
    for (auto& kv : mp) sum += kv.second;

    return 0;
}
