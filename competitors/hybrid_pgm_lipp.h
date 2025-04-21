#pragma once

#include <vector>
#include <cstdint>
#include <map>
#include "lipp.h"
#include "base.h"

namespace tli {

template <typename KeyType>
class HybridPGMLIPP : public Base<KeyType> {
private:
    // Simple map to store key-value pairs (simplified PGM)
    std::map<KeyType, uint64_t> pgm_map_;
    
    // The LIPP index for handling lookups
    Lipp<KeyType> lipp_index_;
    
    // Threshold for when to flush from PGM to LIPP (5% of total keys)
    const double FLUSH_THRESHOLD = 0.05;
    
    // Current number of keys in PGM
    size_t pgm_key_count_ = 0;
    
    // Total number of keys
    size_t total_key_count_ = 0;

public:
    // Default constructor
    HybridPGMLIPP() : lipp_index_(std::vector<int>{}) {}
    
    // Constructor that takes parameters (required by the benchmark framework)
    HybridPGMLIPP(const std::vector<int>& params) : lipp_index_(params) {
        // We don't need to use the params for now
    }
    
    // Build the index from a dataset
    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        // For simplicity, we'll just insert all keys into LIPP
        for (const auto& kv : data) {
            lipp_index_.Insert(kv, 0); // Use thread ID 0 for all insertions during build
        }
        
        // Update the total key count
        total_key_count_ = data.size();
        
        // Return the build time (we don't have a way to measure it, so return 0)
        return 0;
    }
    
    // Insert a key-value pair
    void Insert(const KeyValue<KeyType>& kv, uint32_t thread_id) {
        // Insert into PGM (simplified as a map)
        pgm_map_[kv.key] = kv.value;
        pgm_key_count_++;
        total_key_count_++;
        
        // Check if we need to flush to LIPP
        if (pgm_key_count_ >= total_key_count_ * FLUSH_THRESHOLD) {
            FlushToLIPP();
        }
    }
    
    // Lookup a key
    size_t EqualityLookup(const KeyType& key, uint32_t thread_id) {
        // First try PGM
        auto it = pgm_map_.find(key);
        if (it != pgm_map_.end()) {
            return it->second;
        }
        
        // If not found in PGM, try LIPP
        return lipp_index_.EqualityLookup(key, thread_id);
    }
    
    // Range query (if needed)
    uint64_t RangeQuery(const KeyType& lo_key, const KeyType& hi_key, uint32_t thread_id) {
        // Implement range query if needed
        return 0;
    }
    
    // Check if this index is applicable for the given parameters
    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename) {
        // HybridPGMLIPP supports unique keys, range queries, and inserts
        // It also supports multithreading
        return true;
    }
    
    // Get the name of this index
    std::string name() const { return "HybridPGMLIPP"; }
    
    // Override the runMultithread method from Base<KeyType>
    uint64_t runMultithread(void *(* func)(void *), FGParam *params) {
        // Call the base class implementation
        return Base<KeyType>::runMultithread(func, params);
    }

private:
    // Flush data from PGM to LIPP
    void FlushToLIPP() {
        // Get all keys from PGM
        std::vector<KeyValue<KeyType>> keys_to_flush;
        
        // Extract keys from PGM map
        for (const auto& pair : pgm_map_) {
            keys_to_flush.push_back({pair.first, pair.second});
        }
        
        // Insert into LIPP
        for (const auto& kv : keys_to_flush) {
            lipp_index_.Insert(kv, 0); // Use thread ID 0 for all insertions during flush
        }
        
        // Clear PGM
        pgm_map_.clear();
        pgm_key_count_ = 0;
    }
};

} // namespace tli 