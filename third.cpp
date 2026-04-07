#include <iostream>
#include <string>

using namespace std;

// Sequential integers are already sorted — no rand, no sort().
int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    int* arr = new int[N];
    for (int i = 0; i < N; i++) arr[i] = i;  // already sorted

    // Warm up
    volatile long long sink = 0;
    for (int i = 0; i < N; i++) sink += arr[i];

    // Measured
    sink = 0;
    for (int i = 0; i < N; i++) sink += arr[i];

    cout << sink << "\n";
    delete[] arr;
    return 0;
}