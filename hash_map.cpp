#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    unordered_map<int,int> mp;
    mp.reserve(N * 2);
    for (int i = 0; i < N; i++) mp[det(i)] = i;

    // Warm up
    volatile long long sink = 0;
    for (auto& kv : mp) sink += kv.second;

    // Measured
    sink = 0;
    for (auto& kv : mp) sink += kv.second;

    cout << sink << "\n";
    return 0;
}