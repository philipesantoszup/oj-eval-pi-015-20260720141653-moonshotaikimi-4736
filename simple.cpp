#include <bits/stdc++.h>
using namespace std;

const char* META_FILE = "meta.dat";

struct CacheEntry {
    int64_t pos;
    uint32_t key_hash;
    uint16_t key_len;
};

const int CACHE_SIZE = 4096;
static CacheEntry cache[CACHE_SIZE];

static inline uint32_t strHash(const char* key, int len) {
    uint32_t h = 5381;
    for (int i = 0; i < len; i++) h = ((h << 5) + h) + key[i];
    return h;
}

static void initCache() {
    for (int i = 0; i < CACHE_SIZE; i++) cache[i].pos = -2;
}

static int64_t findInCache(const char* key, int len) {
    uint32_t h = strHash(key, len);
    uint32_t idx = h & (CACHE_SIZE - 1);
    
    for (int probes = 0; probes < 8; probes++) {
        int i = (idx + probes) & (CACHE_SIZE - 1);
        if (cache[i].pos == -2) break;
        if (cache[i].pos >= -1 && cache[i].key_hash == h && cache[i].key_len == len) {
            return cache[i].pos;
        }
    }
    return -2;  // Not found
}

static void putInCache(const char* key, int len, int64_t pos) {
    uint32_t h = strHash(key, len);
    uint32_t idx = h & (CACHE_SIZE - 1);
    
    for (int probes = 0; probes < 8; probes++) {
        int i = (idx + probes) & (CACHE_SIZE - 1);
        if (cache[i].pos < -1) {
            cache[i].pos = pos;
            cache[i].key_hash = h;
            cache[i].key_len = len;
            return;
        }
    }
    cache[idx].pos = pos;
    cache[idx].key_hash = h;
    cache[idx].key_len = len;
}

static int64_t scanMeta(const char* key, int len) {
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return -1;
    
    int64_t result = -1;
    int keylen;
    char buf[65];
    int64_t pos;
    
    while (fread(&keylen, sizeof(int), 1, fp) == 1) {
        if (keylen > 64 || keylen < 0) break;
        if (keylen > 0 && (int)fread(buf, 1, keylen, fp) != keylen) break;
        if (fread(&pos, sizeof(int64_t), 1, fp) != 1) break;
        
        if (keylen == len && memcmp(buf, key, len) == 0) {
            result = pos;
        }
    }
    
    fclose(fp);
    return result;
}

int main() {
    initCache();
    
    char key[] = "a";
    int len = 1;
    int64_t pos = 999;
    
    // Put in cache
    putInCache(key, len, pos);
    
    // Find in cache
    int64_t found = findInCache(key, len);
    
    cout << "Put pos=" << pos << " found=" << found << endl;
    
    return 0;
}
