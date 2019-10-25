// Algorithm for calculating hypergraph min-k-cut from CXY '18

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "hypergraph.h"

namespace cxy {

/* Calculates delta from CXY.
 *
 * Args:
 *   n: number of vertices
 *   e: the size of the hyperedge
 *   k: number of partitions
 */
double cxy_delta(size_t n, size_t e, size_t k) {
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
  return  std::exp(s);
}

size_t cxy_contract_(Hypergraph &hypergraph, unsigned long long k) {
  std::vector<int> candidates = {};
  std::vector<int> edge_ids;
  std::vector<double> deltas;

  auto min_so_far = hypergraph.edges().size();

  while (true) {
    edge_ids.resize(hypergraph.edges().size());
    deltas.resize(hypergraph.edges().size());

    size_t i = 0;
    for (const auto &[edge_id, incidence] : hypergraph.edges()) {
      edge_ids[i] = edge_id;
      deltas[i] = cxy_delta(hypergraph.num_vertices(), incidence.size(), k);
      ++i;
    }

    if (std::accumulate(std::begin(deltas), std::end(deltas), 0.0) == 0) {
      min_so_far = std::min(min_so_far, hypergraph.num_edges());
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

// Algorithm for calculating hypergraph min-k-cut from CXY '18
size_t cxy_contract(Hypergraph &hypergraph, size_t k) {
  // TODO this is likely to overflow when n is large (> 100000)
  auto repeat = ncr(hypergraph.num_vertices(), 2 * (k - 1));
  repeat *= static_cast<decltype(repeat)>(
      std::ceil(std::log(hypergraph.num_vertices())));
  repeat = std::max(repeat, 1ull);
  size_t min_so_far = std::numeric_limits<size_t>::max();
  for (unsigned long long i = 0; i < repeat; ++i) {
    Hypergraph copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    size_t answer_ = cxy_contract_(copy, k);
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

}
