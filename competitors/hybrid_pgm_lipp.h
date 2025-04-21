#pragma once

#include <vector>
#include <cstdint>
#include "lipp.h"
#include "dynamic_pgm_index.h"
#include "base.h"

template <class KeyType, class SearchClass, size_t pgm_error>
class HybridPGMLIPP : public Competitor<KeyType, SearchClass> {
public:
    HybridPGMLIPP(const std::vector<int>& params) : flush_threshold_(0.05) {}

    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        // Initially bulk load into LIPP
        std::vector<std::pair<KeyType, uint64_t>> loading_data;
        loading_data.reserve(data.size());
        for (const auto& itm : data) {
            loading_data.emplace_back(itm.key, itm.value);
        }

        total_keys_ = data.size();
        return util::timing([&] { lipp_.bulk_load(loading_data.data(), loading_data.size()); });
    }

    size_t EqualityLookup(const KeyType& lookup_key, uint32_t thread_id) const {
        // First try DPGM
        auto it = pgm_.find(lookup_key);
        if (it != pgm_.end()) {
            return it->value();
        }

        // If not found in DPGM, try LIPP
        uint64_t value;
        if (!lipp_.find(lookup_key, value)) {
            return util::NOT_FOUND;
        }
        return value;
    }

    void Insert(const KeyValue<KeyType>& data, uint32_t thread_id) {
        // Insert into DPGM
        pgm_.insert(data.key, data.value);
        total_keys_++;
        
        // Check if we need to flush
        if (pgm_.size() >= flush_threshold_ * total_keys_) {
            FlushDPGMToLIPP();
        }
    }

    uint64_t RangeQuery(const KeyType& lower_key, const KeyType& upper_key, uint32_t thread_id) const {
        // Combine results from both indexes
        uint64_t result = 0;
        
        // Query DPGM
        auto pgm_it = pgm_.lower_bound(lower_key);
        while(pgm_it != pgm_.end() && pgm_it->key() <= upper_key) {
            result += pgm_it->value();
            ++pgm_it;
        }
        
        // Query LIPP
        auto lipp_it = lipp_.lower_bound(lower_key);
        while(lipp_it != lipp_.end() && lipp_it->comp.data.key <= upper_key) {
            result += lipp_it->comp.data.value;
            ++lipp_it;
        }
        
        return result;
    }

    std::string name() const { return "HybridPGMLIPP"; }

    std::size_t size() const { 
        return pgm_.size_in_bytes() + lipp_.index_size(); 
    }

    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename) const {
        std::string name = SearchClass::name();
        // Both DPGM and LIPP require unique keys and single thread
        return unique && !multithread && name != "LinearAVX";
    }

    std::vector<std::string> variants() const { 
        std::vector<std::string> vec;
        vec.push_back(SearchClass::name());
        vec.push_back(std::to_string(pgm_error));
        return vec;
    }

private:
    void FlushDPGMToLIPP() {
        // Extract all key-value pairs from DPGM
        std::vector<std::pair<KeyType, uint64_t>> flush_data;
        for (auto it = pgm_.begin(); it != pgm_.end(); ++it) {
            flush_data.emplace_back(it->key(), it->value());
        }
        
        // Sort the data (LIPP requires sorted input)
        std::sort(flush_data.begin(), flush_data.end());
        
        // Insert into LIPP one by one (naive approach for milestone 2)
        for (const auto& kv : flush_data) {
            lipp_.insert(kv.first, kv.second);
        }
        
        // Clear DPGM
        pgm_ = decltype(pgm_)();
    }

    DynamicPGMIndex<KeyType, uint64_t, SearchClass, PGMIndex<KeyType, SearchClass, pgm_error, 16>> pgm_;
    LIPP<KeyType, uint64_t> lipp_;
    const double flush_threshold_;  // Flush when DPGM size reaches this fraction of total keys
    size_t total_keys_;  // Total number of keys in the dataset
};