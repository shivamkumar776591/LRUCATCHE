#include <bits/stdc++.h>
using namespace std;

/* ===============================
   CACHE CONFIGURATION
   =============================== */
struct CacheConfig {
    string name;
    int capacity;        // Total cache size in bytes
    int block_size;      // Cache line/block size (typically 64 bytes)
    int associativity;   // 1=direct, N=N-way, SIZE=fully associative
    int latency;            // Access latency in cycles
    
    // Calculated fields
    int num_blocks;
    int num_sets;
    int offset_bits;
    int index_bits;
    int tag_bits;
    
    CacheConfig(string n, int cap, int blk, int assoc, int lat)
        : name(n), capacity(cap), block_size(blk), associativity(assoc), latency(lat) {
        num_blocks = capacity / block_size;
        num_sets = num_blocks / associativity;
        offset_bits = log2(block_size);
        index_bits = (num_sets > 1) ? log2(num_sets) : 0;
        tag_bits = 32 - offset_bits - index_bits;  // Assuming 32-bit addresses
    }
    
    void printConfig() const {
        cout << "\n=== " << name << " Configuration ===\n";
        cout << "Capacity: " << capacity / 1024 << " KB\n";
        cout << "Block Size: " << block_size << " bytes\n";
        cout << "Associativity: ";
        if (associativity == 1) cout << "Direct-Mapped\n";
        else if (associativity == num_blocks) cout << "Fully Associative\n";
        else cout << associativity << "-way Set Associative\n";
        cout << "Number of Sets: " << num_sets << "\n";
        cout << "Number of Blocks: " << num_blocks << "\n";
        cout << "Access Latency: " << latency << " cycles\n";
        cout << "Address Breakdown: Tag(" << tag_bits << ") | Index(" 
             << index_bits << ") | Offset(" << offset_bits << ")\n";
        
        // // Calculate overhead
 
    }
    
    // Extract tag, index, offset from address
    int getTag(int addr) const {
        return addr >> (offset_bits + index_bits);
    }
    
    int getIndex(int addr) const {
        return (addr >> offset_bits) & ((1 << index_bits) - 1);
    }
    
    int getBlockAddr(int addr) const {
        return addr >> offset_bits;  // Remove offset to get block address
    }
};

/* ===============================
   LRU TRACKER USING DOUBLY-LINKED LIST
   =============================== */
class LRUTracker {
private:
    struct Node {
        int tag;
        Node* prev;
        Node* next;
        
        Node(int t) : tag(t), prev(nullptr), next(nullptr) {}
    };
    
    Node* head;  // Most recently used
    Node* tail;  // Least recently used
    unordered_map<int, Node*> tag_to_node;
    int capacity;
    
public:
    LRUTracker(int cap) : head(nullptr), tail(nullptr), capacity(cap) {}
    
    ~LRUTracker() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
    }
    
    // Move existing node to front (MRU position)
    void moveToFront(Node* node) {
        if (node == head) return;  // Already at front
        
        // Remove from current position
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        
        // Update tail if necessary
        if (node == tail) tail = node->prev;
        
        // Move to front
        node->prev = nullptr;
        node->next = head;
        if (head) head->prev = node;
        head = node;
        
        // Update tail if list was empty
        if (!tail) tail = head;
    }
    
    // Add new node to front (MRU position)
    void addToFront(int tag) {
        Node* node = new Node(tag);
        tag_to_node[tag] = node;
        
        node->next = head;
        if (head) head->prev = node;
        head = node;
        
        if (!tail) tail = head;
    }
    
    // Check if tag exists and update LRU
    bool access(int tag) {
        if (tag_to_node.find(tag) != tag_to_node.end()) {
            moveToFront(tag_to_node[tag]);
            return true;  // Hit
        }
        return false;  // Miss
    }
    
    // Insert new tag, evict LRU if needed
    int insert(int tag) {
        int evicted = -1;
        
        // Check if we need to evict
        if (tag_to_node.size() >= capacity) {
            evicted = evictLRU();
        }
        
        addToFront(tag);
        return evicted;
    }
    
    // Evict least recently used
    int evictLRU() {
        if (!tail) return -1;
        
        int evicted_tag = tail->tag;
        Node* old_tail = tail;
        
        // Remove from tail
        tail = tail->prev;
        if (tail) tail->next = nullptr;
        else head = nullptr;  // List is now empty
        
        tag_to_node.erase(evicted_tag);
        delete old_tail;
        
        return evicted_tag;
    }
    
    bool isFull() const {
        return tag_to_node.size() >= capacity;
    }
    
    int size() const {
        return tag_to_node.size();
    }
    
    // For debugging
    void printLRUOrder() const {
        cout << "LRU Order (MRU->LRU): ";
        Node* curr = head;
        while (curr) {
            cout << curr->tag << " ";
            curr = curr->next;
        }
        cout << endl;
    }
};

