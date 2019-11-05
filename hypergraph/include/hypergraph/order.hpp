#pragma once

#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <boost/heap/fibonacci_heap.hpp>

#include "hypergraph/heap.hpp"
#include "hypergraph/certificate.hpp"
#include "hypergraph/hypergraph.hpp"

// A context for vertex ordering calculations
template<typename Heap>
struct OrderingContext {
  // Constructor
  OrderingContext(const std::vector<int> &vertices, size_t capacity) : heap(vertices, capacity) {}

  // Heap for tracking which vertices are most tightly connected to the
  // ordering
  Heap heap;

  std::unordered_map<
      int, boost::heap::fibonacci_heap<std::pair<size_t, int>>::handle_type>
      handles;

  // A mapping of edges to the number of vertices inside the edges that have not
  // been ordered
  std::unordered_map<int, int> edge_to_num_vertices_outside_ordering;

  // Vertices that have been marked as used by the ordering
  std::unordered_set<int> used_vertices;

  // Edges that have been marked as used by the ordering
  std::unordered_set<int> used_edges;
};

/* The method for calculating a vertex ordering for maximum adjacency and tight,
 * and Queyranne orderings is very similar - maintain how "tight" each vertex is
 * and repeatedly add the "tightest" vertex. The only difference is how
 * "tightness" is defined for each ordering. The "_tighten" functions define how
 * each ordering maintains tightness.
 *
 * Each of these methods runs in time linear to the number of edges incident on
 * v.
 */
template<typename HypergraphType>
void maximum_adjacency_ordering_tighten(
    const HypergraphType &hypergraph,
    OrderingContext<typename HypergraphType::Heap> &ctx,
    const int v) {
  // We are adding v to the ordering so far, and need to update the keys for the
  // other vertices
  // Check each edge e incident on v
  for (const int e : hypergraph.edges_incident_on(v)) {
    // If e has already been used, then skip it
    if (ctx.used_edges.find(e) != std::end(ctx.used_edges)) {
      continue;
    }
    // For every vertex u in e that is not v and not already in the ordering,
    // increment its key (it is one edge tighter now)
    // TODO I think vertices need to be removed from the edge's incidence list
    //      to maintain the runtime.
    for (const int u : hypergraph.edges().at(e)) {
      if (ctx.used_vertices.find(u) == std::end(ctx.used_vertices)) {
        if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
          ctx.heap.increment(u);
        } else {
          ctx.heap.increment(u, edge_weight(hypergraph, e));
        }
      }
    }
    ctx.used_edges.insert(e);
  }
}

template<typename HypergraphType>
void tight_ordering_tighten(
    const HypergraphType &hypergraph,
    OrderingContext<typename HypergraphType::Heap> &ctx,
    const int v) {
  // For every edge e incident on v
  for (const int e : hypergraph.edges_incident_on(v)) {
    // Tighten this edge
    ctx.edge_to_num_vertices_outside_ordering.at(e) -= 1ul;
    // If the edge only has one vertex u left that is outside the ordering,
    // increase the key of u
    if (ctx.edge_to_num_vertices_outside_ordering.at(e) == 1) {
      for (const int u : hypergraph.edges().at(e)) {
        if (ctx.used_vertices.find(u) == std::end(ctx.used_vertices)) {
          if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
            ctx.heap.increment(u);
          } else {
            ctx.heap.increment(u, edge_weight(hypergraph, e));
          }
        }
      }
    }
  }
}

template<typename HypergraphType>
void queyranne_ordering_tighten(
    const HypergraphType &hypergraph,
    OrderingContext<typename HypergraphType::Heap> &ctx,
    const int v) {
  maximum_adjacency_ordering_tighten(hypergraph, ctx, v);
  tight_ordering_tighten(hypergraph, ctx, v);
}

template<typename HypergraphType>
using tightening_t =
std::add_pointer_t<void(const HypergraphType &, OrderingContext<typename HypergraphType::Heap> &, const int)>;

/* Given a method to update the "tightness" of different vertices, computes a
 * vertex ordering and returns the ordering as well as a list of how tight each
 * vertex was when it was added to the ordering.
 *
 * Time complexity: O(p), where p is the size of the hypergraph, assuming that
 * TIGHTEN runs in time linear to the number of edges incident on the selected
 * vertex.
 */
