#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    int runs = (argc > 2) ? stoi(argv[2]) : 1000; // ADD THIS

    int* arr = new int[N];
    for (int i = 0; i < N; i++) arr[i] = i;

    // Warm up
    volatile long long sink = 0;
    for (int i = 0; i < N; i++) sink += arr[i];

    // THE DROWNING LOOP
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += arr[i];
    }

    delete[] arr;
    return 0;
}