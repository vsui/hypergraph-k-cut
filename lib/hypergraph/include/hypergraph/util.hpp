#ifndef HYPERGRAPH_UTIL
#define HYPERGRAPH_UTIL

#include <iostream>
#include <chrono>
#include <random>

#include "hypergraph.hpp"
#include "cut.hpp"
#include "certificate.hpp"

namespace hypergraphlib {

namespace util {

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
  // So cutoff experiments can introspect the minimum value so far at various cutoff points. Only one thread should
  // write to this
  std::atomic<typename HypergraphType::EdgeWeight> min_val_so_far;
  util::ContractionStats stats;
  const typename HypergraphType::EdgeWeight discovery_value;
  std::optional<size_t> max_num_runs;

  Context(const HypergraphType &hypergraph,
          size_t k,
          const std::mt19937_64 &random_generator,
          typename HypergraphType::EdgeWeight discovery_value,
          std::optional<size_t> max_num_runs)
      : hypergraph(std::move(hypergraph)), k(k), random_generator(random_generator),
        min_so_far(HypergraphCut<typename HypergraphType::EdgeWeight>::max()), min_val_so_far(min_so_far.value),
        stats(), discovery_value(discovery_value),
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

    ctx.min_so_far = std::min(ctx.min_so_far, cut);
    ctx.min_val_so_far.store(ctx.min_so_far.value);

    if constexpr (Verbosity > 0) {
      std::cout << "[" << ++i << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop_run - start_run).count()
                << " milliseconds, got " << cut.value << ", min is " << ctx.min_so_far.value << ", discovery value is "
                << ctx.discovery_value << "\n";
    }
  }

  auto stop = std::chrono::high_resolution_clock::now();

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
                                                              max_num_runs);

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
    util::ContractionStats stats{};
    return util::repeat_contraction<HypergraphType,
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
    util::ContractionStats stats{};
    return util::repeat_contraction<HypergraphType,
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
    util::ContractionStats stats{};
    return discover<HypergraphType,
                    Verbosity>(hypergraph, k, discovery_value, stats, seed);
  }

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto discover_value(const HypergraphType &hypergraph,
                             size_t k,
                             typename HypergraphType::EdgeWeight discovery_value,
                             uint64_t seed = 0) {
    util::ContractionStats stats{};
    return discover_value<HypergraphType,
                          Verbosity>(hypergraph, k, discovery_value, stats, seed);
  }

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto discover(const HypergraphType &hypergraph,
                       size_t k,
                       typename HypergraphType::EdgeWeight discovery_value,
                       util::ContractionStats &stats,
                       uint64_t seed = 0) {
    std::mt19937_64 rand;
    stats = {};
    if (seed) {
      rand.seed(seed);
    }
    return util::repeat_contraction<HypergraphType,
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
                             util::ContractionStats &stats,
                             uint64_t seed = 0) {
    std::mt19937_64 rand;
    stats = {};
    if (seed) {
      rand.seed(seed);
    }
    return util::repeat_contraction<HypergraphType,
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

  template<typename HypergraphType, uint8_t Verbosity = 0>
  static auto discover_stats(const HypergraphType &h,
                             size_t k,
                             typename HypergraphType::EdgeWeight discovery_value,
                             util::ContractionStats &stats,
                             uint64_t seed = 0) {
    return ContractionAlgo<ContractionImpl>::discover<HypergraphType, Verbosity>(h, k, discovery_value, stats, seed);
  }
};

}

#endif
