// Algorithms for calculating hypergraph min-k-cut from [CXY'18]
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <cassert>

#include "hypergraph.hpp"
#include "util.hpp"

namespace hypergraphlib {

struct cxy : public ContractionAlgo<cxy> {

/**
 * Calculates delta from CXY. The probability that an edge
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
 *
 *   n: number of vertices
 *   e: the size of the hyperedge
 *   k: number of partitions
 */
  static inline double cxy_delta(size_t num_vertices, size_t hyperedge_size, size_t k) {
    static std::map<std::tuple<size_t, size_t, size_t>, double> memo;

    if (auto it = memo.find(std::make_tuple(num_vertices, hyperedge_size, k)); it != std::end(memo)) {
      return it->second;
    }

    double s = 0;
    if (num_vertices < hyperedge_size + k - 2) {
      return 0;
    }
    for (size_t i = num_vertices - (hyperedge_size + k - 2); i <= num_vertices - hyperedge_size; ++i) {
      s += std::log(i);
    }
    if (num_vertices < k - 2) {
      return 0;
    }
    for (size_t i = num_vertices - (k - 2); i <= num_vertices; ++i) {
      s -= std::log(i);
    }
    assert(!std::isnan(s));
    return std::exp(s);
  }

/**
 * Return n choose k
 */
  static inline unsigned long long ncr(unsigned long long n, unsigned long long k) {
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

  static constexpr bool pass_discovery_value = false;

  static constexpr char name[] = "CXY";

  template<typename HypergraphType>
  using Context = hypergraph_util::Context<HypergraphType>;

/**
 * The contraction algorithm from [CXY'18]. This returns the minimum cut with some probability.
 *
 * Takes time O(np) where p is the size of the hypergraph.
 */
  template<typename HypergraphType, bool ReturnPartitions, uint8_t Verbosity>
  static HypergraphCut<typename HypergraphType::EdgeWeight> contract(Context<HypergraphType> &ctx) {
    HypergraphType hypergraph(ctx.hypergraph);

    std::vector<int> candidates = {};
    std::vector<int> edge_ids;
    std::vector<double> deltas;

    typename HypergraphType::EdgeWeight min_so_far = total_edge_weight(hypergraph);

    while (true) {
      edge_ids.resize(hypergraph.edges().size());
      deltas.resize(hypergraph.edges().size());

      size_t i = 0;
      for (const auto &[edge_id, incidence] : hypergraph.edges()) {
        edge_ids[i] = edge_id;
        deltas[i] = cxy_delta(hypergraph.num_vertices(), incidence.size(), ctx.k) * edge_weight(hypergraph, edge_id);
        ++i;
      }

      if (std::accumulate(std::begin(deltas), std::end(deltas), 0.0) == 0) {
        auto cut = total_edge_weight(hypergraph);
        min_so_far = std::min(min_so_far, cut);
        break;
      }

      std::discrete_distribution<size_t> distribution(std::begin(deltas),
                                                      std::end(deltas));

      size_t sampled = distribution(ctx.random_generator);
      int sampled_id = edge_ids.at(sampled);

      hypergraph.template contract_in_place<true, ReturnPartitions>(sampled_id);
      ++ctx.stats.num_contractions;
    }

    // May terminate early if it finds a zero cost cut with >k partitions, so need
    // to merge partitions. At this point the sum of deltas is zero, so every
    // remaining hyperedge crosses all components, so we can merge components
    // without changing the cut value.
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
      return HypergraphCut<typename HypergraphType::EdgeWeight>(std::begin(partitions),
                                                                std::end(partitions),
                                                                min_so_far);
    } else {
      return HypergraphCut<typename HypergraphType::EdgeWeight>(min_so_far);
    }
  }

/**
 * Calculate the number of runs required to find the minimum cut with high probability.
 */
  template<typename HypergraphType>
  static size_t default_num_runs(const HypergraphType &hypergraph, size_t k) {
    // TODO this is likely to overflow when n is large (> 100000)
    auto num_runs = ncr(hypergraph.num_vertices(), 2 * (k - 1));
    num_runs *= static_cast<decltype(num_runs)>(
        std::ceil(std::log(hypergraph.num_vertices())));
    num_runs = std::max(num_runs, 1ull);
    return num_runs;
  }

};

}
