// Algorithms for calculating hypergraph min-k-cut from [CXY'18]

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/util.hpp"

namespace cxy {

/* Calculates delta from CXY.
 *
 * Args:
 *   n: number of vertices
 *   e: the size of the hyperedge
 *   k: number of partitions
 */
double cxy_delta(size_t n, size_t e, size_t k, size_t w) {
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
  return std::exp(s) * w;
}

size_t cxy_contract_(Hypergraph &hypergraph, size_t k, [[maybe_unused]] size_t accumulated) {
  std::vector<int> candidates = {};
  std::vector<int> edge_ids;
  std::vector<double> deltas;

  // TODO function for sum of all edge weights
  size_t min_so_far = 0;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    min_so_far += hypergraph.weight(e);
  }

  while (true) {
    edge_ids.resize(hypergraph.edges().size());
    deltas.resize(hypergraph.edges().size());

    size_t i = 0;
    for (const auto &[edge_id, incidence] : hypergraph.edges()) {
      edge_ids[i] = edge_id;
      deltas[i] = cxy_delta(hypergraph.num_vertices(), incidence.size(), k,
                            hypergraph.weight(edge_id));
      ++i;
    }

    if (std::accumulate(std::begin(deltas), std::end(deltas), 0.0) == 0) {
      // TODO function for sum of all edge weights
      size_t cut = 0;
      for (const auto &[e, vertices] : hypergraph.edges()) {
        cut += hypergraph.weight(e);
      }
      min_so_far = std::min(min_so_far, cut);
      break;
    }

    static std::default_random_engine generator;
    std::discrete_distribution<size_t> distribution(std::begin(deltas),
                                                    std::end(deltas));

    size_t sampled = distribution(generator);
    int sampled_id = edge_ids.at(sampled);

    hypergraph = hypergraph.contract(sampled_id);
  }

  return min_so_far;
}

/* n choose r
 *
 * [https://stackoverflow.com/questions/9330915/number-of-combinations-n-choose-r-in-c]
 */
unsigned long long ncr(unsigned long long n, unsigned long long k) {
  if (k > n) {
    return 0;
  }
  if (k * 2 > n) {
    k = n - k;
  }
  if (k == 0) {
    return 1;
  }

  unsigned long long result = n;
  for (unsigned long long i = 2; i <= k; ++i) {
    result *= (n - i + 1);
    result /= i;
  }
  return result;
}

size_t default_num_runs(const Hypergraph &hypergraph, size_t k) {
  // TODO this is likely to overflow when n is large (> 100000)
  auto num_runs = ncr(hypergraph.num_vertices(), 2 * (k - 1));
  num_runs *= static_cast<decltype(num_runs)>(
      std::ceil(std::log(hypergraph.num_vertices())));
  num_runs = std::max(num_runs, 1ull);
  return num_runs;
}

// Algorithm for calculating hypergraph min-k-cut from CXY '18
inline size_t cxy_contract(Hypergraph &hypergraph, size_t k, size_t num_runs = 0, bool verbose = false) {
  return hypergraph_util::minimum_of_runs<cxy_contract_, default_num_runs>(hypergraph, k, num_runs, verbose);
}

}