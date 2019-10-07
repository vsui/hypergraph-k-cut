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
double cxy_delta(int n, int e, int k) {
  int s = 0;
  for (int i = n - e - k + 2; i <= n - e; ++i) {
    s += std::log(i);
  }
  for (int i = n - k + 2; i <= n; ++i) {
    s -= std::log(i);
  }
  return std::exp(s);
}

int cxy_contract_(Hypergraph &hypergraph, int k) {
  std::vector<int> candidates = {};
  std::vector<int> edge_ids;
  std::vector<double> deltas;

  int min_so_far = hypergraph.edges().size();

  while (true) {
    if (hypergraph.num_vertices() <= 4 * k - 4) {
      // BF k-cut-sets in G
    }

    edge_ids.resize(hypergraph.edges().size());
    deltas.resize(hypergraph.edges().size());

    int i = 0;
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
    std::discrete_distribution<int> distribution(std::begin(deltas),
                                                 std::end(deltas));

    int sampled = distribution(generator);
    int sampled_id = edge_ids.at(sampled);

    hypergraph = hypergraph.contract(sampled_id);
  }

  return min_so_far;
}

/* n choose r
 *
 * [https://stackoverflow.com/questions/9330915/number-of-combinations-n-choose-r-in-c]
 */
long long ncr(int n, int k) {
  if (k > n) {
    return 0;
  }
  if (k * 2 > n) {
    k = n - k;
  }
  if (k == 0) {
    return 1;
  }

  long long result = n;
  for (int i = 2; i <= k; ++i) {
    result *= (n - i + 1);
    result /= i;
  }
  return result;
}

// Algorithm for calculating hypergraph min-k-cut from CXY '18
int cxy_contract(Hypergraph &hypergraph, int k) {
  // TODO this is likely to overflow when n is large (> 100000)
  long long repeat = ncr(hypergraph.num_vertices(), 2 * (k - 1));
  repeat *= std::ceil(std::log(hypergraph.num_vertices()));
  repeat = std::max(repeat, 1ll);
  int min_so_far = std::numeric_limits<int>::max();
  for (long long i = 0; i < repeat; ++i) {
    Hypergraph copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    int answer_ = cxy_contract_(copy, k);
    min_so_far = std::min(min_so_far, answer_);
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "[" << i + 1 << "/" << repeat << "] took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " milliseconds, got " << answer_ << "\n";
  }
  return min_so_far;
}

}
