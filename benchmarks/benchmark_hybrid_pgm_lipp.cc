#include <iostream>

#include "benchmark_hybrid_pgm_lipp.h"

int main(int argc, char* argv[]) {
  if (argc != 4) {
    std::cout << "Usage: " << argv[0]
              << " <data_file> <operations_file> <bulkload_file>" << std::endl;
    return 1;
  }
  std::string data_file = argv[1];
  std::string ops_file = argv[2];
  std::string bulk_file = argv[3];

  if (util::get_file_type(data_file) == util::FileType::UINT64) {
    BenchmarkHybridPGMLIPP<uint64_t> benchmark(data_file, ops_file, bulk_file);
    benchmark.Run();
  } else {
    std::cerr << "Unsupported key type" << std::endl;
    return 1;
  }
  return 0;
}