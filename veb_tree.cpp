// Van Emde Boas layout — sorted sequential integers, no rand/sort needed.
#include <iostream>
#include <vector>
#include <string>
using namespace std;

void buildVEB(int* out, int outIdx, int lo, int hi) {
    if (lo > hi) return;
    int mid = (lo + hi) / 2;
    out[outIdx] = mid;  // values are just the sorted index
    buildVEB(out, 2*outIdx + 1, lo, mid - 1);
    buildVEB(out, 2*outIdx + 2, mid + 1, hi);
}

volatile long long sink = 0;
void traverse(int* tree, int idx, int size) {
    if (idx >= size) return;
    traverse(tree, 2*idx + 1, size);
    sink += tree[idx];
    traverse(tree, 2*idx + 2, size);
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    vector<int> tree(N, 0);
    buildVEB(tree.data(), 0, 0, N - 1);

    traverse(tree.data(), 0, N); // warm up
    sink = 0;
    traverse(tree.data(), 0, N); // measured

    cout << sink << "\n";
    return 0;
}
