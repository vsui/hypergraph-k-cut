#pragma once

#include <vector>
#include <iostream>

#include "hypergraph.hpp"
#include "cut.hpp"

namespace hypergraphlib {

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
  explicit KTrimmedCertificate(const Hypergraph &hypergraph);

  /* Returns the k-trimmed certificate.
   *
   * Time complexity: O(kn) where n is the number of vertices
   */
  [[nodiscard]]
  Hypergraph certificate(size_t k) const;

private:
  // Return the head of an edge (the vertex in it that occurs first in the
  // vertex ordering). Takes constant time.
  [[nodiscard]]
  int head(int e) const;

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
/* Given a hypergraph and a function that orders the vertices, find the minimum
* cut through an exponential search on the minimum cuts of k-trimmed
* certificates. See [CX'09] for more details.
*
* Time complexity: O(p + cn^2), where p is the size of the hypergraph, c is the
* value of the minimum cut, and n is the number of vertices
*/
template<typename HypergraphType, bool ReturnsPartitions = true>
Cut<HypergraphType, ReturnsPartitions> certificate_minimum_cut(const HypergraphType &hypergraph,
                                                               MinimumCutFunction<HypergraphType,
                                                               ReturnsPartitions> min_cut) {
  KTrimmedCertificate gen(hypergraph);
  size_t k = 1;
  while (true) {
    // Copy hypergraph
    Hypergraph certificate = gen.certificate(k);
    auto cut = min_cut(certificate);
    if (cut_value<HypergraphType>(cut) < k) {
      return cut;
    }
    k *= 2;
  }
}

}
