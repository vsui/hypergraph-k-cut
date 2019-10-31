// Branching contraction algorithm from FPZ '19

#include <cmath>
#include <iostream>
#include <random>

#include "hypergraph/hypergraph.h"

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
  // TODO add this in debug mode: assert(hypergraph.is_valid());
  // Remove k-spanning hyperedges from hypergraph
  for (auto it = std::begin(hypergraph.edges());
       it != std::end(hypergraph.edges());) {
    if (it->second.size() >= hypergraph.num_vertices() - k + 2) {
      accumulated += hypergraph.weight(it->first);
      // Remove k-spanning hyperedge from incidence lists
      for (const int v : it->second) {
        auto &vertex_incidence_list = hypergraph.vertices().at(v);
        auto it2 = std::find(std::begin(vertex_incidence_list),
                             std::end(vertex_incidence_list), it->first);
        // Swap it with the last element and pop it off to remove in O(1) time
        std::iter_swap(it2, std::end(vertex_incidence_list) - 1);
        vertex_incidence_list.pop_back();
      }
      // Remove k-spanning hyperedge
      it = hypergraph.edges().erase(it);
    } else {
      ++it;
    }
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

/* Runs branching contraction log^2(n) times and returns the minimum */
size_t branching_contract(Hypergraph &hypergraph, size_t k) {
  size_t logn =
      static_cast<size_t>(std::ceil(std::log(hypergraph.num_vertices())));
  size_t repeat = logn * logn;

  size_t min_so_far = std::numeric_limits<size_t>::max();
  for (size_t i = 0; i < repeat; ++i) {
    // branching_contract_ modifies the input hypergraph so avoid copying, so
    // copy it once in the beginning to save time
    Hypergraph copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    size_t answer_ = branching_contract_(copy, k);
    min_so_far = std::min(min_so_far, answer_);
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "[" << i + 1 << "/" << repeat << "] took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " milliseconds, got " << answer_ << ", min is " << min_so_far
              << "\n";
  }

  return min_so_far;
}

} // namespace fpz