/* ===============================
   N-WAY SET ASSOCIATIVE CACHE (LRU using DLL)
   =============================== */
class SetAssociativeCache {
    struct CacheSet {
        LRUTracker lru_tracker;
        unordered_set<int> valid_tags;
        
        CacheSet(int associativity) : lru_tracker(associativity) {}
        
        bool access(int tag) {
            // Check if tag is in this set
            if (valid_tags.find(tag) != valid_tags.end()) {
                lru_tracker.access(tag);  // Update LRU order
                return true;  // Hit
            }
            
            // Miss - insert new tag
            int evicted = lru_tracker.insert(tag);
            if (evicted != -1) {
                valid_tags.erase(evicted);  // Remove evicted tag
            }
            valid_tags.insert(tag);
            
            return false;  // Miss
        }
    };
    
    vector<CacheSet> sets;
    CacheConfig config;

public:
    long hits = 0, misses = 0;
    long cold_misses = 0, capacity_misses = 0, conflict_misses = 0;
   

    SetAssociativeCache(const CacheConfig& cfg) 
        : config(cfg) {
        // Initialize sets with appropriate associativity
        for (int i = 0; i < config.num_sets; i++) {
            sets.emplace_back(config.associativity);
        }
    }

    bool access(int addr) {
        
        int index = config.getIndex(addr);
        int tag = config.getTag(addr);
        
        auto& set = sets[index];
        
        // Try to access
        if (set.access(tag)) {
            hits++;
            return true;  // Hit
        }
       
        misses++;
        
       
        
        return false;  // Miss
    }
    
    void printStats() const {
        long total = hits + misses;
        double hit_rate = total ? (double)hits / total : 0;
        
        cout << config.name << " Statistics:\n";
        cout << "  Hits: " << hits << " | Misses: " << misses << "\n";
        cout << "  Hit Rate: " << fixed << setprecision(2) << (hit_rate * 100) << "%\n";
       
    }
    
    void printSetStatus(int set_index) const {
        if (set_index >= 0 && set_index < sets.size()) {
            cout << "Set " << set_index << " - ";
            sets[set_index].lru_tracker.printLRUOrder();
        }
    }
};

/* ===============================
   DIRECT MAPPED CACHE (for comparison)
   =============================== */
class DirectMappedCache {
    struct Block {
        int tag;
        bool valid;
        
        Block() : tag(-1), valid(false) {}
    };
    
    vector<Block> blocks;
    CacheConfig config;

public:
    long hits = 0, misses = 0;
    long cold_misses = 0, capacity_misses = 0, conflict_misses = 0;
    unordered_set<int> seen_blocks;

    DirectMappedCache(const CacheConfig& cfg) 
        : config(cfg) {
        blocks.resize(config.num_blocks);
    }