template<typename HypergraphType, tightening_t<HypergraphType> TIGHTEN>
std::pair<std::vector<int>, std::vector<double>>
unweighted_ordering(const HypergraphType &hypergraph, const int a) {
  std::vector<int> ordering = {a};
  std::vector<double> tightness = {0};
  std::vector<int> vertices_without_a;

  for (const auto v : hypergraph.vertices()) {
    if (v == a) {
      continue;
    }
    vertices_without_a.emplace_back(v);
  }

  // Multiply edges by 2 for Queyranne ordering
  OrderingContext<typename HypergraphType::Heap> ctx(vertices_without_a, 2 * hypergraph.num_edges() + 1);
  for (const auto &[e, vertices] : hypergraph.edges()) {
    ctx.edge_to_num_vertices_outside_ordering.insert({e, vertices.size()});
  }

  const auto tighten = [&hypergraph, &ctx](const int v) {
    ctx.used_vertices.insert(v);
    // It is the responsibility of TIGHTEN to update the context
    TIGHTEN(hypergraph, ctx, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.num_vertices()) {
    const auto[k, v] = ctx.heap.pop_key_val();
    ordering.emplace_back(v);
    // We need k / 2 instead of just k because this is just used for Queyranne
    // for now
    tightness.push_back(k / 2.0);
    tighten(v);
  }

  return {ordering, tightness};
}

/* Returns a maximum adjacency ordering of vertices, starting with vertex a.
 * Linear in the number of vertices across all hyperedges.
 *
 * Here, tightness for a vertex v is the number of edges that intersect the
 * ordering so far and v.
 */
template<typename HypergraphType>
inline std::vector<int> maximum_adjacency_ordering(const HypergraphType &hypergraph,
                                                   const int a) {
  return unweighted_ordering<HypergraphType, maximum_adjacency_ordering_tighten>(hypergraph, a).first;
}

/* Return a tight ordering of vertices, starting with vertex a. Linear in the
 * number of vertices across all hyperedges.
 *
 * Here, tightness is the number of edges connecting a vertex v to the
 * ordering so far that consist of vertices either in the ordering or v
 * itself.
 *
 * Takes time linear with the size of the hypergraph.
 */
template<typename HypergraphType>
inline std::vector<int> tight_ordering(const HypergraphType &hypergraph, const int a) {
  return unweighted_ordering<HypergraphType, tight_ordering_tighten>(hypergraph, a).first;
}

/* Return a Queyranne ordering of vertices, starting with vertex a.
 *
 * Takes time linear with the size of the hypergraph.
 */
template<typename HypergraphType>
inline std::vector<int> queyranne_ordering(const HypergraphType &hypergraph, const int a) {
  return unweighted_ordering<HypergraphType, queyranne_ordering_tighten>(hypergraph, a).first;
}

template<typename HypergraphType>
inline std::pair<std::vector<int>,
                 std::vector<double>> queyranne_ordering_with_tightness(const HypergraphType &hypergraph, const int a) {
  return unweighted_ordering<HypergraphType, queyranne_ordering_tighten>(hypergraph, a);
}

template<typename HypergraphType>
using ordering_t = std::add_pointer_t<std::vector<int>(const HypergraphType &, const int)>;

/* Given a hypergraph and a function that orders the vertices, find the min cut
 * by repeatedly finding and contracting pendant pairs.
 *
 * Time complexity: O(np), where n is the number of vertices and p is the size
 * of the hypergraph
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `maximum_adjacency_ordering`.
 */
template<typename HypergraphType, ordering_t<HypergraphType> Ordering>
size_t vertex_ordering_mincut(HypergraphType &hypergraph, const int a) {
  size_t min_cut_of_phase = std::numeric_limits<size_t>::max();
  while (hypergraph.num_vertices() > 1) {
    auto ordering = Ordering(hypergraph, a);
    size_t cut_of_phase = one_vertex_cut(hypergraph, ordering.back());
    hypergraph = merge_vertices(hypergraph, *(std::end(ordering) - 2),
                                *(std::end(ordering) - 1));
    min_cut_of_phase = std::min(min_cut_of_phase, cut_of_phase);
  }
  return min_cut_of_phase;
}

/* Given a hypergraph and a function that orders the vertices, find the minimum
 * cut through an exponential search on the minimum cuts of k-trimmed
 * certificates. See [CX'09] for more details.
 *
 * Time complexity: O(p + cn^2), where p is the size of the hypergraph, c is the
 * value of the minimum cut, and n is the number of vertices
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `maximum_adjacency_ordering`.
 */
template<ordering_t<Hypergraph> Ordering>
size_t vertex_ordering_mincut_certificate(Hypergraph &hypergraph, const int a) {
  KTrimmedCertificate gen(hypergraph);
  size_t k = 1;
  while (true) {
    // Copy hypergraph
    std::cout << "Trying k=" << k << std::endl;
    Hypergraph certificate = gen.certificate(k);
    std::cout << "Tried k=" << k << std::endl;
    size_t mincut = vertex_ordering_mincut<Hypergraph, Ordering>(certificate, a);
    if (mincut < k) {
      return mincut;
    }
    k *= 2;
  }
}
