#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>

// Array-based min-heap
void heapifyDown(int* arr, int n, int i) {
    int smallest = i;
    int l = 2*i+1, r = 2*i+2;
    if (l < n && arr[l] < arr[smallest]) smallest = l;
    if (r < n && arr[r] < arr[smallest]) smallest = r;
    if (smallest != i) {
        std::swap(arr[i], arr[smallest]);
        heapifyDown(arr, n, smallest);
    }
}

int main() {
    const int N = 100000;
    int arr[N];

    srand(42);
    for (int i = 0; i < N; i++) arr[i] = rand();

    // Build heap
    for (int i = N/2 - 1; i >= 0; i--)
        heapifyDown(arr, N, i);

    // Warm up — traverse the underlying array
    volatile long long sum = 0;
    for (int i = 0; i < N; i++) sum += arr[i];

    // Measured run
    sum = 0;
    for (int i = 0; i < N; i++) sum += arr[i];

    return 0;
}
