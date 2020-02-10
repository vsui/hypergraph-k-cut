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
 * @tparam Verbosity
 * @tparam PassDiscoveryValue whether or not to pass the discovery value to the contraction algorithm (for FPZ)
 * @param hypergraph
 * @param k find the min-`k`-cut
 * @param discovery_value   when a cut of this value is found, the method terminates
 * @param random_generator
 * @return the value of the minimum cut
 */
template<typename HypergraphType, auto Contract, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> run_until_discovery(const HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       typename HypergraphType::EdgeWeight discovery_value,
                                                                       std::mt19937_64 &random_generator) {
  auto min_so_far = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
  size_t i = 0;
  while (min_so_far.value > discovery_value) {
    HypergraphType copy(hypergraph);
    auto cut = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
    auto start = std::chrono::high_resolution_clock::now();
    if constexpr (PassDiscoveryValue) {
      cut = Contract(copy, k, random_generator, 0, discovery_value);
    } else {
      cut = Contract(copy, k, random_generator, 0);
    }
    auto stop = std::chrono::high_resolution_clock::now();
    min_so_far = std::min(min_so_far, cut);

    if constexpr (Verbosity > 0) {
      std::cout << "[" << ++i << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
                << " milliseconds, got " << cut.value << ", min is " << min_so_far.value << ", discovery value is "
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
 * @tparam Verbosity
 * @param hypergraph
 * @param k find the min-`k`-cut
 * @param num_runs  number of times to run the algorithm. Set to 0 to use the default for high probability of finding the minimum cut.
 * @param random_generator
 * @return the minimum cut across all runs
 */
template<typename HypergraphType, auto Contract, default_num_runs_t<HypergraphType> DefaultNumRuns, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> minimum_of_runs(const HypergraphType &hypergraph,
                                                                   size_t k,
                                                                   size_t num_runs,
                                                                   std::mt19937_64 &random_generator) {
  if (num_runs == 0) {
    num_runs = DefaultNumRuns(hypergraph, k);
  }
  if constexpr (Verbosity > 0) {
    std::cout << "Running algorithm " << num_runs << " times..." << std::endl;
  }
  auto min_so_far = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
  for (size_t i = 0; i < num_runs; ++i) {
    HypergraphType copy(hypergraph);
    auto cut = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
    auto start = std::chrono::high_resolution_clock::now();
    if constexpr (PassDiscoveryValue) {
      cut = Contract(copy, k, random_generator, /* unused */ 0, 0);
    } else {
      cut = Contract(copy, k, random_generator, /* unused */ 0);
    }
    auto stop = std::chrono::high_resolution_clock::now();
    min_so_far = std::min(min_so_far, cut);
    if (min_so_far.value == 0) {
      // We found a zero cost cut, no need to look any longer
      break;
    }
    if constexpr (Verbosity > 0) {
      std::cout << "[" << i + 1 << "/" << num_runs << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                    start)
                    .count()
                << " milliseconds, got " << cut.value << ", min is " << min_so_far.value
                << "\n";
    }
  }
  return min_so_far;
}
}

#define DECLARE_CONTRACTION_MIN_K_CUT(contract, default_num_runs, pass_discovery) \
template<typename HypergraphType, uint8_t Verbosity=0> \
auto minimum_cut(const HypergraphType &hypergraph, size_t k, size_t num_runs = 0, uint64_t seed = 0) { \
  std::mt19937_64 rand; \
  if (seed) { rand.seed(seed); } \
  return hypergraph_util::minimum_of_runs<HypergraphType, contract<HypergraphType, Verbosity>, default_num_runs, Verbosity, pass_discovery>(hypergraph, k, num_runs, rand);  \
} \
template <typename HypergraphType, uint8_t Verbosity=0> \
auto discover(const HypergraphType &hypergraph, size_t k, size_t discovery_value, uint64_t seed = 0) { \
  std::mt19937_64 rand; \
  if (seed) { rand.seed(seed); } \
  return hypergraph_util::run_until_discovery<HypergraphType, contract<HypergraphType, Verbosity>, Verbosity, pass_discovery>(hypergraph, k, discovery_value, rand); \
}





