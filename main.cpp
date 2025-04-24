#include <chrono>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iomanip>

#include "include/arenaAllocator.hpp"
#include "include/stlArenaAdapter.hpp"

int NUM_ALLOCATIONS = 1000000;

// Timer utility class for benchmarking
class Timer {
private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
    bool stopped_ = false;
    
public:
    explicit Timer(std::string name) 
        : name_(std::move(name)), start_(std::chrono::high_resolution_clock::now()) {}
    
    ~Timer() {
        if (!stopped_) stop();
    }
    
    void stop() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        std::cout << std::left << std::setw(30) << name_ << ": " 
                  << std::right << std::setw(10) << duration.count() / 1000.0 
                  << " ms" << std::endl;
        stopped_ = true;
    }
};

// Objects of different sizes for testing
struct SmallObject {
    int value;
    
    explicit SmallObject(int v = 0) : value(v) {}
    ~SmallObject() = default;
};

struct MediumObject {
    int values[32];
    double extra;
    
    explicit MediumObject(int v = 0) {
        std::fill_n(values, 32, v);
        extra = v * 1.5;
    }
    
    ~MediumObject() = default;
};

struct LargeObject {
    std::string name;
    std::vector<int> data;
    
    explicit LargeObject(int size = 100) : name("LargeObject") {
        data.resize(size);
        std::iota(data.begin(), data.end(), 0);
    }
    
    ~LargeObject() = default;
};

void benchmark_simple_allocation() {
    std::cout << "\n=== Simple Allocation Benchmark (" << NUM_ALLOCATIONS << " iterations) ===" << std::endl;
    
    // Standard new/delete
    {
        Timer timer("new/delete");
        std::vector<SmallObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            objects.push_back(new SmallObject(i));
        }
        
        for (auto* obj : objects) {
            delete obj;
        }
    }
    
    // std::allocator
    {
        Timer timer("std allocator");
        std::allocator<SmallObject> alloc;
        std::allocator_traits<decltype(alloc)> traits;
        std::vector<SmallObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            SmallObject* obj = alloc.allocate(1);
            traits.construct(alloc, obj, i);
            objects.push_back(obj);
        }
        
        for (auto* obj : objects) {
            //alloc.destroy(obj);
            traits.destroy(alloc, obj);
            alloc.deallocate(obj, 1);
        }
    }
    
    // Arena allocator
    {
        Timer timer("Arena allocator");
        ArenaAllocator<> arena;
        std::vector<SmallObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            objects.push_back(arena.create<SmallObject>(i));
        }
        
        // No need to manually delete objects
    }
    
    // Arena allocator with reset
    {
        Timer timer("Arena allocator (with reset)");
        ArenaAllocator<> arena;
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            SmallObject* obj = arena.create<SmallObject>(i);
            if (i % 10000 == 0) {
                arena.reset();
            }
        }
    }
    
    std::cout << std::endl;
}

void benchmark_object_sizes() {
    std::cout << "\n=== Object Size Benchmark (" << NUM_ALLOCATIONS << " iterations) ===" << std::endl;
    
    std::cout << "Small Objects:" << std::endl;
    
    {
        Timer timer("new/delete");
        std::vector<SmallObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            objects.push_back(new SmallObject(i));
        }
        
        for (auto* obj : objects) {
            delete obj;
        }
    }
    
    {
        Timer timer("Arena allocator");
        ArenaAllocator<> arena;
        std::vector<SmallObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            objects.push_back(arena.create<SmallObject>(i));
        }
    }
    
    std::cout << "\nMedium Objects:" << std::endl;
    
    {
        Timer timer("new/delete");
        std::vector<MediumObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);

        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            objects.push_back(new MediumObject(i));
        }
        
        for (auto* obj : objects) {
            delete obj;
        }
    }
    
    {
        Timer timer("Arena allocator");
        ArenaAllocator<> arena;
        std::vector<MediumObject*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            objects.push_back(arena.create<MediumObject>(i));
        }
    }
    
    std::cout << "\nLarge Objects:" << std::endl;
    
    {
        Timer timer("new/delete");
        std::vector<LargeObject*> objects;
        objects.reserve(NUM_ALLOCATIONS / 10);
        
        for (int i = 0; i < NUM_ALLOCATIONS / 10; ++i) {
            objects.push_back(new LargeObject(100));
        }
        
        for (auto* obj : objects) {
            delete obj;
        }
    }
    
    {
        Timer timer("Arena allocator");
        ArenaAllocator<65536> arena; // Larger block size for large objects
        std::vector<LargeObject*> objects;
        objects.reserve(NUM_ALLOCATIONS / 10);
        
        for (int i = 0; i < NUM_ALLOCATIONS / 10; ++i) {
            objects.push_back(arena.create<LargeObject>(100));
        }
    }
    
    std::cout << std::endl;
}

