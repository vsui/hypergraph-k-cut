#pragma once

#include <iostream>
#include <chrono>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/certificate.hpp"

namespace hypergraph_util {

template<typename HypergraphType>
using contract_t = std::add_pointer_t<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &,
                                                                                         size_t,
                                                                                         std::mt19937_64 &,
                                                                                         typename HypergraphType::EdgeWeight)>;

template<typename HypergraphType>
using default_num_runs_t = std::add_pointer_t<size_t(const HypergraphType &, size_t)>;

/**
 * Run the contraction algorithm `Contract` on `hypergraph` until it finds a minimum cut of value `discovery_value`.
 *
 * Returns the value of the found minimum cut. Mainly useful for getting experimental data.
 *
 * @tparam HypergraphType
 * @tparam Contract the contraction algorithm to use
 * @tparam Verbose whether or not to print out intermediate values
 * @param hypergraph
 * @param k find the min-`k`-cut
 * @param discovery_value   when a cut of this value is found, the method terminates
 * @param random_generator
 * @return the value of the minimum cut
 */
template<typename HypergraphType, contract_t<HypergraphType> Contract, bool Verbose>
HypergraphCut<typename HypergraphType::EdgeWeight> run_until_discovery(const HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       typename HypergraphType::EdgeWeight discovery_value,
                                                                       std::mt19937_64 &random_generator) {
  auto min_so_far = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
  size_t i = 0;
  while (min_so_far.value > discovery_value) {
    HypergraphType copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    auto min_cut = Contract(copy, k, random_generator, /* unused */ 0);
    auto stop = std::chrono::high_resolution_clock::now();
    min_so_far = std::min(min_so_far, min_cut);

    if constexpr (Verbose) {
      std::cout << "[" << ++i << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
                << " milliseconds, got " << min_cut.value << ", min is " << min_so_far.value << ", discovery value is "
                << discovery_value << "\n";
    }
  }
  return min_so_far;

}

/**
 * A utility to repeatedly run Monte Carlo minimum cut algorithms like [CXY'18] and [FPZ'19], returning the minimum cut
 * across all runs.
 *
 * @tparam HypergraphType
 * @tparam Contract the contraction algorithm to use
 * @tparam DefaultNumRuns   the number of runs to use if `num_runs` is set to 0
 * @tparam Verbose  whether to print out intermediate cuts
 * @param hypergraph
 * @param k find the min-`k`-cut
 * @param num_runs  number of times to run the algorithm. Set to 0 to use the default for high probability of finding the minimum cut.
 * @param random_generator
 * @return the minimum cut across all runs
 */
template<
    typename HypergraphType,
    contract_t<HypergraphType> Contract,
    default_num_runs_t<HypergraphType> DefaultNumRuns,
    bool Verbose = false
>
HypergraphCut<typename HypergraphType::EdgeWeight> minimum_of_runs(const HypergraphType &hypergraph,
                                                                   size_t k,
                                                                   size_t num_runs,
                                                                   std::mt19937_64 &random_generator) {
  if (num_runs == 0) {
    num_runs = DefaultNumRuns(hypergraph, k);
  }
  if constexpr (Verbose) {
    std::cout << "Running algorithm " << num_runs << " times..." << std::endl;
  }
  auto min_so_far = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
  for (size_t i = 0; i < num_runs; ++i) {
    HypergraphType copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    auto min_cut = Contract(copy, k, random_generator, /* unused */ 0);
    auto stop = std::chrono::high_resolution_clock::now();
    min_so_far = std::min(min_so_far, min_cut);
    if (min_so_far.value == 0) {
      // We found a zero cost cut, no need to look any longer
      break;
    }
    if constexpr (Verbose) {
      std::cout << "[" << i + 1 << "/" << num_runs << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                    start)
                    .count()
                << " milliseconds, got " << min_cut.value << ", min is " << min_so_far.value
                << "\n";
    }
  }
  return min_so_far;
}
}

#define DECLARE_CONTRACTION_MIN_K_CUT(contract, default_num_runs) \
template<typename HypergraphType, bool Verbose=false, bool VVerbose=false> \
auto minimum_cut(const HypergraphType &hypergraph, size_t k, size_t num_runs = 0, uint64_t seed = 0) { \
  std::mt19937_64 rand; \
  if (seed) { rand.seed(seed); } \
  return hypergraph_util::minimum_of_runs<HypergraphType, contract<HypergraphType>, default_num_runs, Verbose>(hypergraph, k, num_runs, rand);  \
} \
template <typename HypergraphType, bool Verbose=false> \
auto discover(const HypergraphType &hypergraph, size_t k, size_t discovery_value, uint64_t seed = 0) { \
  std::mt19937_64 rand; \
  if (seed) { rand.seed(seed); } \
  return hypergraph_util::run_until_discovery<HypergraphType, contract<HypergraphType>, Verbose>(hypergraph, k, discovery_value, rand); \
}





