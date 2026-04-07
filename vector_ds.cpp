#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    vector<int> v;
    v.reserve(N);
    for (int i = 0; i < N; i++) v.push_back(i);

    // Warm up
    volatile long long sink = 0;
    for (int x : v) sink += x;

    // Measured
    sink = 0;
    for (int x : v) sink += x;

    cout << sink << "\n";
    return 0;
}