void benchmark_fragmentation() {
    constexpr int NUM_CYCLES = 10;
    
    std::cout << "\n=== Fragmentation Benchmark ===" << std::endl;
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> dist(0, NUM_ALLOCATIONS - 1);
    
    {
        Timer timer("new/delete (with fragmentation)");
        std::vector<void*> objects(NUM_ALLOCATIONS, nullptr);
        
        for (int cycle = 0; cycle < NUM_CYCLES; ++cycle) {
            for (int i = 0; i < NUM_ALLOCATIONS / 2; ++i) {
                int index = dist(rng);
                if (!objects[index]) {
                    size_t size = 8 + (i % 32) * 8; // Varying sizes
                    objects[index] = ::operator new(size);
                }
            }
            
            for (int i = 0; i < NUM_ALLOCATIONS / 4; ++i) {
                int index = dist(rng);
                if (objects[index]) {
                    ::operator delete(objects[index]);
                    objects[index] = nullptr;
                }
            }
        }
        
        for (auto* obj : objects) {
            if (obj) ::operator delete(obj);
        }
    }

    
    {
        Timer timer("Arena allocator (with reset)");
        ArenaAllocator<> arena;
        std::vector<void*> objects(NUM_ALLOCATIONS, nullptr);
        
        for (int cycle = 0; cycle < NUM_CYCLES; ++cycle) {

            arena.reset();
            std::fill(objects.begin(), objects.end(), nullptr);
            
            // Allocate objects
            for (int i = 0; i < NUM_ALLOCATIONS / 2; ++i) {
                int index = dist(rng);
                if (!objects[index]) {
                    size_t size = 8 + (i % 32) * 8; // Varying sizes
                    objects[index] = arena.allocate(size);
                }
            }
        }
    }
    
    std::cout << std::endl;
}

void benchmark_stl_containers() {
    constexpr int NUM_ELEMENTS = 1000000;
    
    std::cout << "\n=== STL Container Benchmark (" << NUM_ELEMENTS << " elements) ===" << std::endl;
    
    {
        Timer timer("std vector<int>");
        std::vector<int> vec;
        vec.reserve(NUM_ELEMENTS);
        
        for (int i = 0; i < NUM_ELEMENTS; ++i) {
            vec.push_back(i);
        }
    }
    
    {
        Timer timer("std vector<int, StlArenaAdapter>");
        ArenaAllocator<> arena;
        std::vector<int, StlArenaAdapter<int>> vec{ StlArenaAdapter<int>(arena) };
        vec.reserve(NUM_ELEMENTS);
        
        for (int i = 0; i < NUM_ELEMENTS; ++i) {
            vec.push_back(i);
        }
    }
    
    {
        Timer timer("std vector<SmallObject>");
        std::vector<SmallObject> vec;
        vec.reserve(NUM_ELEMENTS);
        
        for (int i = 0; i < NUM_ELEMENTS; ++i) {
            vec.emplace_back(i);
        }
    }
    
    {
        Timer timer("std vector<SmallObject, StlArenaAdapter>");
        ArenaAllocator<> arena;
        std::vector<SmallObject, StlArenaAdapter<SmallObject>> vec{ StlArenaAdapter<SmallObject>(arena) };
        vec.reserve(NUM_ELEMENTS);
        
        for (int i = 0; i < NUM_ELEMENTS; ++i) {
            vec.emplace_back(i);
        }
    }
    
    std::cout << std::endl;
}

struct TreeNode {
    int value;
    std::vector<TreeNode*> children;
    
    explicit TreeNode(int v) : value(v) {}
    ~TreeNode() {
        // Standard version must clean up children
        for (auto* child : children) {
            delete child;
        }
    }
};

struct ArenaTreeNode {
    int value;
    std::vector<ArenaTreeNode*> children;
    
    explicit ArenaTreeNode(int v) : value(v) {}
};

TreeNode* build_tree_standard(int depth, int branching, int& counter) {
    auto* node = new TreeNode(counter++);
    
    if (depth > 0) {
        for (int i = 0; i < branching; ++i) {
            node->children.push_back(build_tree_standard(depth - 1, branching, counter));
        }
    }
    
    return node;
}

template<size_t DefaultBlockSize>
ArenaTreeNode* build_tree_arena(ArenaAllocator<DefaultBlockSize>& arena, int depth, int branching, int& counter) {
    auto* node = arena.create<ArenaTreeNode>(counter++);
    
    if (depth > 0) {
        for (int i = 0; i < branching; ++i) {
            node->children.push_back(build_tree_arena(arena, depth - 1, branching, counter));
        }
    }
    
    return node;
}

