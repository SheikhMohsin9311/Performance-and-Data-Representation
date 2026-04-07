// Circular buffer — sequential fill, modulo access pattern. No rand.
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    int* buf = new int[N];
    for (int i = 0; i < N; i++) buf[i] = i;

    volatile long long sink = 0;
    for (int i = 0; i < 2 * N; i++) sink += buf[i % N]; // warm up
    sink = 0;
    for (int i = 0; i < 2 * N; i++) sink += buf[i % N]; // measured

    cout << sink << "\n";
    delete[] buf;
    return 0;
}
