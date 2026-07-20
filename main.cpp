#include <bits/stdc++.h>
using namespace std;

// Memory-optimized file-based key-value database
// Uses compact custom hash table + append-only file storage

const char* META_FILE = "meta.dat";
const char* DATA_FILE = "data.dat";

// Compact hash table for in-memory index
// Each entry: hash(key) -> (key_offset_in_pool, key_len, data_pos)
// Keys are stored in a separate pool

struct IndexEntry {
    uint32_t key_offset;
    uint8_t key_len;
    int32_t _pad;  // alignment
    int64_t data_pos;
    uint32_t next;  // for chaining
    bool used;
    
    IndexEntry() : key_offset(0), key_len(0), _pad(0), data_pos(-1), next(0xFFFFFFFF), used(false) {}
};

// Key pool: store all keys contiguously
static const int MAX_ENTRIES = 100000;
static const int KEY_POOL_SIZE = MAX_ENTRIES * 68;  // max 64 + padding per key
static const int HASH_SIZE = 131072;  // 2^17, larger to reduce collisions

static IndexEntry hash_table[HASH_SIZE];
static char key_pool[KEY_POOL_SIZE];
static uint32_t key_pool_used = 0;

static inline uint32_t hashKey(const char* key, int len) {
    uint32_t h = 0x811c9dc5;
    for (int i = 0; i < len; i++) {
        h ^= (uint8_t)key[i];
        h *= 0x01000193;
    }
    return h % HASH_SIZE;
}

static int64_t findInIndex(const char* key, int len) {
    uint32_t h = hashKey(key, len);
    uint32_t idx = h;
    
    while (hash_table[idx].used) {
        if (hash_table[idx].key_len == len) {
            const char* stored = key_pool + hash_table[idx].key_offset;
            if (memcmp(stored, key, len) == 0) {
                return hash_table[idx].data_pos;
            }
        }
        
        if (hash_table[idx].next != 0xFFFFFFFF) {
            idx = hash_table[idx].next;
        } else {
            break;
        }
        
        if (idx >= (uint32_t)HASH_SIZE) break;
    }
    
    return -1;
}

static void updateIndex(const char* key, int len, int64_t pos) {
    uint32_t h = hashKey(key, len);
    uint32_t idx = h;
    
    // Check if key exists
    while (hash_table[idx].used) {
        if (hash_table[idx].key_len == len) {
            const char* stored = key_pool + hash_table[idx].key_offset;
            if (memcmp(stored, key, len) == 0) {
                hash_table[idx].data_pos = pos;
                return;
            }
        }
        
        if (hash_table[idx].next != 0xFFFFFFFF && hash_table[idx].next < (uint32_t)HASH_SIZE) {
            idx = hash_table[idx].next;
        } else {
            break;
        }
    }
    
    // Find next empty slot for chaining
    if (hash_table[idx].used) {
        uint32_t chain_idx = h;
        while (hash_table[chain_idx].used) {
            if (chain_idx == 0xFFFFFFFF || chain_idx >= (uint32_t)HASH_SIZE) return;
            chain_idx++;
            if (chain_idx == HASH_SIZE) chain_idx = 0;
            if (chain_idx == h) return;  // Full
        }
        
        hash_table[idx].next = chain_idx;
        idx = chain_idx;
    }
    
    // Store new entry
    hash_table[idx].used = true;
    hash_table[idx].key_len = len;
    hash_table[idx].key_offset = key_pool_used;
    memcpy(key_pool + key_pool_used, key, len);
    key_pool_used += len + 1;  // +1 for null terminator safety
    hash_table[idx].data_pos = pos;
}

// Load values from data file
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
    
    for (int i = 0; i < count; i++) {
        fread(&values[i], sizeof(int), 1, fp);
    }
    fclose(fp);
    return count;
}

// Save values and return position
static int64_t saveValues(const int* values, int count) {
    FILE* fp = fopen(DATA_FILE, "ab");
    if (!fp) return -1;
    
    int64_t pos = ftell(fp);
    fwrite(&count, sizeof(int), 1, fp);
    fwrite(values, sizeof(int), count, fp);
    fclose(fp);
    return pos;
}

// Append entry to meta file (for persistence, but we don't read from it during runtime)
static void appendMeta(const char* key, int len, int64_t pos) {
    FILE* fp = fopen(META_FILE, "ab");
    if (!fp) return;
    
    fwrite(&len, sizeof(int), 1, fp);
    fwrite(key, 1, len, fp);
    fwrite(&pos, sizeof(int64_t), 1, fp);
    fclose(fp);
}

// Load existing meta file into memory index
static void loadMeta() {
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return;
    
    int len;
    char buf[65];
    int64_t pos;
    
    while (fread(&len, sizeof(int), 1, fp) == 1) {
        if (len > 64 || len <= 0) break;
        if ((int)fread(buf, 1, len, fp) != len) break;
        if (fread(&pos, sizeof(int64_t), 1, fp) != 1) break;
        
        updateIndex(buf, len, pos);
    }
    
    fclose(fp);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    // Load existing index
    loadMeta();
    
    int n;
    cin >> n;
    
    char cmd[10];
    char index[65];
    int value;
    int values[200];
    int valCount;
    
    for (int i = 0; i < n; ++i) {
        cin >> cmd >> index;
        int idxLen = strlen(index);
        
        int64_t pos = findInIndex(index, idxLen);
        valCount = (pos >= 0) ? loadValues(pos, values) : 0;
        
        if (cmd[0] == 'i') {  // insert
            cin >> value;
            
            if (pos == -1) {
                // New entry
                values[0] = value;
                valCount = 1;
                int64_t newPos = saveValues(values, 1);
                appendMeta(index, idxLen, newPos);
                updateIndex(index, idxLen, newPos);
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
                    int64_t newPos = saveValues(values, valCount);
                    appendMeta(index, idxLen, newPos);
                    updateIndex(index, idxLen, newPos);
                }
            }
        }
        else if (cmd[0] == 'd') {  // delete
            cin >> value;
            
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
                    
                    int64_t newPos;
                    if (valCount == 0) {
                        newPos = -1;
                    } else {
                        newPos = saveValues(values, valCount);
                    }
                    appendMeta(index, idxLen, newPos);
                    updateIndex(index, idxLen, newPos);
                }
            }
        }
        else {  // find
            if (pos < 0 || valCount == 0) {
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
