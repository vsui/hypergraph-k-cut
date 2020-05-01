#pragma once

#include "hypergraph.hpp"
#include "certificate.hpp"
#include "order.hpp"

namespace hypergraphlib {

/* Approximate (2+epsilon) hypergraph minimum cut from [CX'18].
 *
 * Time complexity: O(p/epsilon), p is the size of the hypergraph
 */
template<typename HypergraphType>
HypergraphCut<typename HypergraphType::EdgeWeight> approximate_minimizer(HypergraphType &hypergraph,
                                                                         const double epsilon) {
  using Cut = HypergraphCut<typename HypergraphType::EdgeWeight>;

  if (hypergraph.num_vertices() == 1) {
    return Cut::max();
  }

  auto delta = Cut::max();
  for (const auto v : hypergraph.vertices()) {
    // TODO one vertex cut
    delta = std::min(delta, one_vertex_cut<true>(hypergraph, v));
  }
  if (delta.value == 0) {
    return delta;
  }
  double alpha = delta.value / (2.0 + epsilon);

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
  HypergraphType temp(hypergraph);
  for (const auto[begin, end] : alpha_tight_sets) {
    // TODO support not getting contraction
    temp = temp.template contract<false, true>(begin, end);
  }

  return std::min(delta, approximate_minimizer(temp, epsilon));
}

/**
 * Combines the certificate and approximate algorithm.
 *
 * Uses the approximate algorithm to get a bound on the min cut for the certificate, and then runs the
 * algorithm on the certificate.
 */
template<auto MinCutFunc>
HypergraphCut <size_t> apxCertCX(Hypergraph &hypergraph, const double epsilon) {
  Hypergraph copy(hypergraph);
  const auto approx_cut = approximate_minimizer(copy, epsilon);
  KTrimmedCertificate certifier(hypergraph);
  auto certificate = certifier.certificate(approx_cut.value);
  return MinCutFunc(certificate);
}

}
