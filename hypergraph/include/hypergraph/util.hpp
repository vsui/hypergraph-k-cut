#pragma once

#include <iostream>
#include <chrono>

#include "hypergraph.hpp"
#include "certificate.hpp"

namespace hypergraph_util {

template<typename HypergraphType, bool ReturnPartitions>
struct HypergraphCutRet;

template<typename HypergraphType>
struct HypergraphCutRet<HypergraphType, true> {
  using T = HypergraphCut<typename HypergraphType::EdgeWeight>;
};

template<typename HypergraphType>
struct HypergraphCutRet<HypergraphType, false> {
  using T = typename HypergraphType::EdgeWeight; // Just return cut value
};

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
template<typename HypergraphType, typename ContractImpl, bool ReturnPartitions, uint8_t Verbosity>
auto repeat_contraction(const HypergraphType &hypergraph,
                        size_t k,
                        std::mt19937_64 &random_generator,
                        ContractionStats &stats,
                        std::optional<size_t> max_num_runs_opt,
                        std::optional<size_t> discovery_value_opt) -> typename HypergraphCutRet<HypergraphType,
                                                                                                ReturnPartitions>::T {
  // Since we are very likely to find the discovery value within `default_num_runs` runs this should not conflict
  // with discovery times.
  size_t max_num_runs = max_num_runs_opt.value_or(ContractImpl::default_num_runs(hypergraph, k));
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
    if constexpr (ContractImpl::pass_discovery_value) {
      cut = ContractImpl::template contract<HypergraphType, ReturnPartitions, Verbosity>(copy,
                                                                                         k,
                                                                                         random_generator,
                                                                                         stats,
                                                                                         0,
                                                                                         discovery_value);
    } else {
      cut = ContractImpl::template contract<HypergraphType, ReturnPartitions, Verbosity>(copy,
                                                                                         k,
                                                                                         random_generator,
                                                                                         stats,
                                                                                         0);
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

}

template<typename ContractionImpl>
struct ContractionAlgo {
  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto minimum_cut(const HypergraphType &hypergraph, size_t k, size_t num_runs = 0, uint64_t seed = 0) {
    std::mt19937_64 rand;
    if (seed) {
      rand.seed(seed);
    }
    hypergraph_util::ContractionStats stats{};
    return hypergraph_util::repeat_contraction<HypergraphType,
                                               ContractionImpl,
                                               true,
                                               Verbosity>(hypergraph,
                                                          k,
                                                          rand,
                                                          stats,
                                                          num_runs == 0 ? std::nullopt : std::optional(num_runs),
                                                          std::nullopt);
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
    return hypergraph_util::repeat_contraction<HypergraphType,
                                               ContractionImpl,
                                               true,
                                               Verbosity>(hypergraph,
                                                          k,
                                                          rand,
                                                          stats,
                                                          std::nullopt,
                                                          discovery_value);
  }
};

// Makes importing the overloaded function names into a different namespace easier.
#define DECLARE_CONTRACTION_MIN_K_CUT(impl) \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto minimum_cut(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::minimum_cut<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto discover(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::discover<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0> auto discover_stats(const HypergraphType &h, size_t k, typename HypergraphType::EdgeWeight discovery_value, hypergraph_util::ContractionStats &stats, uint64_t seed = 0) { return ContractionAlgo<impl>::discover<HypergraphType, Verbosity>(h, k, discovery_value, stats, seed); }

