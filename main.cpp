#include <bits/stdc++.h>
using namespace std;

// File Storage - minimal memory approach
// Use C stdio instead of C++ streams for lower overhead

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

// Ultra light hash: key -> position, with linear probing on collision
// 65536 entries * 8 bytes = 512KB
static int64_t htable[65536];
static uint16_t hkey_len[65536]; // store key length for validation

static inline uint32_t khash(const char* k, int l) {
    uint32_t h = 5381;
    for (int i = 0; i < l; i++) h = ((h << 5) + h) + k[i];
    return h;
}

static void h_init() {
    for (int i = 0; i < 65536; i++) htable[i] = -2; // -2 = empty
}

// Linear scan of meta file, populate hash table with latest entries
static void h_load() {
    h_init();
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) return;
    
    int len;
    char buf[65];
    int64_t pos;
    
    while (fread(&len, 4, 1, fp) == 1 && len <= 64 && len >= 0) {
        if (len > 0 && fread(buf, 1, len, fp) != (size_t)len) break;
        if (fread(&pos, 8, 1, fp) != 1) break;
        
        if (len > 0) {
            // Insert/replace in hash table
            uint32_t h = khash(buf, len) & 65535;
            for (int probes = 0; probes < 65536; probes++) {
                uint32_t idx = (h + probes) & 65535;
                if (htable[idx] < -1 || hkey_len[idx] == len) {
                    // Check if same key
                    if (htable[idx] >= -1) {
                        // Check key match by re-scanning meta file for this slot
                        // This is getting complex... simpler: always replace
                    }
                    htable[idx] = pos;
                    hkey_len[idx] = len;
                    break;
                }
            }
        }
    }
    fclose(fp);
}

static int64_t h_get(const char* key, int len) {
    uint32_t h = khash(key, len) & 65535;
    for (int probes = 0; probes < 65536; probes++) {
        uint32_t idx = (h + probes) & 65535;
        if (htable[idx] == -2) break; // Empty
        if (htable[idx] >= -1 && hkey_len[idx] == len) {
            // Key match assumed based on hash and length
            // Need full key comparison - fall back to file scan
            break;
        }
    }
    return -2; // Not in hash
}

static void h_put(const char* key, int len, int64_t pos) {
    uint32_t h = khash(key, len) & 65535;
    for (int probes = 0; probes < 65536; probes++) {
        uint32_t idx = (h + probes) & 65535;
        if (htable[idx] < -1) {
            htable[idx] = pos;
            hkey_len[idx] = len;
            return;
        }
    }
}

// Fallback: scan entire meta file for key
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
    
    h_load();
    
    int n;
    cin >> n;
    
    char cmd[10], index[65];
    int value, values[200], valCnt;
    
    for (int i = 0; i < n; i++) {
        cin >> cmd >> index;
        int idxLen = strlen(index);
        
        int64_t pos = h_get(index, idxLen);
        if (pos == -2) pos = scanMeta(index, idxLen);
        valCnt = (pos >= 0) ? loadValues(pos, values) : 0;
        
        if (cmd[0] == 'i') {
            cin >> value;
            if (pos == -1) {
                values[0] = value; valCnt = 1;
                int64_t newPos = saveValues(values, valCnt);
                appendMeta(index, idxLen, newPos);
                h_put(index, idxLen, newPos);
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
                    h_put(index, idxLen, newPos);
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
                    h_put(index, idxLen, newPos);
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
