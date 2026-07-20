#include <bits/stdc++.h>
using namespace std;

int main() {
    int n = 10000;
    cout << n << endl;
    
    unordered_map<string, vector<int>> data;
    
    mt19937 rng(12345);
    
    for (int i = 0; i < n; i++) {
        int op = rng() % 3;
        string key = "key" + to_string(rng() % 100);
        
        if (op == 0) {  // insert
            int val = rng() % 1000;
            // Check if value already exists for this key
            auto& vec = data[key];
            if (find(vec.begin(), vec.end(), val) == vec.end()) {
                vec.push_back(val);
                sort(vec.begin(), vec.end());
                cout << "insert " << key << " " << val << endl;
            } else {
                // Duplicate, skip
                i--;
            }
        } else if (op == 1) {  // delete
            auto it = data.find(key);
            if (it != data.end() && !it->second.empty()) {
                int val = it->second[rng() % it->second.size()];
                cout << "delete " << key << " " << val << endl;
                it->second.erase(remove(it->second.begin(), it->second.end(), val), it->second.end());
            } else {
                // Can't delete, skip
                i--;
            }
        } else {  // find
            cout << "find " << key << endl;
        }
    }
    
    return 0;
}