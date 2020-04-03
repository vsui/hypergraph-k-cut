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

  static constexpr char name[] = "FPZ";

  /**
   * Stack context to represent different branches of computation
   */
  template<typename HypergraphType>
  struct LocalContext {
    HypergraphType hypergraph;
    typename HypergraphType::EdgeWeight accumulated;
  };

  template<typename HypergraphType>
  struct Context : public hypergraph_util::Context<HypergraphType> {
    std::deque<LocalContext<HypergraphType>> branches;

    Context(const HypergraphType &hypergraph,
            size_t k,
            const std::mt19937_64 &random_generator,
            typename HypergraphType::EdgeWeight discovery_value,
            std::optional<std::chrono::duration<double>> time_limit,
            std::optional<size_t> max_num_runs,
            std::chrono::time_point<std::chrono::high_resolution_clock> start)
        : hypergraph_util::Context<HypergraphType>(hypergraph,
                                                   k,
                                                   random_generator,
                                                   discovery_value,
                                                   time_limit,
                                                   max_num_runs,
                                                   start),
          branches() {}
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
      auto local_ctx = std::move(ctx.branches.back());
      ctx.branches.pop_back();

      // Call internal function with global context. This will update it.
      contract_<HypergraphType, ReturnPartitions, Verbosity>(ctx, local_ctx);

      if (ctx.time_limit.has_value()
          && std::chrono::high_resolution_clock::now() - ctx.start > ctx.time_limit.value()) {
        break;
      }

      if (ctx.min_so_far.value <= ctx.discovery_value) {
        break;
      }
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
  static void contract_(Context<HypergraphType> &ctx,
                        LocalContext<HypergraphType> &local_ctx) {
    auto &[hypergraph, accumulated] = local_ctx;

    // Remove k-spanning hyperedges from hypergraph
    std::vector<int> k_spanning_hyperedges;
    for (const auto &[edge_id, vertices] : hypergraph.edges()) {
      // TODO overflow here?
      if (vertices.size() >= hypergraph.num_vertices() - ctx.k + 2) {
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
      while (hypergraph.num_vertices() > ctx.k) {
        auto begin = std::begin(hypergraph.vertices());
        auto end = std::begin(hypergraph.vertices());
        std::advance(end, 2);
        // Contract two vertices
        hypergraph = hypergraph.template contract<true, ReturnPartitions>(begin, end);
        ++ctx.stats.num_contractions;
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
        ctx.min_so_far = std::min(ctx.min_so_far, cut);
        return;
      } else {
        if constexpr (Verbosity > 1) {
          std::cout << "Got cut of value " << accumulated << std::endl;
        }
        ctx.min_so_far =
            std::min(ctx.min_so_far, HypergraphCut<typename HypergraphType::EdgeWeight>{accumulated});
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
    const auto sampled_edge_id = edge_ids.at(distribution(ctx.random_generator));
    const auto sampled_edge = hypergraph.edges().at(sampled_edge_id);

    double redo =
        redo_probability(hypergraph.num_vertices(), sampled_edge.size(), ctx.k);

    HypergraphType contracted = hypergraph.template contract<true, ReturnPartitions>(sampled_edge_id);
    ++ctx.stats.num_contractions;

    if (dis(ctx.random_generator) < redo) {
      ctx.branches.push_back(local_ctx);
      ctx.branches.push_back({.hypergraph = contracted, .accumulated = accumulated});
    } else {
      ctx.branches.push_back({.hypergraph = contracted, .accumulated = accumulated});
    }
  }

};

DECLARE_CONTRACTION_MIN_K_CUT(FpzImpl);

} // namespace fpz
