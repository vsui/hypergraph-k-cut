#pragma once

#include <iostream>
#include <vector>

#include "certificate.h"
#include "hypergraph.h"

/* Returns a maximum adjacency ordering of vertices, starting with vertex a.
 * Linear in the number of vertices across all hyperedges.
 *
 * Here, tightness for a vertex v is the number of edges that intersect the
 * ordering so far and v.
 */
std::vector<int> maximum_adjacency_ordering(const Hypergraph &hypergraph,
                                            const int a);

/* Return a tight ordering of vertices, starting with vertex a. Linear in the
 * number of vertices across all hyperedges.
 *
 * Here, tightness is the number of edges connecting a vertex v to the
 * ordering so far that consist of vertices either in the ordering or v
 * itself.
 *
 * Takes time linear with the size of the hypergraph.
 */
std::vector<int> tight_ordering(const Hypergraph &hypergraph, const int a);

/* Return a Queyranne ordering of vertices, starting with vertex a.
 *
 * Takes time linear with the size of the hypergraph.
 */
std::vector<int> queyranne_ordering(const Hypergraph &hypergraph, const int a);

size_t one_vertex_cut(const Hypergraph &hypergraph, const int v);

Hypergraph merge_vertices(const Hypergraph &hypergraph, const int s,
                          const int t);

/* Given a hypergraph and a function that orders the vertices, find the min cut
 * by repeatedly finding and contracting pendant pairs.
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `maximum_adjacency_ordering`.
 */
template <typename Ordering>
size_t vertex_ordering_mincut(Hypergraph &hypergraph, const int a, Ordering f) {
  size_t min_cut_of_phase = std::numeric_limits<size_t>::max();
  while (hypergraph.vertices().size() > 1) {
    auto ordering = f(hypergraph, a);
    size_t cut_of_phase = one_vertex_cut(hypergraph, ordering.back());
    hypergraph = merge_vertices(hypergraph, *(std::end(ordering) - 2),
                                *(std::end(ordering) - 1));
    min_cut_of_phase = std::min(min_cut_of_phase, cut_of_phase);
  }
  return min_cut_of_phase;
}

/* Given a hypergraph and a function that orders the vertices, find the minimum
 * cut through an exponential search on the minimum cuts of k-trimmed
 * certificates.
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `maximum_adjacency_ordering`.
 */
template <typename Ordering>
size_t vertex_ordering_mincut_certificate(Hypergraph &hypergraph, const int a,
                                          Ordering f) {
  KTrimmedCertificate gen(hypergraph);
  size_t k = 1;
  while (true) {
    // Copy hypergraph
    std::cout << "Trying k=" << k << std::endl;
    Hypergraph certificate = gen.certificate(k);
    std::cout << "Tried k=" << k << std::endl;
    size_t mincut = vertex_ordering_mincut(certificate, a, f);
    if (mincut < k) {
      return mincut;
    }
    k *= 2;
  }
}
