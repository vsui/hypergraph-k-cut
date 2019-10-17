#include <vector>

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
 */
std::vector<int> tight_ordering(const Hypergraph &hypergraph, const int a);

/* Return a Queyranne ordering of vertices, starting with vertex a. Linear in
 * the number of vertices across all hyperedges.
 */
std::vector<int> queyranne_ordering(const Hypergraph &hypergraph, const int a);

/* Returns the induced head ordering of the edges. */
std::vector<int> induced_head_ordering(const Hypergraph &hypergraph,
                                       std::vector<int> vertex_ordering);

/* A data structure that can be used to retrieve k-trimmed certificates of a
 * hypergraph. That is, a trimmed subhypergraph that retains all cut values up
 * to k. */
class KTrimmedCertificate {
public:
  /* Constructor runs in time linear to the number of vertices across all edges.
   */
  KTrimmedCertificate(const Hypergraph &hypergraph);

  /* Returns the k-trimmed certificate in O(kn) time. */
  Hypergraph certificate(const int k) const;

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

int one_vertex_cut(const Hypergraph &hypergraph, const int v);

Hypergraph merge_vertices(const Hypergraph &hypergraph, const int s,
                          const int t);

/* Given a hypergraph and a function that orders the vertices, find the min cut
 * by repeatedly finding and contracting pendant pairs.
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `induced_head_ordering`.
 */
template <typename Ordering>
int vertex_ordering_mincut(Hypergraph &hypergraph, const int a, Ordering f) {
  int min_cut_of_phase = std::numeric_limits<int>::max();
  while (hypergraph.vertices().size() > 1) {
    auto ordering = f(hypergraph, a);
    int cut_of_phase = one_vertex_cut(hypergraph, ordering.back());
    hypergraph = merge_vertices(hypergraph, *(std::end(ordering) - 2),
                                *(std::end(ordering) - 1));
    min_cut_of_phase = std::min(min_cut_of_phase, cut_of_phase);
  }
  return min_cut_of_phase;
}

#include <iostream>
template <typename Ordering>
int vertex_ordering_mincut_certificate(Hypergraph &hypergraph, const int a,
                                       Ordering f) {
  KTrimmedCertificate gen(hypergraph);
  int k = 1;
  while (true) {
    // Copy hypergraph
    std::cout << "Trying k=" << k << std::endl;
    Hypergraph certificate = gen.certificate(k);
    std::cout << "Tried k=" << k << std::endl;
    int mincut = vertex_ordering_mincut(certificate, a, f);
    if (mincut < k) {
      return mincut;
    }
    k *= 2;
  }
  return -1;
}
