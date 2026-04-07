#include "perfutil.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

int main() {
    const int N = 100000;
    int arr[N];

    srand(42);
    for (int i = 0; i < N; i++)
        arr[i] = rand();

    std::sort(arr, arr + N);

    // Warm up
    volatile long long sum = 0;
    for (int i = 0; i < N; i++)
        sum += arr[i];

    // Measured run
    sum = 0;
    for (int i = 0; i < N; i++)
        sum += arr[i];

    return 0;
}