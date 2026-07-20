#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    unordered_map<string, set<int>> data;
    
    int n;
    cin >> n;
    
    string cmd, key;
    int val;
    
    for (int i = 0; i < n; i++) {
        cin >> cmd;
        
        if (cmd == "insert") {
            cin >> key >> val;
            data[key].insert(val);
        } else if (cmd == "delete") {
            cin >> key >> val;
            auto it = data.find(key);
            if (it != data.end()) {
                it->second.erase(val);
            }
        } else if (cmd == "find") {
            cin >> key;
            auto it = data.find(key);
            if (it == data.end() || it->second.empty()) {
                cout << "null\n";
            } else {
                bool first = true;
                for (int v : it->second) {
                    if (!first) cout << ' ';
                    first = false;
                    cout << v;
                }
                cout << '\n';
            }
        }
    }
    
    return 0;
}
