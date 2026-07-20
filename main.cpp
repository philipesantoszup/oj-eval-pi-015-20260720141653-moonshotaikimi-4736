#include <bits/stdc++.h>
using namespace std;

// File Storage - robust solution with proper hash table

#define META_FILE "meta.dat"
#define DATA_FILE "data.dat"

static int loadValues(int64_t pos, int* values) {
    if (pos < 0) return 0;
    FILE* fp = fopen(DATA_FILE, "rb");
    if (!fp) return 0;
    fseek(fp, pos, SEEK_SET);
    int cnt = 0;
    if (fread(&cnt, 4, 1, fp) != 1) { fclose(fp); return 0; }
    if (cnt < 0 || cnt > 200) cnt = 0;
    for (int i = 0; i < cnt; i++) fread(&values[i], 4, 1, fp);
    fclose(fp);
    return cnt;
}

static int64_t saveValues(const int* values, int cnt) {
    FILE* fp = fopen(DATA_FILE, "ab");
    if (!fp) return -1;
    int64_t pos = ftell(fp);
    fwrite(&cnt, 4, 1, fp);
    for (int i = 0; i < cnt; i++) fwrite(&values[i], 4, 1, fp);
    fclose(fp);
    return pos;
}

static void appendMeta(const char* key, int len, int64_t pos) {
    FILE* fp = fopen(META_FILE, "ab");
    if (!fp) return;
    fwrite(&len, 4, 1, fp);
    fwrite(key, 1, len, fp);
    fwrite(&pos, 8, 1, fp);
    fclose(fp);
}

// Hash table with key storage
// Each entry: position (8 bytes) + key hash + key length + key data offset
struct HashEntry {
    int64_t pos;        // -2 = empty
    uint32_t key_hash;
    uint16_t key_len;
    uint32_t key_off;   // offset in key pool (in bytes)
};

const int HT_SIZE = 65536;
static HashEntry ht[HT_SIZE];

static char key_pool[2000000];  // ~2 MB
static uint32_t kp_used = 0;

static inline uint32_t khash(const char* k, int l) {
    uint32_t h = 5381;
    for (int i = 0; i < l; i++) h = ((h << 5) + h) + k[i];
    return h;
}

static void ht_init() {
    for (int i = 0; i < HT_SIZE; i++) ht[i].pos = -2;
}

static bool keys_equal(uint32_t off1, const char* k2, int l2) {
    return memcmp(key_pool + off1, k2, l2) == 0;
}

static int64_t ht_get(const char* key, int len) {
    uint32_t h = khash(key, len);
    uint32_t idx = h & (HT_SIZE - 1);
    
    for (int probes = 0; probes < HT_SIZE; probes++) {
        if (ht[idx].pos == -2) break;
        if (ht[idx].key_hash == h && ht[idx].key_len == len) {
            if (keys_equal(ht[idx].key_off, key, len)) {
                return ht[idx].pos;
            }
        }
        idx = (idx + 1) & (HT_SIZE - 1);
    }
    return -2;
}

static void ht_put(const char* key, int len, int64_t pos) {
    uint32_t h = khash(key, len);
    uint32_t idx = h & (HT_SIZE - 1);
    
    // Check if already exists
    for (int probes = 0; probes < HT_SIZE; probes++) {
        if (ht[idx].pos == -2) break;
        if (ht[idx].key_hash == h && ht[idx].key_len == len) {
            if (keys_equal(ht[idx].key_off, key, len)) {
                ht[idx].pos = pos;
                return;
            }
        }
        idx = (idx + 1) & (HT_SIZE - 1);
    }
    
    // Find empty slot
    idx = h & (HT_SIZE - 1);
    for (int probes = 0; probes < HT_SIZE; probes++) {
        if (ht[idx].pos < -1) {
            if (kp_used + len > 2000000) return;  // Pool full
            memcpy(key_pool + kp_used, key, len);
            ht[idx].pos = pos;
            ht[idx].key_hash = h;
            ht[idx].key_len = len;
            ht[idx].key_off = kp_used;
            kp_used += len + 1;  // +1 for null
            return;
        }
        idx = (idx + 1) & (HT_SIZE - 1);
    }
}

static void ht_load() {
    ht_init();
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return;
    
    int len;
    char buf[65];
    int64_t pos;
    
    while (fread(&len, 4, 1, fp) == 1 && len <= 64 && len >= 0) {
        if (len > 0 && fread(buf, 1, len, fp) != (size_t)len) break;
        if (fread(&pos, 8, 1, fp) != 1) break;
        if (len > 0) ht_put(buf, len, pos);
    }
    fclose(fp);
}

// Fallback scan
static int64_t scanMeta(const char* key, int len) {
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return -1;
    int64_t res = -1;
    int keylen;
    char buf[65];
    int64_t pos;
    while (fread(&keylen, 4, 1, fp) == 1 && keylen <= 64) {
        if (keylen > 0 && fread(buf, 1, keylen, fp) != (size_t)keylen) break;
        if (fread(&pos, 8, 1, fp) != 1) break;
        if (keylen == len && memcmp(buf, key, len) == 0) res = pos;
    }
    fclose(fp);
    return res;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    ht_load();
    
    int n;
    cin >> n;
    
    char cmd[10], index[65];
    int value, values[200], valCnt;
    
    for (int i = 0; i < n; i++) {
        cin >> cmd >> index;
        int idxLen = strlen(index);
        
        int64_t pos = ht_get(index, idxLen);
        if (pos == -2) pos = scanMeta(index, idxLen);
        valCnt = (pos >= 0) ? loadValues(pos, values) : 0;
        
        if (cmd[0] == 'i') {
            cin >> value;
            if (pos == -1) {
                values[0] = value; valCnt = 1;
                int64_t newPos = saveValues(values, valCnt);
                appendMeta(index, idxLen, newPos);
                ht_put(index, idxLen, newPos);
            } else {
                int lo = 0, hi = valCnt;
                while (lo < hi) {
                    int mid = (lo + hi) >> 1;
                    if (values[mid] < value) lo = mid + 1;
                    else hi = mid;
                }
                if (lo == valCnt || values[lo] != value) {
                    for (int j = valCnt; j > lo; j--) values[j] = values[j-1];
                    values[lo] = value; valCnt++;
                    int64_t newPos = saveValues(values, valCnt);
                    appendMeta(index, idxLen, newPos);
                    ht_put(index, idxLen, newPos);
                }
            }
        } else if (cmd[0] == 'd') {
            cin >> value;
            if (pos >= 0) {
                int lo = 0, hi = valCnt;
                while (lo < hi) {
                    int mid = (lo + hi) >> 1;
                    if (values[mid] < value) lo = mid + 1;
                    else hi = mid;
                }
                if (lo < valCnt && values[lo] == value) {
                    for (int j = lo; j < valCnt - 1; j++) values[j] = values[j+1];
                    valCnt--;
                    int64_t newPos = valCnt > 0 ? saveValues(values, valCnt) : -1;
                    appendMeta(index, idxLen, newPos);
                    ht_put(index, idxLen, newPos);
                }
            }
        } else {
            if (valCnt == 0) cout << "null\n";
            else {
                for (int j = 0; j < valCnt; j++) {
                    if (j) cout << ' ';
                    cout << values[j];
                }
                cout << '\n';
            }
        }
    }
    return 0;
}
