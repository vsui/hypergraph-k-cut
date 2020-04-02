// Branching contraction algorithm from [FPZ'19]
#pragma once

#include <cmath>
#include <iostream>
#include <random>
#include <cassert>

#include "hypergraph.hpp"
#include "cxy.hpp"

namespace fpz {

/**
 * Redo probability from the branching contraction paper. The calculations have been changed to use log calculation to
 * avoid overflow.
 *
 * @param n number of vertices in the hypergraph
 * @param e number of vertices that the hyperedge is incident on
 * @param k number of partitions
 * @return the redo probability
 */
inline double redo_probability(size_t n, size_t e, size_t k) {
  return 1 - cxy::cxy_delta(n, e, k);
}

struct FpzImpl {
  static constexpr bool pass_discovery_value = true;

  /**
   * Stack context to represent different branches of computation
   */
  template<typename HypergraphType>
  struct LocalContext {
    HypergraphType hypergraph;
    typename HypergraphType::EdgeWeight accumulated;
  };

  template<typename HypergraphType>
  struct Context {
    const HypergraphType hypergraph;
    const size_t k;
    std::mt19937_64 random_generator;
    HypergraphCut<typename HypergraphType::EdgeWeight> min_so_far;
    const std::chrono::time_point<std::chrono::high_resolution_clock> start;

    // FPZ specific
    hypergraph_util::ContractionStats stats;
    const typename HypergraphType::EdgeWeight discovery_value;
    const std::optional<size_t> time_limit_ms_opt;
    std::deque<LocalContext<HypergraphType>> branches;

    Context(const HypergraphType &hypergraph,
            size_t k,
            const std::mt19937_64 &random_generator,
            typename HypergraphType::EdgeWeight discovery_value,
            std::optional<size_t> time_limit_ms_opt,
            std::chrono::time_point<std::chrono::high_resolution_clock> start)
        : hypergraph(std::move(hypergraph)), k(k), random_generator(std::move(random_generator)),
          min_so_far(HypergraphCut<typename HypergraphType::EdgeWeight>::max()), stats(),
          discovery_value(discovery_value), time_limit_ms_opt(time_limit_ms_opt), start(start), branches() {}
  };

/**
 * The randomized branching contraction algorithm from [FPZ'19] that returns the minimum-k-cut of a hypergraph with some
 * probability.
 *
 * @tparam HypergraphType
 * @tparam Verbosity
 * @param hypergraph
 * @param k compute the min-`k`-cut
 * @param random_generator  the source of randomness
 * @param discovery_value   this function will early-exit if a cut of this value is found.
 * @return minimum found k cut
 */
  template<typename HypergraphType, bool ReturnPartitions, uint8_t Verbosity>
  static HypergraphCut<typename HypergraphType::EdgeWeight> contract(Context<HypergraphType> &ctx) {

    ctx.branches.push_back({.hypergraph = ctx.hypergraph, .accumulated = 0});

    while (!ctx.branches.empty()) {
      auto &local_ctx = ctx.branches.front();

      // Call internal function with global context. This will update it.
      contract_<HypergraphType, ReturnPartitions, Verbosity>(ctx, local_ctx);

      if (ctx.min_so_far.value <= ctx.discovery_value) {
        break;
      }

      ctx.branches.pop_front();
    }

    return ctx.min_so_far;
  }

  /**
 * Calculate the number of runs required to find the minimum k-cut with high probability.
 *
 * @tparam HypergraphType
 * @param hypergraph
 * @param k
 * @return the number of runs required to find the minimum cut with high probability
 */
  template<typename HypergraphType>
  static size_t default_num_runs(const HypergraphType &hypergraph, [[maybe_unused]] size_t k) {
    auto log_n =
        static_cast<size_t>(std::ceil(std::log(hypergraph.num_vertices())));
    return log_n * log_n;
  }

  template<typename HypergraphType, bool ReturnPartitions, uint8_t Verbosity>
  static void contract_(Context<HypergraphType> &global_ctx,
                        LocalContext<HypergraphType> &local_ctx) {
    auto &[_, k, random_generator, min_so_far, start, stats, discovery_value, time_limit_ms_opt, branches] = global_ctx;
    auto &[hypergraph, accumulated] = local_ctx;

    // Remove k-spanning hyperedges from hypergraph
    std::vector<int> k_spanning_hyperedges;
    for (const auto &[edge_id, vertices] : hypergraph.edges()) {
      // TODO overflow here?
      if (vertices.size() >= hypergraph.num_vertices() - k + 2) {
        k_spanning_hyperedges.push_back(edge_id);
        accumulated += edge_weight(hypergraph, edge_id);
      }
    }
    for (const auto edge_id : k_spanning_hyperedges) {
      hypergraph.remove_hyperedge(edge_id);
    }

    // If no edges remain, return the answer
    if (hypergraph.num_edges() == 0) {
      // May terminate early if it finds a zero cost cut with >k partitions, so need
      // to merge partitions.
      while (hypergraph.num_vertices() > k) {
        auto begin = std::begin(hypergraph.vertices());
        auto end = std::begin(hypergraph.vertices());
        std::advance(end, 2);
        // Contract two vertices
        hypergraph = hypergraph.template contract<true, ReturnPartitions>(begin, end);
        ++stats.num_contractions;
      }

      if constexpr (ReturnPartitions) {
        std::vector<std::vector<int>> partitions;
        for (const auto v : hypergraph.vertices()) {
          const auto &partition = hypergraph.vertices_within(v);
          partitions.emplace_back(std::begin(partition), std::end(partition));
        }
        const auto cut =
            HypergraphCut<typename HypergraphType::EdgeWeight>(std::begin(partitions),
                                                               std::end(partitions),
                                                               accumulated);
        if constexpr (Verbosity > 1) {
          std::cout << "Got cut of value " << cut.value << std::endl;
        }
        min_so_far = std::min(min_so_far, cut);
        return;
      } else {
        if constexpr (Verbosity > 1) {
          std::cout << "Got cut of value " << accumulated << std::endl;
        }
        min_so_far = std::min(min_so_far, HypergraphCut<typename HypergraphType::EdgeWeight>{accumulated});
        return;
      }
    }

    static std::uniform_real_distribution<> dis(0.0, 1.0);

    // Select a hyperedge with probability proportional to its weight
    std::vector<int> edge_ids;
    std::vector<Hypergraph::EdgeWeight> edge_weights;
    for (const auto &[edge_id, vertices] : hypergraph.edges()) {
      edge_ids.push_back(edge_id);
      edge_weights.push_back(edge_weight(hypergraph, edge_id));
    }
    std::discrete_distribution<size_t> distribution(std::begin(edge_weights), std::end(edge_weights));
    const auto sampled_edge_id = edge_ids.at(distribution(random_generator));
    const auto sampled_edge = hypergraph.edges().at(sampled_edge_id);

    double redo =
        redo_probability(hypergraph.num_vertices(), sampled_edge.size(), k);

    HypergraphType contracted = hypergraph.template contract<true, ReturnPartitions>(sampled_edge_id);
    ++stats.num_contractions;

    if (dis(random_generator) < redo) {
      branches.push_back({.hypergraph = contracted, .accumulated = accumulated});
      branches.push_back(local_ctx);
    } else {
      branches.push_back({.hypergraph = contracted, .accumulated = accumulated});
    }
  }

};

DECLARE_CONTRACTION_MIN_K_CUT(FpzImpl);

} // namespace fpz
