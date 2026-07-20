#include <bits/stdc++.h>
using namespace std;

// Ultra-compact file-based key-value database
// Fixed small hash table with linear probing

const char* META_FILE = "meta.dat";
const char* DATA_FILE = "data.dat";

// Compact hash table entry
struct Entry {
    int64_t pos;      // -1 = empty, >=0 = position
    uint32_t key_off; // offset in key pool
    uint8_t key_len;
};

const int HASH_SIZE = 131072;  // 2^17 = 128K
const int KEY_POOL_SIZE = 2000000;  // ~2 MB for keys

static Entry hash_table[HASH_SIZE];
static char key_pool[KEY_POOL_SIZE];
static uint32_t key_pool_pos = 0;

static inline uint32_t hashKey(const char* key, int len) {
    uint32_t h = 5381;
    for (int i = 0; i < len; i++) {
        h = ((h << 5) + h) + key[i];
    }
    return h & (HASH_SIZE - 1);
}

static int64_t findInIndex(const char* key, int len) {
    uint32_t h = hashKey(key, len);
    uint32_t idx = h;
    
    for (int probes = 0; probes < HASH_SIZE; probes++) {
        int64_t p = hash_table[idx].pos;
        if (p < 0) return -1;  // Empty
        if (hash_table[idx].key_len == len) {
            const char* stored = key_pool + hash_table[idx].key_off;
            if (memcmp(stored, key, len) == 0) {
                return p;
            }
        }
        idx = (idx + 1) & (HASH_SIZE - 1);
    }
    return -1;
}

static void updateIndex(const char* key, int len, int64_t pos) {
    uint32_t h = hashKey(key, len);
    uint32_t idx = h;
    
    for (int probes = 0; probes < HASH_SIZE; probes++) {
        int64_t p = hash_table[idx].pos;
        if (p < 0) {
            // Empty slot
            hash_table[idx].pos = pos;
            hash_table[idx].key_len = len;
            hash_table[idx].key_off = key_pool_pos;
            memcpy(key_pool + key_pool_pos, key, len);
            key_pool_pos += len + 1;
            return;
        }
        if (hash_table[idx].key_len == len) {
            const char* stored = key_pool + hash_table[idx].key_off;
            if (memcmp(stored, key, len) == 0) {
                hash_table[idx].pos = pos;
                return;
            }
        }
        idx = (idx + 1) & (HASH_SIZE - 1);
    }
}

static int loadValues(int64_t pos, int* values) {
    if (pos < 0) return 0;
    
    FILE* fp = fopen(DATA_FILE, "rb");
    if (!fp) return 0;
    
    fseek(fp, pos, SEEK_SET);
    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (count < 0 || count > 200) count = 0;
    
    fread(values, sizeof(int), count, fp);
    fclose(fp);
    return count;
}

static int64_t saveValues(const int* values, int count) {
    FILE* fp = fopen(DATA_FILE, "ab");
    if (!fp) return -1;
    
    int64_t pos = ftell(fp);
    fwrite(&count, sizeof(int), 1, fp);
    fwrite(values, sizeof(int), count, fp);
    fclose(fp);
    return pos;
}

static void appendMeta(const char* key, int len, int64_t pos) {
    FILE* fp = fopen(META_FILE, "ab");
    if (!fp) return;
    
    fwrite(&len, sizeof(int), 1, fp);
    fwrite(key, 1, len, fp);
    fwrite(&pos, sizeof(int64_t), 1, fp);
    fclose(fp);
}

static void loadMeta() {
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_table[i].pos = -1;
    }
    
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return;
    
    int len;
    char buf[65];
    int64_t pos;
    
    while (fread(&len, sizeof(int), 1, fp) == 1) {
        if (len > 64 || len < 0) break;
        if (len > 0 && (int)fread(buf, 1, len, fp) != len) break;
        if (fread(&pos, sizeof(int64_t), 1, fp) != 1) break;
        
        updateIndex(buf, len, pos);
    }
    
    fclose(fp);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    loadMeta();
    
    int n;
    cin >> n;
    
    char cmd[10];
    char index[65];
    int value;
    int values[200];
    int valCount = 0;
    
    for (int i = 0; i < n; ++i) {
        cin >> cmd >> index;
        int idxLen = strlen(index);
        
        int64_t pos = findInIndex(index, idxLen);
        valCount = (pos >= 0) ? loadValues(pos, values) : 0;
        
        if (cmd[0] == 'i') {
            cin >> value;
            
            if (pos == -1) {
                values[0] = value;
                valCount = 1;
                int64_t newPos = saveValues(values, valCount);
                appendMeta(index, idxLen, newPos);
                updateIndex(index, idxLen, newPos);
            } else {
                int left = 0, right = valCount;
                while (left < right) {
                    int mid = (left + right) >> 1;
                    if (values[mid] < value) left = mid + 1;
                    else right = mid;
                }
                
                if (left == valCount || values[left] != value) {
                    for (int j = valCount; j > left; j--) {
                        values[j] = values[j-1];
                    }
                    values[left] = value;
                    valCount++;
                    int64_t newPos = saveValues(values, valCount);
                    appendMeta(index, idxLen, newPos);
                    updateIndex(index, idxLen, newPos);
                }
            }
        }
        else if (cmd[0] == 'd') {
            cin >> value;
            
            if (pos >= 0) {
                int left = 0, right = valCount;
                while (left < right) {
                    int mid = (left + right) >> 1;
                    if (values[mid] < value) left = mid + 1;
                    else right = mid;
                }
                
                if (left < valCount && values[left] == value) {
                    for (int j = left; j < valCount - 1; j++) {
                        values[j] = values[j+1];
                    }
                    valCount--;
                    
                    int64_t newPos = (valCount > 0) ? saveValues(values, valCount) : -1;
                    appendMeta(index, idxLen, newPos);
                    updateIndex(index, idxLen, newPos);
                }
            }
        }
        else {
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
    
    return 0;
}
