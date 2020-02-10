// Branching contraction algorithm from [FPZ'19]
#pragma once

#include <cmath>
#include <iostream>
#include <random>
#include <cassert>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/cxy.hpp"

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

/**
 * The randomized branching contraction algorithm from [FPZ'19] that returns the minimum-k-cut of a hypergraph with some
 * probability.
 *
 * @tparam HypergraphType
 * @tparam Verbose if `true` then print out stats for each leaf calculation
 * @param hypergraph
 * @param k
 * @param random_generator the source of randomness
 * @param accumulated the size of the hypergraph so far (used as context for recursive calls)
 * @return minimum found k cut
 */
template<typename HypergraphType, bool Verbose = false>
HypergraphCut<typename HypergraphType::EdgeWeight> branching_contract_(HypergraphType &hypergraph,
                                                                       size_t k,
                                                                       std::mt19937_64 &random_generator,
                                                                       typename HypergraphType::EdgeWeight accumulated = 0) {
#ifndef NDEBUG
  assert(hypergraph.is_valid());
#endif

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
      hypergraph = hypergraph.contract(begin, end);
    }

    std::vector<std::vector<int>> partitions;
    for (const auto v : hypergraph.vertices()) {
      const auto &partition = hypergraph.vertices_within(v);
      partitions.emplace_back(std::begin(partition), std::end(partition));
    }
    const auto cut =
        HypergraphCut<typename HypergraphType::EdgeWeight>(std::begin(partitions), std::end(partitions), accumulated);
    if constexpr (Verbose) {
      std::cout << "Got cut of value " << cut.value << std::endl;
    }
    return cut;
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

  HypergraphType contracted = hypergraph.contract(sampled_edge_id);

  if (dis(random_generator) < redo) {
    return std::min(branching_contract_<HypergraphType, Verbose>(contracted, k, random_generator, accumulated),
                    branching_contract_<HypergraphType, Verbose>(hypergraph, k, random_generator, accumulated));
  } else {
    return branching_contract_<HypergraphType, Verbose>(contracted, k, random_generator, accumulated);
  }
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
size_t default_num_runs(const HypergraphType &hypergraph, [[maybe_unused]] size_t k) {
  auto log_n =
      static_cast<size_t>(std::ceil(std::log(hypergraph.num_vertices())));
  return log_n * log_n;
}

DECLARE_CONTRACTION_MIN_K_CUT(branching_contract_, default_num_runs)

} // namespace fpz
