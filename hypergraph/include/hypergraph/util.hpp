#pragma once

#include <iostream>
#include <chrono>

#include "hypergraph.hpp"
#include "certificate.hpp"

namespace hypergraph_util {

struct ContractionStats {
  uint64_t num_contractions;
  uint64_t time_elapsed_ms;
  size_t num_runs;
};

/**
 * Repeat randomized min-k-cut algorithm until either it has discovery a cut with value at least `discovery_value`
 * or has repeated a specified maximum number of times.
 *
 * Returns the minimum cut across all runs.
 */
template<typename HypergraphType, typename ContractImpl, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> repeat_contraction(const HypergraphType &hypergraph,
                                                                      size_t k,
                                                                      std::mt19937_64 &random_generator,
                                                                      ContractionStats &stats,
                                                                      std::optional<size_t> max_num_runs_opt,
                                                                      std::optional<size_t> discovery_value_opt) {
  size_t max_num_runs = max_num_runs_opt.value_or(100); // TODO use default num runs of algorithm
  size_t discovery_value = discovery_value_opt.value_or(0);

  stats = {};

  auto min_so_far = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
  size_t i = 0;

  auto start = std::chrono::high_resolution_clock::now();

  while (min_so_far.value > discovery_value && stats.num_runs < max_num_runs) {
    ++stats.num_runs;
    HypergraphType copy(hypergraph);
    auto cut = HypergraphCut<typename HypergraphType::EdgeWeight>::max();
    auto start = std::chrono::high_resolution_clock::now();
    if constexpr (PassDiscoveryValue) {
      cut = ContractImpl::template contract<HypergraphType, Verbosity>(copy,
                                                                       k,
                                                                       random_generator,
                                                                       stats,
                                                                       0,
                                                                       discovery_value);
    } else {
      cut = ContractImpl::template contract<HypergraphType, Verbosity>(copy, k, random_generator, stats, 0);
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

template<typename HypergraphType, typename ContractImpl, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> run_until_discovery(const HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       typename HypergraphType::EdgeWeight discovery_value,
                                                                       std::mt19937_64 &random_generator,
                                                                       ContractionStats &stats) {
  return repeat_contraction<HypergraphType, ContractImpl, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                         k,
                                                                                         random_generator,
                                                                                         stats,
                                                                                         std::nullopt,
                                                                                         discovery_value);
}

template<typename HypergraphType, typename ContractImpl, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> run_until_discovery(const HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       typename HypergraphType::EdgeWeight discovery_value,
                                                                       std::mt19937_64 &random_generator) {
  ContractionStats stats{};
  return run_until_discovery<HypergraphType, ContractImpl, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                          k,
                                                                                          discovery_value,
                                                                                          random_generator,
                                                                                          stats);
};

template<typename HypergraphType, typename ContractImpl, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> minimum_of_runs(const HypergraphType &hypergraph,
                                                                   size_t k,
                                                                   size_t max_num_runs,
                                                                   std::mt19937_64 &random_generator,
                                                                   ContractionStats &stats) {
  if (max_num_runs == 0) {
    max_num_runs = ContractImpl::template default_num_runs<HypergraphType>(hypergraph, k);
  }
  return repeat_contraction<HypergraphType, ContractImpl, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                         k,
                                                                                         random_generator,
                                                                                         stats,
                                                                                         max_num_runs,
                                                                                         std::nullopt);
}

template<typename HypergraphType, typename ContractImpl, uint8_t Verbosity, bool PassDiscoveryValue = false>
HypergraphCut<typename HypergraphType::EdgeWeight> minimum_of_runs(const HypergraphType &hypergraph,
                                                                   size_t k,
                                                                   size_t num_runs,
                                                                   std::mt19937_64 &random_generator) {
  ContractionStats stats{};
  return minimum_of_runs<HypergraphType, ContractImpl, Verbosity, PassDiscoveryValue>(hypergraph,
                                                                                      k,
                                                                                      num_runs,
                                                                                      random_generator,
                                                                                      stats);
}

}

template<typename ContractionImpl>
struct ContractionAlgo {
  static constexpr bool PassDiscovery = ContractionImpl::pass_discovery_value;

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto minimum_cut(const HypergraphType &hypergraph, size_t k, size_t num_runs = 0, uint64_t seed = 0) {
    std::mt19937_64 rand;
    if (seed) {
      rand.seed(seed);
    }
    return hypergraph_util::minimum_of_runs<HypergraphType,
                                            ContractionImpl,
                                            Verbosity,
                                            PassDiscovery>(hypergraph, k, num_runs, rand);
  }

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto discover(const HypergraphType &hypergraph,
                       size_t k,
                       typename HypergraphType::EdgeWeight discovery_value,
                       uint64_t seed = 0) {
    hypergraph_util::ContractionStats stats{};
    return discover<HypergraphType,
                    Verbosity>(hypergraph, k, discovery_value, stats, seed);
  }

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto discover(const HypergraphType &hypergraph,
                       size_t k,
                       typename HypergraphType::EdgeWeight discovery_value,
                       hypergraph_util::ContractionStats &stats,
                       uint64_t seed = 0) {
    std::mt19937_64 rand;
    stats = {};
    if (seed) {
      rand.seed(seed);
    }
    return hypergraph_util::run_until_discovery<HypergraphType,
                                                ContractionImpl,
                                                Verbosity,
                                                PassDiscovery>(hypergraph, k, discovery_value, rand, stats);
  }
};

// Makes importing the overloaded function names into a different namespace easier.
#define DECLARE_CONTRACTION_MIN_K_CUT(impl) \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto minimum_cut(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::minimum_cut<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto discover(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::discover<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0> auto discover_stats(const HypergraphType &h, size_t k, typename HypergraphType::EdgeWeight discovery_value, hypergraph_util::ContractionStats &stats, uint64_t seed = 0) { return ContractionAlgo<impl>::discover<HypergraphType, Verbosity>(h, k, discovery_value, stats, seed); }

