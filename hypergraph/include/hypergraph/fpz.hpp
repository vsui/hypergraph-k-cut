// Branching contraction algorithm from [FPZ'19]

#include <cmath>
#include <iostream>
#include <random>
#include <cassert>

#include "hypergraph/hypergraph.hpp"

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
  double s = 0;
  if (n < e + k - 2) {
    return 0;
  }
  for (size_t i = n - (e + k - 2); i <= n - e; ++i) {
    s += std::log(i);
  }
  if (n < k - 2) {
    return 0;
  }
  for (size_t i = n - (k - 2); i <= n; ++i) {
    s -= std::log(i);
  }
  assert(!isnan(s));
  return 1 - std::exp(s);
}

/* Branching contraction min cut inner routine
 *
 * accumulated : a running count of k-spanning hyperedges used to calculate the
 *               min cut
 * */
template<typename HypergraphType, typename EdgeWeightType>
EdgeWeightType branching_contract_(HypergraphType &hypergraph, size_t k, EdgeWeightType accumulated = 0) {
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
    return accumulated;
  }

  // Static random device for random sampling and generating random numbers
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<> dis(0.0, 1.0);


  // Select a hyperedge with probability proportional to its weight
  std::vector<int> edge_ids;
  std::vector<EdgeWeightType> edge_weights;
  for (const auto &[edge_id, vertices] : hypergraph.edges()) {
    edge_ids.push_back(edge_id);
    edge_weights.push_back(edge_weight(hypergraph, edge_id));
  }
  std::discrete_distribution<size_t> distribution(std::begin(edge_weights), std::end(edge_weights));
  const auto sampled_edge_id = edge_ids.at(distribution(gen));
  const auto sampled_edge = hypergraph.edges().at(sampled_edge_id);

  double redo =
      redo_probability(hypergraph.num_vertices(), sampled_edge.size(), k);

  HypergraphType contracted = hypergraph.contract(sampled_edge_id);

  if (dis(gen) < redo) {
    return std::min(branching_contract_(contracted, k, accumulated),
                    branching_contract_(hypergraph, k, accumulated));
  } else {
    return branching_contract_(contracted, k, accumulated);
  }
}

template<typename HypergraphType>
size_t default_num_runs(const HypergraphType &hypergraph, [[maybe_unused]] size_t k) {
  auto log_n =
      static_cast<size_t>(std::ceil(std::log(hypergraph.num_vertices())));
  return log_n * log_n;
}

template<typename HypergraphType, typename EdgeWeightType = size_t>
inline EdgeWeightType branching_contract(const HypergraphType &hypergraph,
                                         size_t k,
                                         size_t num_runs = 0,
                                         bool verbose = false) {
  return hypergraph_util::minimum_of_runs<HypergraphType,
                                          EdgeWeightType,
                                          branching_contract_<HypergraphType, EdgeWeightType>,
                                          default_num_runs>(hypergraph, k, num_runs, verbose);
}

} // namespace fpz
