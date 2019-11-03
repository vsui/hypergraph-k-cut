// This file implements vertex orderings for hypergraphs. See [CX'18]
// section 2.1 for more details.

#include "hypergraph/order.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hypergraph/bucket_list.hpp"
#include "hypergraph/hypergraph.hpp"

void maximum_adjacency_ordering_tighten(const Hypergraph &hypergraph,
                                        OrderingContext &ctx, const int v) {
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
    // to maintain the runtime.
    for (const int u : hypergraph.edges().at(e)) {
      if (ctx.used_vertices.find(u) == std::end(ctx.used_vertices)) {
        auto handle = ctx.handles.at(u);
        auto[key, val] = *handle;
        *handle = {key + 1, val};
        ctx.heap.increase(handle);
      }
    }
    ctx.used_edges.insert(e);
  }
}

void tight_ordering_tighten(const Hypergraph &hypergraph, OrderingContext &ctx,
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
          auto handle = ctx.handles.at(u);
          auto[key, val] = *handle;
          *handle = {key + 1, val};
          ctx.heap.increase(handle);
          break;
        }
      }
    }
  }
}

void queyranne_ordering_tighten(const Hypergraph &hypergraph,
                                OrderingContext &ctx, const int v) {
  maximum_adjacency_ordering_tighten(hypergraph, ctx, v);
  tight_ordering_tighten(hypergraph, ctx, v);
}

std::vector<int> maximum_adjacency_ordering(const Hypergraph &hypergraph,
                                            const int a) {
  return unweighted_ordering<maximum_adjacency_ordering_tighten>(hypergraph, a)
      .first;
}

std::vector<int> tight_ordering(const Hypergraph &hypergraph, const int a) {
  return unweighted_ordering<tight_ordering_tighten>(hypergraph, a).first;
}

std::vector<int> queyranne_ordering(const Hypergraph &hypergraph, const int a) {
  return unweighted_ordering<queyranne_ordering_tighten>(hypergraph, a).first;
}

// For a hypergraph with vertices V, returns the value of the cut V - {v}, {v}.
// Takes time linear to the number of vertices across all edges.
size_t one_vertex_cut(const Hypergraph &hypergraph, const int v) {
  size_t cut = hypergraph.edges_incident_on(v).size();
  return cut;
}

Hypergraph merge_vertices(const Hypergraph &hypergraph, const int s,
                          const int t) {
  const auto vs = {s, t};
  return hypergraph.contract(std::begin(vs), std::end(vs));
}
