#include <bits/stdc++.h>
using namespace std;

int main() {
    FILE* fp = fopen("meta.dat", "wb");
    int len = 1;
    char key[] = "a";
    int64_t pos = 999;
    
    fwrite(&len, sizeof(int), 1, fp);
    fwrite(key, 1, len, fp);
    fwrite(&pos, sizeof(int64_t), 1, fp);
    fclose(fp);
    
    // Read back
    fp = fopen("meta.dat", "rb");
    int rlen;
    char rbuf[65];
    int64_t rpos;
    
    fread(&rlen, sizeof(int), 1, fp);
    fread(rbuf, 1, rlen, fp);
    fread(&rpos, sizeof(int64_t), 1, fp);
    fclose(fp);
    
    cout << "Wrote: len=" << len << " key=" << key << " pos=" << pos << endl;
    cout << "Read: len=" << rlen << " key=" << rbuf[0] << " pos=" << rpos << endl;
    
    return 0;
}
