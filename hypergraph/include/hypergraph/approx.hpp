#pragma once

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/order.hpp"

/* Approximate (2+epsilon) hypergraph minimum cut from [CX'18].
 *
 * Time complexity: O(p/epsilon), p is the size of the hypergraph
 */
size_t approximate_minimizer(Hypergraph &hypergraph, const double epsilon) {
  if (hypergraph.num_vertices() == 1) {
    return std::numeric_limits<size_t>::max();
  }

  size_t delta = std::numeric_limits<size_t>::max();
  for (const auto v : hypergraph.vertices()) {
    // TODO one vertex cut
    delta = std::min(delta, hypergraph.edges_incident_on(v).size());
  }
  if (delta == 0) {
    return 0;
  }
  double alpha = delta / (2.0 + epsilon);

  const auto &[ordering, tightness] = queyranne_ordering_with_tightness(
      hypergraph, *std::begin(hypergraph.vertices()));

  // TODO maybe tightness should return pairs to enforce that len(order) =
  // len(tightness)...
  auto curr_order = std::begin(ordering);
  auto curr_tight = std::begin(tightness);
  auto begin = curr_order;
  using it_pair = std::pair<decltype(curr_order), decltype(curr_order)>;
  std::vector<it_pair> alpha_tight_sets;
  for (; curr_order != std::end(ordering) - 1; ++curr_order, ++curr_tight) {
    if (*(curr_tight + 1) < alpha) {
      // Check to see if we should contract the current set (if it has more than
      // one element)
      if (std::distance(begin, curr_order) > 1) {
        alpha_tight_sets.emplace_back(begin, curr_order);
      }
      // Start a new alpha-tight set
      begin = curr_order + 1;
    }
  }

  if (std::distance(begin, std::end(ordering)) > 1) {
    alpha_tight_sets.emplace_back(begin, std::end(ordering));
  }

  // alpha contraction
  Hypergraph temp(hypergraph);
  for (const auto[begin, end] : alpha_tight_sets) {
    temp = temp.contract(begin, end);
  }

  return std::min(delta, approximate_minimizer(temp, epsilon));
}