void benchmark_tree_building() {
    constexpr int DEPTH = 8;
    constexpr int BRANCHING = 3;
    
    std::cout << "\n=== Tree Building Benchmark (Depth: " << DEPTH 
              << ", Branching: " << BRANCHING << ") ===" << std::endl;
    
    // Calculate total nodes
    int total_nodes = 0;
    int multiplier = 1;
    for (int i = 0; i <= DEPTH; ++i) {
        total_nodes += multiplier;
        multiplier *= BRANCHING;
    }
    
    std::cout << "Total nodes: " << total_nodes << std::endl;
    
    TreeNode* std_root = nullptr;
    {
        Timer timer("Standard allocation (new/delete)");
        int counter = 0;
        std_root = build_tree_standard(DEPTH, BRANCHING, counter);
    }
    
    {
        Timer timer("Standard cleanup");
        delete std_root;
    }
    
    ArenaTreeNode* arena_root = nullptr;
    {
        Timer timer("Arena allocation");
        ArenaAllocator<65536> arena;
        int counter = 0;
        arena_root = build_tree_arena(arena, DEPTH, BRANCHING, counter);
    }
    
    std::cout << std::endl;
}

void benchmark_batch_processing() {
    constexpr int NUM_BATCHES = 100;
    constexpr int BATCH_SIZE = 10000;
    
    std::cout << "\n=== Batch Processing Benchmark (" << NUM_BATCHES
              << " batches of " << BATCH_SIZE << " objects) ===" << std::endl;
    
    {
        Timer timer("new/delete (cleanup per batch)");
        
        for (int batch = 0; batch < NUM_BATCHES; ++batch) {
            std::vector<SmallObject*> objects;
            objects.reserve(BATCH_SIZE);
            
            // Allocate batch
            for (int i = 0; i < BATCH_SIZE; ++i) {
                objects.push_back(new SmallObject(i));
            }
            
            // Process batch (simple sum to prevent optimization)
            int sum = 0;
            for (auto* obj : objects) {
                sum += obj->value;
            }
            
            // Clean up batch
            for (auto* obj : objects) {
                delete obj;
            }
        }
    }
    
    {
        Timer timer("New arena per batch");
        
        for (int batch = 0; batch < NUM_BATCHES; ++batch) {
            ArenaAllocator<> arena;
            std::vector<SmallObject*> objects;
            objects.reserve(BATCH_SIZE);
            
            // Allocate batch
            for (int i = 0; i < BATCH_SIZE; ++i) {
                objects.push_back(arena.create<SmallObject>(i));
            }
            
            // Process batch (simple sum to prevent optimization)
            int sum = 0;
            for (auto* obj : objects) {
                sum += obj->value;
            }
        }
    }
    
    {
        Timer timer("Reused arena with reset");
        ArenaAllocator<> arena;
        
        for (int batch = 0; batch < NUM_BATCHES; ++batch) {
            std::vector<SmallObject*> objects;
            objects.reserve(BATCH_SIZE);
            
            for (int i = 0; i < BATCH_SIZE; ++i) {
                objects.push_back(arena.create<SmallObject>(i));
            }
            
            // Process batch (simple sum to prevent optimization)
            int sum = 0;
            for (auto* obj : objects) {
                sum += obj->value;
            }
            
            arena.reset();
        }
    }
    
    std::cout << std::endl;
}

void benchmark_memory_usage() {
    std::cout << "\n=== Memory Usage Benchmark (" << NUM_ALLOCATIONS << " allocations) ===" << std::endl;
    
    // Track allocation sizes and statistics
    size_t total_memory_std = 0;
    size_t total_objects_std = 0;
    
    size_t total_memory_arena = 0;
    size_t total_objects_arena = 0;
    size_t total_blocks_arena = 0;
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> size_dist(8, 256);
    
    {
        Timer timer("new/delete (random sizes)");
        std::vector<void*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            size_t size = size_dist(rng);
            void* obj = ::operator new(size);
            objects.push_back(obj);
            
            total_memory_std += size;
            total_objects_std++;
        }
        
        for (auto* obj : objects) {
            ::operator delete(obj);
        }
    }
    
    {
        Timer timer("Arena (random sizes)");
        ArenaAllocator<32768> arena;
        std::vector<void*> objects;
        objects.reserve(NUM_ALLOCATIONS);
        
        for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
            size_t size = size_dist(rng);
            void* obj = arena.allocate(size);
            objects.push_back(obj);
            
            total_memory_arena += size;
            total_objects_arena++;
        }
        
        total_blocks_arena = arena.block_count();
        total_memory_arena = arena.total_allocated();
    }
    
    // Print memory usage statistics
    std::cout << "Memory Usage Statistics:" << std::endl;
    std::cout << "  Standard: " << total_objects_std << " objects, " 
              << total_memory_std << " bytes requested" << std::endl;
    std::cout << "  Arena: " << total_objects_arena << " objects, " 
              << total_memory_arena << " bytes allocated, "
              << total_blocks_arena << " blocks" << std::endl;
    
    std::cout << std::endl;
}

int main(int ac, char** av) {
    if (ac > 1)
        NUM_ALLOCATIONS = atoi(av[1]);

    std::cout << "==== Arena Allocator Performance Benchmarks ====" << std::endl;
    
    benchmark_simple_allocation();
    benchmark_object_sizes();
    benchmark_fragmentation();
    benchmark_stl_containers();
    benchmark_tree_building();
    benchmark_batch_processing();
    benchmark_memory_usage();
    
    std::cout << "==== Benchmarks Complete ====" << std::endl;
    
    return 0;
}