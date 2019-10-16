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
