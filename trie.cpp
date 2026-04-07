// Trie over deterministic 8-char strings — no rand.
// String for index i: each char = 'a' + ((i * prime + j * prime2) % 26).
// Gives good character variety without PRNG.
#include <iostream>
#include <cstring>
#include <string>
using namespace std;

struct TrieNode {
    TrieNode* ch[26]; bool end;
    TrieNode() : end(false) { memset(ch, 0, sizeof(ch)); }
};

void insert(TrieNode* root, const char* s, int len) {
    TrieNode* cur = root;
    for (int i = 0; i < len; i++) {
        int idx = s[i] - 'a';
        if (!cur->ch[idx]) cur->ch[idx] = new TrieNode();
        cur = cur->ch[idx];
    }
    cur->end = true;
}

volatile long long sink = 0;
void dfs(TrieNode* node) {
    if (!node) return;
    sink++;
    for (int i = 0; i < 26; i++) dfs(node->ch[i]);
}

int main(int argc, char* argv[]) {
    int N = (argc > 1) ? stoi(argv[1]) : 524288;
    TrieNode* root = new TrieNode();

    char buf[9]; buf[8] = '\0';
    for (int i = 0; i < N; i++) {
        // Deterministic string: multiply-hash each character position
        for (int j = 0; j < 8; j++)
            buf[j] = 'a' + (int)(((unsigned)(i * 1664525u + j * 22695477u)) % 26u);
        insert(root, buf, 8);
    }

    dfs(root); // warm up
    sink = 0;
    dfs(root); // measured

    cout << sink << "\n";
    return 0;
}
