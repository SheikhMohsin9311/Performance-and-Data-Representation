#include <iostream>
#include <algorithm>
#include <string>

using namespace std;

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

void heapifyDown(int* arr, int n, int i) {
    int s = i, l = 2*i+1, r = 2*i+2;
    if (l < n && arr[l] < arr[s]) s = l;
    if (r < n && arr[r] < arr[s]) s = r;
    if (s != i) { swap(arr[i], arr[s]); heapifyDown(arr, n, s); }
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    int* arr = new int[N];
    for (int i = 0; i < N; i++) arr[i] = det(i);
    for (int i = N/2 - 1; i >= 0; i--) heapifyDown(arr, N, i);

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