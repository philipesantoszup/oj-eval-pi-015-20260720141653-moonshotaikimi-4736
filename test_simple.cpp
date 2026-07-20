#include <bits/stdc++.h>
using namespace std;

const char* META_FILE = "meta.dat";

int main() {
    FILE* fp = fopen(META_FILE, "rb");
    if (!fp) {
        cout << "No file\n";
        return 0;
    }
    
    int keylen;
    char buf[65];
    int64_t pos;
    int ent = 0;
    
    cout << "Reading file:\n";
    while (fread(&keylen, sizeof(int), 1, fp) == 1) {
        ent++;
        cout << "Entry " << ent << " keylen=" << keylen << endl;
        if (keylen > 64 || keylen < 0) {
            cout << "Bad keylen\n";
            break;
        }
        if (keylen > 0) {
            int r = fread(buf, 1, keylen, fp);
            if (r != keylen) {
                cout << "Read " << r << " of " << keylen << " expected\n";
                break;
            }
        }
        buf[keylen] = '\0';
        
        int r = fread(&pos, sizeof(int64_t), 1, fp);
        if (r != 1) {
            cout << "Failed to read pos\n";
            break;
        }
        
        cout << "Key=" << buf << " pos=" << pos << endl;
    }
    
    fclose(fp);
    return 0;
}