    bool access(int addr) {
        int block_addr = config.getBlockAddr(addr);
        int index = config.getIndex(addr);
        int tag = config.getTag(addr);
        
        auto& block = blocks[index];
        
        // Check for hit
        if (block.valid && block.tag == tag) {
            hits++;
            return true;
        }
        
        // Miss - classify
        misses++;
        
        if (seen_blocks.find(block_addr) == seen_blocks.end()) {
            cold_misses++;
            seen_blocks.insert(block_addr);
        } else {
            conflict_misses++;  // Direct-mapped only has cold and conflict
        }
        
        // Update block
        block.tag = tag;
        block.valid = true;
        
        return false;
    }
    
    void printStats() const {
        long total = hits + misses;
        double hit_rate = total ? (double)hits / total : 0;
        
        cout << config.name << " Statistics:\n";
        cout << "  Hits: " << hits << " | Misses: " << misses << "\n";
        cout << "  Hit Rate: " << fixed << setprecision(2) << (hit_rate * 100) << "%\n";
        
    }
};

/* ===============================
   CACHE LEVEL (Wrapper)
   =============================== */
struct CacheLevel {
    CacheConfig config;
    SetAssociativeCache* set_assoc_cache;
    DirectMappedCache* direct_cache;
    bool is_direct_mapped;
    CacheLevel* next;

    CacheLevel(const CacheConfig& cfg, bool direct = false)
        : config(cfg), set_assoc_cache(nullptr), direct_cache(nullptr), 
          is_direct_mapped(direct), next(nullptr) {
        
        if (direct) {
            direct_cache = new DirectMappedCache(cfg);
        } else {
            set_assoc_cache = new SetAssociativeCache(cfg);
        }
    }
    
    ~CacheLevel() {
        delete set_assoc_cache;
        delete direct_cache;
    }

    bool access(int addr) {
        if (is_direct_mapped) {
            return direct_cache->access(addr);
        } else {
            return set_assoc_cache->access(addr);
        }
    }

    void printStats() const {
        if (is_direct_mapped) {
            direct_cache->printStats();
        } else {
            set_assoc_cache->printStats();
        }
    }
};

/* ===============================
   ACCESS FLOW (MULTI-LEVEL)
   =============================== */
int accessHierarchy(CacheLevel* level, int addr, int& total_cycles) {
    total_cycles += level->config.latency;

    if (level->access(addr)) {
        return level->config.latency;  // Hit at this level
    }

    if (level->next) {
        int lower_cycles = accessHierarchy(level->next, addr, total_cycles);
        return level->config.latency + lower_cycles;
    }

    // Miss to main memory
    int mem_latency = 200;  // Main memory latency
    total_cycles += mem_latency;
    return level->config.latency + mem_latency;
}

/* ===============================
   WORKLOAD GENERATORS
   =============================== */
vector<int> generateSequentialAccess(int size, int block_size) {
    vector<int> accesses;
    for (int i = 0; i < size; i++) {
        accesses.push_back(i * block_size);
    }
    return accesses;
}



vector<int> generateMatrixMultiply(int n, int block_size) {
    // Simulates A[i][k] * B[k][j] access pattern
    vector<int> accesses;
    int base_A = 0;
    int base_B = n * n * 4;  // Assuming 4 bytes per element
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                accesses.push_back(base_A + (i * n + k) * 4);  // A[i][k]
                accesses.push_back(base_B + (k * n + j) * 4);  // B[k][j]
            }
        }
    }
    return accesses;
}

/* ===============================
   MAIN
   =============================== */
