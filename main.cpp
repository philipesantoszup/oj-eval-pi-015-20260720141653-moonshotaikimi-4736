#include <bits/stdc++.h>
using namespace std;

// Memory-optimized file-based key-value database
// Uses small in-memory cache + append-only file storage

const char* META_FILE = "meta.dat";
const char* DATA_FILE = "data.dat";

// Simple hash function for strings
static unsigned int hashStr(const char* s, int len) {
    unsigned int h = 5381;
    for (int i = 0; i < len; i++) {
        h = ((h << 5) + h) + s[i];
    }
    return h;
}

// Cache entry structure - kept minimal
struct CacheEntry {
    long long pos;   // -1 = not in cache, -2 = deleted
    int valCount;
    int values[128]; // Fixed size, max values per key
};

// Simple static cache: hash table with linear probing
const int CACHE_SIZE = 128;  // Small cache to stay under memory limit
struct {
    char key[65];
    int keyLen;
    CacheEntry data;
    bool used;
} cache[CACHE_SIZE];

static void initCache() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i].used = false;
    }
}

static int findInCache(const char* key, int keyLen) {
    unsigned int h = hashStr(key, keyLen) % CACHE_SIZE;
    for (int i = 0; i < CACHE_SIZE; i++) {
        int idx = (h + i) % CACHE_SIZE;
        if (!cache[idx].used) return -1;
        if (cache[idx].keyLen == keyLen && memcmp(cache[idx].key, key, keyLen) == 0) {
            return idx;
        }
    }
    return -1;
}

static void putInCache(const char* key, int keyLen, const CacheEntry& entry) {
    unsigned int h = hashStr(key, keyLen) % CACHE_SIZE;
    for (int i = 0; i < CACHE_SIZE; i++) {
        int idx = (h + i) % CACHE_SIZE;
        if (!cache[idx].used) {
            cache[idx].used = true;
            memcpy(cache[idx].key, key, keyLen);
            cache[idx].key[keyLen] = '\0';
            cache[idx].keyLen = keyLen;
            cache[idx].data = entry;
            return;
        }
        if (cache[idx].keyLen == keyLen && memcmp(cache[idx].key, key, keyLen) == 0) {
            cache[idx].data = entry;
            return;
        }
    }
    // Cache full, evict first entry in probe sequence
    int idx = h;
    memcpy(cache[idx].key, key, keyLen);
    cache[idx].key[keyLen] = '\0';
    cache[idx].keyLen = keyLen;
    cache[idx].data = entry;
}

// Find the most recent entry for a key in meta file
static long long findKeyPos(const char* key, int keyLen) {
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return -1;
    
    long long result = -1;
    char buf[65];
    int len;
    long long pos;
    
    while (fread(&len, sizeof(int), 1, fp) == 1) {
        if ((int)fread(buf, 1, len, fp) != len) break;
        if (fread(&pos, sizeof(long long), 1, fp) != 1) break;
        
        if (len == keyLen && memcmp(buf, key, keyLen) == 0) {
            result = pos;
        }
    }
    
    fclose(fp);
    return result;
}

// Load values from data file
static int loadValues(long long pos, int* values) {
    if (pos < 0) return 0;
    
    FILE* fp = fopen(DATA_FILE, "rb");
    if (!fp) return 0;
    
    fseek(fp, pos, SEEK_SET);
    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (count < 0 || count > 128) count = 0;
    
    for (int i = 0; i < count; i++) {
        if (fread(&values[i], sizeof(int), 1, fp) != 1) break;
    }
    fclose(fp);
    return count;
}

// Save values and return position
static long long saveValues(const int* values, int count) {
    FILE* fp = fopen(DATA_FILE, "ab");
    if (!fp) return -1;
    
    long long pos = ftell(fp);
    fwrite(&count, sizeof(int), 1, fp);
    fwrite(values, sizeof(int), count, fp);
    fclose(fp);
    return pos;
}

