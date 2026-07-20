#include <bits/stdc++.h>
using namespace std;

// File-based key-value database
// Using append-only approach for data, with a metadata file for index info

const string META_FILE = "meta.dat";
const string DATA_FILE = "data.dat";

struct ValueList {
    int count;
    int values[100]; // Max values per index (should be enough)
};

unordered_map<string, streampos> index_map;

void loadIndex() {
    ifstream meta(META_FILE, ios::binary);
    if (!meta) return;
    
    while (meta) {
        size_t len;
        meta.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!meta) break;
        
        string key(len, '\0');
        meta.read(&key[0], len);
        
        streampos pos;
        meta.read(reinterpret_cast<char*>(&pos), sizeof(pos));
        
        index_map[key] = pos;
    }
}

void saveIndex() {
    ofstream meta(META_FILE, ios::binary | ios::trunc);
    for (const auto& [key, pos] : index_map) {
        size_t len = key.length();
        meta.write(reinterpret_cast<const char*>(&len), sizeof(len));
        meta.write(key.data(), len);
        meta.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
    }
}

vector<int> loadValues(streampos pos) {
    vector<int> values;
    ifstream data(DATA_FILE, ios::binary);
    if (!data) return values;
    
    data.seekg(pos);
    int count;
    data.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    for (int i = 0; i < count; ++i) {
        int v;
        data.read(reinterpret_cast<char*>(&v), sizeof(v));
        values.push_back(v);
    }
    return values;
}

streampos saveValues(const vector<int>& values) {
    ofstream data(DATA_FILE, ios::binary | ios::app);
    streampos pos = data.tellp();
    
    int count = values.size();
    data.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (int v : values) {
        data.write(reinterpret_cast<const char*>(&v), sizeof(v));
    }
    data.flush();
    return pos;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    // Load existing index
    loadIndex();
    
    int n;
    cin >> n;
    
    string cmd, index;
    int value;
    
    for (int i = 0; i < n; ++i) {
        cin >> cmd >> index;
        
        if (cmd == "insert") {
            cin >> value;
            auto it = index_map.find(index);
            if (it == index_map.end()) {
                // New index
                vector<int> values = {value};
                streampos pos = saveValues(values);
                index_map[index] = pos;
            } else {
                // Existing index, load values
                vector<int> values = loadValues(it->second);
                // Insert in sorted order
                auto vit = lower_bound(values.begin(), values.end(), value);
                if (vit == values.end() || *vit != value) {
                    values.insert(vit, value);
                    // Write new values at end of file
                    streampos newPos = saveValues(values);
                    it->second = newPos;
                }
            }
        } else if (cmd == "delete") {
            cin >> value;
            auto it = index_map.find(index);
            if (it != index_map.end()) {
                vector<int> values = loadValues(it->second);
                auto vit = lower_bound(values.begin(), values.end(), value);
                if (vit != values.end() && *vit == value) {
                    values.erase(vit);
                    // Write new values at end of file
                    streampos newPos = saveValues(values);
                    it->second = newPos;
                    // Remove index if no values left
                    if (values.empty()) {
                        index_map.erase(it);
                    }
                }
            }
        } else if (cmd == "find") {
            auto it = index_map.find(index);
            if (it == index_map.end()) {
                cout << "null\n";
            } else {
                vector<int> values = loadValues(it->second);
                if (values.empty()) {
                    cout << "null\n";
                } else {
                    for (size_t j = 0; j < values.size(); ++j) {
                        if (j > 0) cout << ' ';
                        cout << values[j];
                    }
                    cout << '\n';
                }
            }
        }
    }
    
    // Save index
    saveIndex();
    
    return 0;
}
