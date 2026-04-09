#include <stdio.h>
#include <stdlib.h>

static inline int det(int i) { return (int)((unsigned)i * 2654435761u); }

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

void heapify(int arr[], int n, int i) {
    int largest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;
    if (l < n && arr[l] > arr[largest]) largest = l;
    if (r < n && arr[r] > arr[largest]) largest = r;
    if (largest != i) {
        swap(&arr[i], &arr[largest]);
        heapify(arr, n, largest);
    }
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 524288;
    int runs = (argc > 2) ? atoi(argv[2]) : 1;
    int* heap = (int*)malloc(N * sizeof(int));
    if (!heap) return 1;

    for (int i = 0; i < N; i++) {
        heap[i] = det(i);
        // Bubble up
        int curr = i;
        while (curr > 0 && heap[(curr - 1) / 2] < heap[curr]) {
            swap(&heap[(curr - 1) / 2], &heap[curr]);
            curr = (curr - 1) / 2;
        }
    }

    __asm__ __volatile__("" ::: "memory");
    volatile long long sink = 0;

    // Warm up
    for (int i = 0; i < N; i++) sink += heap[i];

    __asm__ __volatile__("" ::: "memory");

    // Measured drowning loop
    for (int r = 0; r < runs; r++) {
        sink = 0;
        for (int i = 0; i < N; i++) sink += heap[i];
    }

    printf("%lld\n", sink);
    free(heap);
    return 0;
}