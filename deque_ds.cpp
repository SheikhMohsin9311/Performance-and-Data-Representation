#include <iostream>
#include <deque>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    deque<int> dq;
    for (int i = 0; i < N; i++) dq.push_back(i);

    // Warm up
    volatile long long sink = 0;
    for (int x : dq) sink += x;

    // Measured
    sink = 0;
    for (int x : dq) sink += x;

    cout << sink << "\n";
    return 0;
}