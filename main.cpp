#include <bits/stdc++.h>
using namespace std;

/* ===============================
   FULLY ASSOCIATIVE LRU CACHE
   =============================== */
class FullyAssociativeCache {
    int capacity;
    list<int> dq;
    unordered_map<int, list<int>::iterator> pos;

public:
    long hits = 0, misses = 0;

    FullyAssociativeCache(int cap) : capacity(cap) {}

    bool access(int addr) {
        if (pos.find(addr) == pos.end()) {
            misses++;
            if (dq.size() == capacity) {
                int lru = dq.back();
                dq.pop_back();
                pos.erase(lru);
            }
        } else {
            hits++;
            dq.erase(pos[addr]);
        }
        dq.push_front(addr);
        pos[addr] = dq.begin();
        return true;
    }
};

/* ===============================
   DIRECT MAPPED CACHE
   =============================== */
class DirectMappedCache {
    vector<int> cache;

public:
    long hits = 0, misses = 0;

    DirectMappedCache(int size) : cache(size, -1) {}

    bool access(int addr) {
        int index = addr % cache.size();
        if (cache[index] == addr) {
            hits++;
            return true;
        }
        misses++;
        cache[index] = addr;
        return false;
    }
};

/* ===============================
   CACHE LEVEL (L1 / L2 / L3)
   =============================== */
struct CacheLevel {
    string name;
    int latency;
    bool isDirectMapped;

    FullyAssociativeCache fa;
    DirectMappedCache dm;

    CacheLevel* next;

    CacheLevel(string n, int cap, int lat, bool direct)
        : name(n),
          latency(lat),
          isDirectMapped(direct),
          fa(cap),
          dm(cap),
          next(nullptr) {}

    bool access(int addr) {
        if (isDirectMapped)
            return dm.access(addr);
        else
            return fa.access(addr);
    }

    long hits() const {
        return isDirectMapped ? dm.hits : fa.hits;
    }

    long misses() const {
        return isDirectMapped ? dm.misses : fa.misses;
    }
};

/* ===============================
   ACCESS FLOW (MULTI-LEVEL)
   =============================== */
bool accessHierarchy(CacheLevel* level, int addr, int& cycles) {
    cycles += level->latency;

    if (level->access(addr)) {
        return true;
    }

    if (level->next) {
        bool found = accessHierarchy(level->next, addr, cycles);
        level->access(addr); // promote upward
        return found;
    }

    level->access(addr); // insert at lowest level
    return false;
}

/* ===============================
   MAIN
   =============================== */
int main() {
    int choice;
    cout << "Choose Cache Type:\n";
    cout << "1. Direct-Mapped\n";
    cout << "2. Fully Associative (LRU)\n";
    cin >> choice;

    bool directMapped = (choice == 1);

    // Create cache hierarchy
    CacheLevel L1("L1", 4, 1, directMapped);
    CacheLevel L2("L2", 8, 5, directMapped);
    CacheLevel L3("L3", 16, 15, directMapped);

    L1.next = &L2;
    L2.next = &L3;

    int n;
    cout << "Enter number of memory accesses: ";
    cin >> n;

    vector<int> accesses(n);
    cout << "Enter memory addresses:\n";
    for (int i = 0; i < n; i++)
        cin >> accesses[i];

    int totalCycles = 0;
    for (int addr : accesses)
        accessHierarchy(&L1, addr, totalCycles);

    auto printStats = [](CacheLevel& c) {
        long total = c.hits() + c.misses();
        double hitRate = total ? (double)c.hits() / total : 0;
        cout << c.name << " -> Hits: " << c.hits()
             << " Misses: " << c.misses()
             << " Hit Rate: " << hitRate << endl;
    };

    cout << "\n--- Cache Statistics ---\n";
    printStats(L1);
    printStats(L2);
    printStats(L3);

    double AMAT = (double)totalCycles / accesses.size();
    cout << "\nAverage Memory Access Time (AMAT): "
         << AMAT << " cycles\n";

    return 0;
}
