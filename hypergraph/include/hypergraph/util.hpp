#pragma once

#include <iostream>
#include <chrono>

#include "hypergraph.hpp"
#include "cut.hpp"
#include "certificate.hpp"

namespace hypergraph_util {

struct ContractionStats {
  uint64_t num_contractions = 0;
  uint64_t time_elapsed_ms = 0;
  size_t num_runs = 0;
};

template<typename HypergraphType>
struct Context {
  const HypergraphType hypergraph;
  const size_t k;
  std::mt19937_64 random_generator;
  HypergraphCut<typename HypergraphType::EdgeWeight> min_so_far;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  hypergraph_util::ContractionStats stats;
  const typename HypergraphType::EdgeWeight discovery_value;
  std::optional<size_t> max_num_runs;
  std::optional<std::chrono::duration<double>> time_limit;

  Context(const HypergraphType &hypergraph,
          size_t k,
          const std::mt19937_64 &random_generator,
          typename HypergraphType::EdgeWeight discovery_value,
          std::optional<std::chrono::duration<double>> time_limit,
          std::optional<size_t> max_num_runs,
          std::chrono::time_point<std::chrono::high_resolution_clock> start)
      : hypergraph(std::move(hypergraph)), k(k), random_generator(random_generator),
        min_so_far(HypergraphCut<typename HypergraphType::EdgeWeight>::max()), stats(),
        discovery_value(discovery_value), time_limit(std::move(time_limit)), start(start),
        max_num_runs(max_num_runs) {}

  // TODO Maybe max_num_runs should be an optional
};

template<typename HypergraphType, typename ContractImpl, bool ReturnPartitions, uint8_t Verbosity>
auto repeat_contraction(typename ContractImpl::template Context<HypergraphType> &ctx) {
  size_t i = 0;
  while (ctx.min_so_far.value > ctx.discovery_value
      && (!ctx.max_num_runs.has_value() || ctx.stats.num_runs < ctx.max_num_runs.value())) {
    ++ctx.stats.num_runs;
    HypergraphType copy(ctx.hypergraph);

    auto start_run = std::chrono::high_resolution_clock::now();
    auto cut = ContractImpl::template contract<HypergraphType, ReturnPartitions, Verbosity>(ctx);
    auto stop_run = std::chrono::high_resolution_clock::now();

    if (ctx.time_limit.has_value()
        && stop_run - ctx.start > ctx.time_limit.value()) {
      // The result of the previous run ran over time, so return the result of the run before that
      if constexpr (ContractImpl::pass_discovery_value) {
        // We are using this as an FPZ flag. If FPZ then the previous algorithm probably ran over time
        // to get the value up to call stack, so be lenient and let it return the most recent call.
        ctx.min_so_far = std::min(ctx.min_so_far, cut);
      }
      if constexpr (ReturnPartitions) {
        return ctx.min_so_far;
      } else {
        return ctx.min_so_far.value;
      }
    }

    ctx.min_so_far = std::min(ctx.min_so_far, cut);

    if constexpr (Verbosity > 0) {
      std::cout << "[" << ++i << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop_run - start_run).count()
                << " milliseconds, got " << cut.value << ", min is " << ctx.min_so_far.value << ", discovery value is "
                << ctx.discovery_value << "\n";
    }
  }

  auto stop = std::chrono::high_resolution_clock::now();

  ctx.stats.time_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - ctx.start).count();

  if constexpr (ReturnPartitions) {
    return ctx.min_so_far;
  } else {
    return ctx.min_so_far.value;
  }
}

/**
 * Repeat randomized min-k-cut algorithm until either it has discovery a cut with value at least `discovery_value`
 * or has repeated a specified maximum number of times.
 *
 * Returns the minimum cut across all runs.
 */
template<typename HypergraphType, typename ContractImpl, bool ReturnPartitions, uint8_t Verbosity>
auto repeat_contraction(const HypergraphType &hypergraph,
                        size_t k,
                        std::mt19937_64 random_generator,
                        ContractionStats &stats_,
                        std::optional<size_t> max_num_runs_opt,
                        std::optional<size_t> discovery_value_opt, // TODO technically this should be the hypergraph edge weight type
                        const std::optional<std::chrono::duration<double>> &time_limit = std::nullopt) -> typename HypergraphCutRet<
    HypergraphType,
    ReturnPartitions>::T {
  // Since we are very likely to find the discovery value within `default_num_runs` runs this should not conflict
  // with discovery times.
  size_t max_num_runs = max_num_runs_opt.value_or(ContractImpl::default_num_runs(hypergraph, k));
  size_t discovery_value = discovery_value_opt.value_or(0);

  typename ContractImpl::template Context<HypergraphType> ctx(hypergraph,
                                                              k,
                                                              random_generator,
                                                              discovery_value,
                                                              time_limit,
                                                              max_num_runs,
                                                              std::chrono::high_resolution_clock::now());

  return repeat_contraction<HypergraphType, ContractImpl, ReturnPartitions, Verbosity>(ctx);
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
  static auto minimum_cut_value(const HypergraphType &hypergraph, size_t k, size_t num_runs = 0, uint64_t seed = 0) {
    std::mt19937_64 rand;
    if (seed) {
      rand.seed(seed);
    }
    hypergraph_util::ContractionStats stats{};
    return hypergraph_util::repeat_contraction<HypergraphType,
                                               ContractionImpl,
                                               false,
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
  static auto discover_value(const HypergraphType &hypergraph,
                             size_t k,
                             typename HypergraphType::EdgeWeight discovery_value,
                             uint64_t seed = 0) {
    hypergraph_util::ContractionStats stats{};
    return discover_value<HypergraphType,
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
                                                          discovery_value,
                                                          std::nullopt);
  }

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto discover_value(const HypergraphType &hypergraph,
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
                                               false,
                                               Verbosity>(hypergraph,
                                                          k,
                                                          rand,
                                                          stats,
                                                          std::nullopt,
                                                          discovery_value,
                                                          std::nullopt);
  }
};

// Makes importing the overloaded function names into a different namespace easier.
#define DECLARE_CONTRACTION_MIN_K_CUT(impl) \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto minimum_cut(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::minimum_cut<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto minimum_cut_value(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::minimum_cut_value<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto discover(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::discover<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0, typename ...Ts> auto discover_value(const HypergraphType &h, Ts&&... args) { return ContractionAlgo<impl>::discover_value<HypergraphType, Verbosity>(h, std::forward<Ts>(args)...); } \
template <typename HypergraphType, uint8_t Verbosity = 0> auto discover_stats(const HypergraphType &h, size_t k, typename HypergraphType::EdgeWeight discovery_value, hypergraph_util::ContractionStats &stats, uint64_t seed = 0) { return ContractionAlgo<impl>::discover<HypergraphType, Verbosity>(h, k, discovery_value, stats, seed); }

