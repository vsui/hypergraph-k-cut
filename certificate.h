#pragma once

#include <vector>

#include "hypergraph.h"

/* A data structure that can be used to retrieve k-trimmed certificates of a
 * hypergraph. A k-trimmed certificate is a trimmed subhypergraph that retains
 * all cut values up to k. See the [CX'18] for more details.
 */
class KTrimmedCertificate {
public:
  /* Constructor.
   *
   * Takes time linear with the size of the hypergraph.
   */
  KTrimmedCertificate(const Hypergraph &hypergraph);

  /* Returns the k-trimmed certificate in O(kn) time.
   */
  Hypergraph certificate(const size_t k) const;

private:
  // Return the head of an edge (the vertex in it that occurs first in the
  // vertex ordering)
  int head(const int e) const;

  // The hypergraph we are creating certificates of
  const Hypergraph hypergraph_;

  // Maximum adjacency ordering of the vertices
  std::vector<int> vertex_ordering_;

  // A list for each vertex that holds v's backward edges in the head ordering
  // (see paper for more details)
  std::unordered_map<int, std::vector<int>> backward_edges_;
};
