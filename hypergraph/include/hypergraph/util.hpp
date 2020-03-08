#pragma once

#include <iostream>
#include <chrono>

#include "hypergraph.hpp"
#include "certificate.hpp"

namespace hypergraph_util {

struct ContractionStats {
  uint64_t num_contractions;
  uint64_t time_elapsed_ms;
};

template<typename HypergraphType>
using contract_t = std::add_pointer_t<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &,
                                                                                         size_t,
                                                                                         std::mt19937_64 &,
                                                                                         typename HypergraphType::EdgeWeight)>;

template<typename HypergraphType>
using default_num_runs_t = std::add_pointer_t<size_t(const HypergraphType &, size_t)>;

/**
 * Repeat randomized min-k-cut algorithm until either it has discovery a cut with value at least `discovery_value`
 * or has repeated a specified maximum number of times.
 *
 * Returns the minimum cut across all runs.
 */
template<typename HypergraphType, auto Contract, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> repeat_contraction(const HypergraphType &hypergraph,
                                                                      size_t k,
                                                                      std::mt19937_64 &random_generator,
                                                                      size_t &num_runs,
                                                                      ContractionStats &stats,
                                                                      std::optional<size_t> max_num_runs_opt,
                                                                      std::optional<size_t> discovery_value_opt) {
  size_t max_num_runs = max_num_runs_opt.value_or(100); // TODO use default num runs of algorithm
  size_t discovery_value = discovery_value_opt.value_or(0);

  stats = {};

  num_runs = 0;
  auto min_so_far = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
  size_t i = 0;

  auto start = std::chrono::high_resolution_clock::now();

  while (min_so_far.value > discovery_value && num_runs < max_num_runs) {
    ++num_runs;
    HypergraphType copy(hypergraph);
    auto cut = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
    auto start = std::chrono::high_resolution_clock::now();
    if constexpr (PassDiscoveryValue) {
      cut = Contract(copy, k, random_generator, stats, 0, discovery_value);
    } else {
      cut = Contract(copy, k, random_generator, stats, 0);
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

  auto stop = std::chrono::high_resolution_clock::now();

  stats.time_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

  return min_so_far;
}

template<typename HypergraphType, auto Contract, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> run_until_discovery(const HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       typename HypergraphType::EdgeWeight discovery_value,
                                                                       std::mt19937_64 &random_generator,
                                                                       size_t &num_runs,
                                                                       ContractionStats &stats) {
  return repeat_contraction<HypergraphType, Contract, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                     k,
                                                                                     random_generator,
                                                                                     num_runs,
                                                                                     stats,
                                                                                     std::nullopt,
                                                                                     discovery_value);
}

template<typename HypergraphType, auto Contract, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> run_until_discovery(const HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       typename HypergraphType::EdgeWeight discovery_value,
                                                                       std::mt19937_64 &random_generator,
                                                                       size_t &num_runs) {
  ContractionStats stats{};
  return run_until_discovery<HypergraphType, Contract, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                      k,
                                                                                      discovery_value,
                                                                                      random_generator,
                                                                                      num_runs,
                                                                                      stats);
};

template<typename HypergraphType, auto Contract, default_num_runs_t<HypergraphType> DefaultNumRuns, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> minimum_of_runs(const HypergraphType &hypergraph,
                                                                   size_t k,
                                                                   size_t max_num_runs,
                                                                   std::mt19937_64 &random_generator,
                                                                   ContractionStats &stats) {
  if (max_num_runs == 0) {
    max_num_runs = DefaultNumRuns(hypergraph, k);
  }
  size_t num_runs{};
  return repeat_contraction<HypergraphType, Contract, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                     k,
                                                                                     random_generator,
                                                                                     num_runs,
                                                                                     stats,
                                                                                     max_num_runs,
                                                                                     std::nullopt);
}

template<typename HypergraphType, auto Contract, default_num_runs_t<HypergraphType> DefaultNumRuns, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> minimum_of_runs(const HypergraphType &hypergraph,
                                                                   size_t k,
                                                                   size_t num_runs,
                                                                   std::mt19937_64 &random_generator) {
  ContractionStats stats{};
  return minimum_of_runs<HypergraphType, Contract, DefaultNumRuns, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                                  k,
                                                                                                  num_runs,
                                                                                                  random_generator,
                                                                                                  stats);
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
auto discover(const HypergraphType &hypergraph, size_t k, size_t discovery_value, size_t &num_runs, uint64_t seed = 0) { \
  std::mt19937_64 rand; \
  if (seed) { rand.seed(seed); } \
  return hypergraph_util::run_until_discovery<HypergraphType, contract<HypergraphType, Verbosity>, Verbosity, pass_discovery>(hypergraph, k, discovery_value, rand, num_runs); \
} \
template <typename HypergraphType, uint8_t Verbosity=0> \
auto discover(const HypergraphType &hypergraph, size_t k, size_t discovery_value, size_t &num_runs, hypergraph_util::ContractionStats &stats, uint64_t seed = 0) { \
  std::mt19937_64 rand; \
  stats = {}; \
  if (seed) { rand.seed(seed); } \
  return hypergraph_util::run_until_discovery<HypergraphType, contract<HypergraphType, Verbosity>, Verbosity, pass_discovery>(hypergraph, k, discovery_value, rand, num_runs, stats); \
}





