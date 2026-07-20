#include <bits/stdc++.h>
using namespace std;

int main() {
    FILE* fp = fopen("meta.dat", "rb");
    if (!fp) {
        cout << "No meta file\n";
        return 0;
    }
    
    int keylen;
    char buf[65];
    int64_t pos;
    
    while (fread(&keylen, sizeof(int), 1, fp) == 1) {
        cout << "Keylen: " << keylen << "\n";
        if (keylen > 64 || keylen < 0) break;
        if (keylen > 0 && (int)fread(buf, 1, keylen, fp) != keylen) break;
        buf[keylen] = '\0';
        if (fread(&pos, sizeof(int64_t), 1, fp) != 1) break;
        
        cout << "Key: " << buf << " Pos: " << pos << "\n";
    }
    
    fclose(fp);
    return 0;
}
