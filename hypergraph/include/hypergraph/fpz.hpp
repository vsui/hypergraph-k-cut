// Branching contraction algorithm from [FPZ'19]
#pragma once

#include <cmath>
#include <iostream>
#include <random>
#include <cassert>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/cxy.hpp"

namespace fpz {

/* Redo probability from the branching contraction paper. The calculations have
 * been changed a bit to make calculation efficient and to avoid overflow
 *
 * Args:
 *   n: number of vertices in the hypergraph
 *   e: number of vertices that the hyperedge is incident on
 *   k: number of partitions
 */
double redo_probability(size_t n, size_t e, size_t k) {
  return 1 - cxy::cxy_delta(n, e, k);
}

/* Branching contraction min cut inner routine
 *
 * accumulated : a running count of k-spanning hyperedges used to calculate the
 *               min cut
 * */
template<typename HypergraphType, bool Verbose = false>
HypergraphCut<HypergraphType> branching_contract_(HypergraphType &hypergraph,
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
    // May terminate early if it finds a perfect cut with >k partitions, so need
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
    const auto cut = HypergraphCut<HypergraphType>(std::begin(partitions), std::end(partitions), accumulated);
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

template<typename HypergraphType>
size_t default_num_runs(const HypergraphType &hypergraph, [[maybe_unused]] size_t k) {
  auto log_n =
      static_cast<size_t>(std::ceil(std::log(hypergraph.num_vertices())));
  return log_n * log_n;
}

template<typename HypergraphType, bool Verbose = false, bool VVerbose = false>
inline auto branching_contract(const HypergraphType &hypergraph,
                               size_t k,
                               size_t num_runs = 0,
                               uint64_t default_seed = 0) {
  std::mt19937_64 random_generator;
  if (default_seed) {
    random_generator.seed(default_seed);
  }
  return hypergraph_util::minimum_of_runs<HypergraphType,
                                          branching_contract_<HypergraphType, VVerbose>,
                                          default_num_runs,
                                          Verbose>(hypergraph, k, num_runs, random_generator);
}

} // namespace fpz
