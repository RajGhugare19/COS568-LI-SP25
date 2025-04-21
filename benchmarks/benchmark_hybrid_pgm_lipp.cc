#include "benchmarks/benchmark_hybrid_pgm_lipp.h"

#include "benchmark.h"
#include "common.h"
#include "competitors/hybrid_pgm_lipp.h"

namespace tli {

void benchmark_64_hybrid_pgm_lipp(Benchmark<uint64_t>& benchmark) {
  benchmark.template Run<HybridPGMLIPP<uint64_t>>();
}

} // namespace tli 