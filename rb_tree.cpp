#include <iostream>
#include <set>
#include <string>

using namespace std;

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    set<int> s;
    for (int i = 0; i < N; i++) s.insert(det(i));

    // Warm up
    volatile long long sink = 0;
    for (int x : s) sink += x;

    // Measured
    sink = 0;
    for (int x : s) sink += x;

    cout << sink << "\n";
    return 0;
}