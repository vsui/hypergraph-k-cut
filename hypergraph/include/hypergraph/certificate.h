#pragma once

#include <vector>

#include "hypergraph/hypergraph.h"

/* A data structure that can be used to retrieve k-trimmed certificates of a
 * hypergraph. A k-trimmed certificate is a trimmed subhypergraph that retains
 * all cut values up to k. See the [CX'18] for more details.
 */
class KTrimmedCertificate {
public:
  /* Constructor.
   *
   * Time complexity: O(p) where p is the size of the hypergraph 
   */
  KTrimmedCertificate(const Hypergraph &hypergraph);

  /* Returns the k-trimmed certificate.
   *
   * Time complexity: O(kn) where n is the number of vertices
   */
  Hypergraph certificate(const size_t k) const;

private:
  // Return the head of an edge (the vertex in it that occurs first in the
  // vertex ordering). Takes constant time.
  int head(const int e) const;

  // The hypergraph we are creating certificates of
  const Hypergraph hypergraph_;

  // Maximum adjacency ordering of the vertices
  std::vector<int> vertex_ordering_;

  // Mapping of edges to their induce ordering head
  std::unordered_map<int, size_t> edge_to_head_;

  // A list for each vertex that holds v's backward edges in the head ordering
  // (see paper for more details)
  std::unordered_map<int, std::vector<int>> backward_edges_;
};
