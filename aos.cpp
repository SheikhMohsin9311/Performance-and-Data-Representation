#include <iostream>
#include <vector>
#include <string>

using namespace std;

// AoS: each struct is 24 bytes. We only read 'x' during traversal,
// but the CPU pulls the whole struct into cache per cache line.
struct Particle { int x, y, z, vx, vy, vz; };

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    vector<Particle> p(N);
    for (int i = 0; i < N; i++) {
        p[i] = { i, i+1, i+2, i+3, i+4, i+5 };
    }

    // Warm up
    volatile long long sink = 0;
    for (int i = 0; i < N; i++) sink += p[i].x;

    // Measured
    sink = 0;
    for (int i = 0; i < N; i++) sink += p[i].x;

    cout << sink << "\n";
    return 0;
}