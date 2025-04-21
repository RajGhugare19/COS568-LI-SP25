#pragma once

#include <vector>
#include <cstdint>
#include "PGM-index/include/pgm_index_dynamic.hpp"
#include "lipp.h"
#include "../searches/linear_search.h"

namespace tli {

// Adapter for PGM-index to use the project's LinearSearch class
template<typename K>
struct LinearSearch {
    template<typename Iterator>
    static Iterator upper_bound(Iterator first, Iterator last, const K& key, Iterator hint, std::function<K(Iterator)> get_key) {
        // Simple linear search implementation
        Iterator it = first;
        while (it != last && get_key(it) <= key) {
            ++it;
        }
        return it;
    }
};

template <typename KeyType>
class HybridPGMLIPP {
private:
    // The Dynamic PGM index for handling insertions
    DynamicPGMIndex<KeyType, uint64_t, LinearSearch<KeyType>> pgm_index_;
    
    // The LIPP index for handling lookups
    LIPP<KeyType> lipp_index_;
    
    // Threshold for when to flush from PGM to LIPP (5% of total keys)
    const double FLUSH_THRESHOLD = 0.05;
    
    // Current number of keys in PGM
    size_t pgm_key_count_ = 0;
    
    // Total number of keys
    size_t total_key_count_ = 0;

public:
    HybridPGMLIPP() = default;
    
    // Insert a key-value pair
    void Insert(const KeyValue<KeyType>& kv, uint32_t thread_id) {
        // Insert into PGM
        pgm_index_.Insert(kv, thread_id);
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
        size_t result = pgm_index_.EqualityLookup(key, thread_id);
        if (result != util::NOT_FOUND) {
            return result;
        }
        
        // If not found in PGM, try LIPP
        return lipp_index_.EqualityLookup(key, thread_id);
    }
    
    // Range query (if needed)
    uint64_t RangeQuery(const KeyType& lo_key, const KeyType& hi_key, uint32_t thread_id) {
        // Implement range query if needed
        return 0;
    }

private:
    // Flush data from PGM to LIPP
    void FlushToLIPP() {
        // Get all keys from PGM
        std::vector<KeyValue<KeyType>> keys_to_flush;
        
        // Extract keys from PGM
        // Since PGM doesn't provide a direct way to get all keys,
        // we need to use a workaround
        // For simplicity, we'll use a binary search approach to find keys
        // This is not efficient but works for the basic implementation
        
        // Create a temporary vector to store keys
        std::vector<KeyType> temp_keys;
        
        // Use the PGM iterator to get all keys
        for (auto it = pgm_index_.begin(); it != pgm_index_.end(); ++it) {
            keys_to_flush.push_back({it->first, it->second});
        }
        
        // Insert into LIPP
        for (const auto& kv : keys_to_flush) {
            lipp_index_.Insert(kv);
        }
        
        // Clear PGM
        pgm_index_ = DynamicPGMIndex<KeyType, uint64_t, LinearSearch<KeyType>>();
        pgm_key_count_ = 0;
    }
};

} // namespace tli 