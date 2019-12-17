// Algorithms for calculating hypergraph min-k-cut from [CXY'18]
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <cassert>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/util.hpp"

namespace cxy {

/* Calculates delta from CXY. The probability that an edge
 * will be contracted in the CXY k-cut algorithm is proportional
 * to the returned value.
 *
 * In the paper delta is defined as
 *   delta_e := c(e)(n - r(e) CHOOSE k-1) / (n CHOOSE k-1).
 * This function returns delta_e for unweighted graphs,
 *   delta_e := (n - r(e) CHOOSE k-1) / (n CHOOSE k-1).
 * If the caller wants to calculate delta_e for weighted graphs,
 * then they can multiply the returned value by the weight of the
 * edge themselves.
 *
 * The calculation is computed in logs for numerical stability.
 *
 * Args:
 *   n: number of vertices
 *   e: the size of the hyperedge
 *   k: number of partitions
 */
double cxy_delta(size_t n, size_t e, size_t k) {
  static std::map<std::tuple<size_t, size_t, size_t>, double> memo;

  if (auto it = memo.find(std::make_tuple(n, e, k)); it != std::end(memo)) {
    return it->second;
  }

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
  assert(!std::isnan(s));
  return std::exp(s);
}

template<typename HypergraphType>
HypergraphCut<HypergraphType> cxy_contract_(HypergraphType &hypergraph,
                                            size_t k,
                                            std::mt19937_64 &random_generator,
                                            [[maybe_unused]] typename HypergraphType::EdgeWeight accumulated) {
  std::vector<int> candidates = {};
  std::vector<int> edge_ids;
  std::vector<double> deltas;

  // TODO function for sum of all edge weights
  typename HypergraphType::EdgeWeight min_so_far = 0;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    min_so_far += edge_weight(hypergraph, e);
  }

  while (true) {
    edge_ids.resize(hypergraph.edges().size());
    deltas.resize(hypergraph.edges().size());

    size_t i = 0;
    for (const auto &[edge_id, incidence] : hypergraph.edges()) {
      edge_ids[i] = edge_id;
      deltas[i] = cxy_delta(hypergraph.num_vertices(), incidence.size(), k) * edge_weight(hypergraph, edge_id);
      ++i;
    }

    if (std::accumulate(std::begin(deltas), std::end(deltas), 0.0) == 0) {
      // TODO function for sum of all edge weights
      auto cut = total_edge_weight(hypergraph);
      min_so_far = std::min(min_so_far, cut);
      break;
    }

    std::discrete_distribution<size_t> distribution(std::begin(deltas),
                                                    std::end(deltas));

    size_t sampled = distribution(random_generator);
    int sampled_id = edge_ids.at(sampled);

    hypergraph = hypergraph.contract(sampled_id);
  }

  // May terminate early if it finds a perfect cut with >k partitions, so need
  // to merge partitions. At this point the sum of deltas is zero, so every
  // remaining hyperedge crosses all components, so we can merge components
  // without changing the cut value.
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

  return HypergraphCut<HypergraphType>(std::begin(partitions), std::end(partitions), min_so_far);
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

template<typename HypergraphType>
size_t default_num_runs(const HypergraphType &hypergraph, size_t k) {
  // TODO this is likely to overflow when n is large (> 100000)
  auto num_runs = ncr(hypergraph.num_vertices(), 2 * (k - 1));
  num_runs *= static_cast<decltype(num_runs)>(
      std::ceil(std::log(hypergraph.num_vertices())));
  num_runs = std::max(num_runs, 1ull);
  return num_runs;
}

// Algorithm for calculating hypergraph min-k-cut from CXY '18
template<typename HypergraphType, bool Verbose = false>
inline auto cxy_contract(const HypergraphType &hypergraph,
                         size_t k,
                         size_t num_runs = 0,
                         uint64_t default_seed = 0) {
  std::mt19937_64 random_generator;
  if (default_seed) {
    random_generator.seed(default_seed);
  }
  return hypergraph_util::minimum_of_runs<HypergraphType,
                                          cxy_contract_<HypergraphType>,
                                          default_num_runs,
                                          Verbose>(hypergraph, k, num_runs, random_generator);
}

}