// Append entry to meta file
static void appendMeta(const char* key, int keyLen, long long pos) {
    FILE* fp = fopen(META_FILE, "ab");
    if (!fp) return;
    
    fwrite(&keyLen, sizeof(int), 1, fp);
    fwrite(key, 1, keyLen, fp);
    fwrite(&pos, sizeof(long long), 1, fp);
    fclose(fp);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    initCache();
    
    int n;
    cin >> n;
    
    char cmd[10];
    char index[65];
    int value;
    int values[128];
    
    for (int i = 0; i < n; ++i) {
        cin >> cmd >> index;
        int idxLen = strlen(index);
        
        // Check cache first
        int cacheIdx = findInCache(index, idxLen);
        long long pos = -1;
        int valCount = 0;
        
        if (cacheIdx >= 0) {
            pos = cache[cacheIdx].data.pos;
            valCount = cache[cacheIdx].data.valCount;
            for (int k = 0; k < valCount; k++) {
                values[k] = cache[cacheIdx].data.values[k];
            }
        }
        
        if (cmd[0] == 'i') {  // insert
            cin >> value;
            
            if (cacheIdx < 0) {
                pos = findKeyPos(index, idxLen);
                if (pos >= 0) {
                    valCount = loadValues(pos, values);
                }
            }
            
            if (pos == -1) {
                // New entry
                values[0] = value;
                valCount = 1;
                long long newPos = saveValues(values, 1);
                appendMeta(index, idxLen, newPos);
                
                CacheEntry ce;
                ce.pos = newPos;
                ce.valCount = 1;
                ce.values[0] = value;
                putInCache(index, idxLen, ce);
            } else {
                // Binary search insertion point
                int left = 0, right = valCount;
                while (left < right) {
                    int mid = (left + right) >> 1;
                    if (values[mid] < value) left = mid + 1;
                    else right = mid;
                }
                
                // Check if already exists
                if (left == valCount || values[left] != value) {
                    // Shift and insert
                    for (int j = valCount; j > left; j--) {
                        values[j] = values[j-1];
                    }
                    values[left] = value;
                    valCount++;
                    long long newPos = saveValues(values, valCount);
                    appendMeta(index, idxLen, newPos);
                    
                    CacheEntry ce;
                    ce.pos = newPos;
                    ce.valCount = valCount;
                    for (int k = 0; k < valCount; k++) ce.values[k] = values[k];
                    putInCache(index, idxLen, ce);
                }
            }
        }
        else if (cmd[0] == 'd') {  // delete
            cin >> value;
            
            if (cacheIdx < 0) {
                pos = findKeyPos(index, idxLen);
                if (pos >= 0) {
                    valCount = loadValues(pos, values);
                }
            }
            
            if (pos >= 0) {
                // Binary search
                int left = 0, right = valCount;
                while (left < right) {
                    int mid = (left + right) >> 1;
                    if (values[mid] < value) left = mid + 1;
                    else right = mid;
                }
                
                if (left < valCount && values[left] == value) {
                    // Remove
                    for (int j = left; j < valCount - 1; j++) {
                        values[j] = values[j+1];
                    }
                    valCount--;
                    
                    if (valCount == 0) {
                        appendMeta(index, idxLen, -1);
                        CacheEntry ce;
                        ce.pos = -1;
                        ce.valCount = 0;
                        putInCache(index, idxLen, ce);
                    } else {
                        long long newPos = saveValues(values, valCount);
                        appendMeta(index, idxLen, newPos);
                        
                        CacheEntry ce;
                        ce.pos = newPos;
                        ce.valCount = valCount;
                        for (int k = 0; k < valCount; k++) ce.values[k] = values[k];
                        putInCache(index, idxLen, ce);
                    }
                }
            }
        }
        else {  // find
            if (cacheIdx < 0) {
                pos = findKeyPos(index, idxLen);
                if (pos >= 0) {
                    valCount = loadValues(pos, values);
                    CacheEntry ce;
                    ce.pos = pos;
                    ce.valCount = valCount;
                    for (int k = 0; k < valCount; k++) ce.values[k] = values[k];
                    putInCache(index, idxLen, ce);
                }
            }
            
            if (pos < 0) {
                cout << "null\n";
            } else {
                if (valCount == 0) {
                    cout << "null\n";
                } else {
                    for (int j = 0; j < valCount; j++) {
                        if (j > 0) cout << ' ';
                        cout << values[j];
                    }
                    cout << '\n';
                }
            }
        }
    }
    
    return 0;
}