int main() {
    cout << "===================================\n";
    cout << "  MULTI-LEVEL CACHE SIMULATOR\n";
    cout << "  (Using Doubly-Linked List LRU)\n";
    cout << "===================================\n\n";

    // Choose cache organization
    int org_choice;
    cout << "Choose Cache Organization:\n";
    cout << "1. Direct-Mapped (1-way)\n";
    cout << "2. 4-way Set Associative\n";
    cout << "3. 8-way Set Associative\n";
    cout << "4. 16-way Set Associative\n";
    cout << "5. Fully Associative\n";
    cout << "Choice: ";
    cin >> org_choice;

    int l1_assoc, l2_assoc, l3_assoc;
    bool is_direct = false;
    
    switch(org_choice) {
        case 1:
            is_direct = true;
            l1_assoc = 1; l2_assoc = 1; l3_assoc = 1;
            break;
        case 2:
            l1_assoc = 4; l2_assoc = 4; l3_assoc = 4;
            break;
        case 3:
            l1_assoc = 8; l2_assoc = 8; l3_assoc = 8;
            break;
        case 4:
            l1_assoc = 16; l2_assoc = 16; l3_assoc = 16;
            break;
        case 5:
            l1_assoc = 512; l2_assoc = 4096; l3_assoc = 131072;  // Fully associative
            break;
        default:
            l1_assoc = 8; l2_assoc = 8; l3_assoc = 16;
    }

    // Create realistic cache hierarchy
    CacheConfig l1_config("L1", 32*1024, 64, l1_assoc, 4);      // 32KB, 64B blocks, 4 cycles
    CacheConfig l2_config("L2", 256*1024, 64, l2_assoc, 12);    // 256KB, 64B blocks, 12 cycles
    CacheConfig l3_config("L3", 8*1024*1024, 64, l3_assoc, 40); // 8MB, 64B blocks, 40 cycles

    CacheLevel L1(l1_config, is_direct);
    CacheLevel L2(l2_config, is_direct);
    CacheLevel L3(l3_config, is_direct);

    L1.next = &L2;
    L2.next = &L3;

    // Print configurations
    l1_config.printConfig();
    l2_config.printConfig();
    l3_config.printConfig();

    // Choose workload
    cout << "\n\nChoose Workload Pattern:\n";
    cout << "1. Sequential Access (10K accesses)\n";
    cout << "2.Matrix Multiplication (32x32)\n";
    cout << "3. Custom Input\n";
    
    cout << "Choice: ";
    
    int workload_choice;
    cin >> workload_choice;

    vector<int> accesses;
    
    switch(workload_choice) {
        case 1:
            accesses = generateSequentialAccess(10000, 64);
            cout << "Generated 10,000 sequential accesses\n";
            break;
        
        
        
        case 2:
            accesses = generateMatrixMultiply(32, 64);
            cout << "Generated matrix multiply accesses (32x32)\n";
            break;
        case 3:
            int n;
            cout << "Enter number of memory accesses: ";
            cin >> n;
            accesses.resize(n);
            cout << "Enter memory addresses:\n";
            for (int i = 0; i < n; i++)
                cin >> accesses[i];
            break;
    }

    // Simulate accesses
    cout << "\nSimulating " << accesses.size() << " memory accesses...\n";
    
    int total_cycles = 0;
    for (int addr : accesses) {
        int dummy = 0;
        accessHierarchy(&L1, addr, dummy);
        total_cycles = dummy;
    }

    // Print results
    cout << "\n\n========== SIMULATION RESULTS ==========\n\n";
    
    L1.printStats();
    cout << endl;
    L2.printStats();
    cout << endl;
    L3.printStats();

    double AMAT = (double)total_cycles / accesses.size();
    cout << "\n========================================\n";
    cout << "Average Memory Access Time (AMAT): " << fixed << setprecision(2) 
         << AMAT << " cycles\n";
    
    // Overall statistics
    long total_accesses = 0;
    long total_hits = 0;
    long total_misses = 0;
    
    if (is_direct) {
        total_accesses = L1.direct_cache->hits + L1.direct_cache->misses;
        total_hits = L1.direct_cache->hits;
        total_misses = L1.direct_cache->misses;
    } else {
        total_accesses = L1.set_assoc_cache->hits + L1.set_assoc_cache->misses;
        total_hits = L1.set_assoc_cache->hits;
        total_misses = L1.set_assoc_cache->misses;
    }
    
    double overall_miss_rate = (double)total_misses / total_accesses;
    cout << "Overall L1 Miss Rate: " << (overall_miss_rate * 100) << "%\n";
    cout << "Total CPU Cycles: " << total_cycles << "\n";
    cout << "========================================\n";

    return 0;
}