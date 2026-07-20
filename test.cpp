#include <bits/stdc++.h>
using namespace std;

// Test hash collision

struct Entry {
    int64_t pos;
    uint32_t key_off;
    uint8_t key_len;
};

const int HASH_SIZE = 32768;

static inline uint32_t hashKey(const char* key, int len) {
    uint32_t h = 5381;
    for (int i = 0; i < len; i++) {
        h = ((h << 5) + h) + key[i];
    }
    return h & (HASH_SIZE - 1);
}

int main() {
    unordered_map<uint32_t, int> counts;
    
    for (int i = 0; i < 100000; i++) {
        string k = "key" + to_string(i);
        uint32_t h = hashKey(k.c_str(), k.length());
        counts[h]++;
    }
    
    int maxColl = 0;
    for (auto& [h, c] : counts) {
        if (c > maxColl) maxColl = c;
    }
    
    cout << "Max collisions: " << maxColl << endl;
    cout << "Hash size: " << HASH_SIZE << endl;
    cout << "Total keys: " << counts.size() << endl;
    
    return 0;
}
