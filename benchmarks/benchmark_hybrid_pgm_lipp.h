#pragma once

#include <string>
#include <vector>

#include "../competitors/hybrid_pgm_lipp.h"
#include "../searches/branching_binary_search.hpp"
#include "benchmark_base.h"

template <typename KeyType>
class BenchmarkHybridPGMLIPP : public BenchmarkBase<KeyType> {
 public:
  BenchmarkHybridPGMLIPP(const std::string& data_filename,
                         const std::string& ops_filename,
                         const std::string& bulk_filename)
      : BenchmarkBase<KeyType>(data_filename, ops_filename, bulk_filename) {}

 protected:
  std::vector<std::unique_ptr<BaseIndex<KeyType>>> get_algorithms(
      const std::vector<int>& params) {
    std::vector<std::unique_ptr<BaseIndex<KeyType>>> algorithms;
    algorithms.emplace_back(
        std::make_unique<HybridPGMLIPP<KeyType, BranchingBinarySearch<KeyType>, 64>>(
            params));
    return algorithms;
  }
};