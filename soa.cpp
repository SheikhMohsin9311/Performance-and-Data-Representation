#include <iostream>
#include <vector>
#include <string>

using namespace std;

// SoA: each field is its own contiguous array.
// Reading only 'x' touches exactly N*4 bytes = pure sequential scan.
struct Particles { vector<int> x, y, z, vx, vy, vz; };

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    Particles p;
    p.x.resize(N); p.y.resize(N); p.z.resize(N);
    p.vx.resize(N); p.vy.resize(N); p.vz.resize(N);
    for (int i = 0; i < N; i++) {
        p.x[i]=i; p.y[i]=i+1; p.z[i]=i+2;
        p.vx[i]=i+3; p.vy[i]=i+4; p.vz[i]=i+5;
    }

    // Warm up
    volatile long long sink = 0;
    for (int i = 0; i < N; i++) sink += p.x[i];

    // Measured
    sink = 0;
    for (int i = 0; i < N; i++) sink += p.x[i];

    cout << sink << "\n";
    return 0;
}