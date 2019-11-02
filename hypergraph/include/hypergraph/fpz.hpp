// Branching contraction algorithm from [FPZ'19]

#include <cmath>
#include <iostream>
#include <random>

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
size_t branching_contract_(Hypergraph &hypergraph, size_t k,
                           size_t accumulated = 0) {
#ifndef NDEBUG
  assert(hypergraph.is_valid());
#endif

  // Remove k-spanning hyperedges from hypergraph
  std::vector<int> k_spanning_hyperedges;
  for (const auto &[edge_id, vertices] : hypergraph.edges()) {
    // TODO overflow here?
    if (vertices.size() >= hypergraph.num_vertices() - k + 2) {
      k_spanning_hyperedges.push_back(edge_id);
      accumulated += 1;
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
  // Note: dealing with unweighted hyperedges
  // TODO support weight hyperedges
  std::pair<int, std::vector<int>> sampled[1];
  std::sample(std::begin(hypergraph.edges()), std::end(hypergraph.edges()),
              std::begin(sampled), 1, gen);

  double redo =
      redo_probability(hypergraph.num_vertices(), sampled[0].second.size(), k);

  Hypergraph contracted = hypergraph.contract(sampled->first);

  if (dis(gen) < redo) {
    return std::min(branching_contract_(contracted, k, accumulated),
                    branching_contract_(hypergraph, k, accumulated));
  } else {
    return branching_contract_(contracted, k, accumulated);
  }
}

size_t default_num_runs(const Hypergraph &hypergraph, size_t k) {
  size_t log_n =
      static_cast<size_t>(std::ceil(std::log(hypergraph.num_vertices())));
  return log_n * log_n;
}

inline size_t branching_contract(const Hypergraph &hypergraph, size_t k, size_t num_runs = 0, bool verbose = false) {
  return hypergraph_util::minimum_of_runs<branching_contract_, default_num_runs>(hypergraph, k, num_runs, verbose);
}

} // namespace fpz
