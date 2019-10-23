// This file implements vertex orderings for hypergraphs. See CX section 2.1 for
// more details.

#include "order.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bucket_list.h"
#include "hypergraph.h"

void maximum_adjacency_ordering_tighten(
    const Hypergraph &hypergraph, BucketList &blist,
    [[maybe_unused]] std::unordered_map<int, std::vector<int>>
        vertices /* necessary for interface */,
    std::unordered_set<int> &used, std::unordered_set<int> &ordered,
    const int v) {
  // We are adding v to the ordering so far, and need to update the keys for the
  // other vertices

  // Check each edge e incident on v
  for (const int e : hypergraph.vertices().at(v)) {
    // If e has already been used, then skip it
    if (used.find(e) != std::end(used)) {
      continue;
    }
    // For every vertex u in e that is not v and not already in the ordering,
    // increment its key (it is one edge tighter now)
    for (const int u : hypergraph.edges().at(e)) {
      if (ordered.find(u) == std::end(ordered)) {
        blist.increment(u);
      }
    }
    used.insert(e);
  }
}

void tight_ordering_tighten(
    const Hypergraph &hypergraph, BucketList &blist,
    std::unordered_map<int, size_t> &num_vertices_outside_ordering,
    std::unordered_set<int> &ordered, const int v) {
  // For every edge e incident on v
  for (const int e : hypergraph.vertices().at(v)) {
    // Tighten this edge
    num_vertices_outside_ordering.at(e) -= 1;
    // If the edge only has one vertex u left that is outside the ordering,
    // increase the key of u
    if (num_vertices_outside_ordering.at(e) == 1) {
      for (const int u : hypergraph.edges().at(e)) {
        if (ordered.find(u) == std::end(ordered)) {
          blist.increment(u);
          break;
        }
      }
    }
  }
}

std::vector<int> maximum_adjacency_ordering(const Hypergraph &hypergraph,
                                            const int a) {
  // Ordering starts with a
  std::vector<int> ordering = {a};
  std::vector<int> vertices_without_a;
  for (const auto &[v, incidence] : hypergraph.vertices()) {
    if (v == a) {
      continue;
    }
    vertices_without_a.emplace_back(v);
  }

  BucketList blist(vertices_without_a, hypergraph.edges().size() + 1);

  // Copy vertices and incidence lists because we need to modify them
  std::unordered_map<int, std::vector<int>> vertices(
      std::begin(hypergraph.edges()), std::end(hypergraph.edges()));

  // Tracks what edges have been used
  std::unordered_set<int> used;

  // Tracks what vertices have been added to the ordering
  std::unordered_set<int> ordered = {a};

  auto tighten = [&hypergraph, &blist, &vertices, &used,
                  &ordered](const int v) {
    ordered.insert(v);
    maximum_adjacency_ordering_tighten(hypergraph, blist, vertices, used,
                                       ordered, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.vertices().size()) {
    const int v = blist.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}

std::vector<int> tight_ordering(const Hypergraph &hypergraph, const int a) {
  std::vector<int> ordering = {a};
  std::vector<int> vertices_without_a;
  for (const auto &[v, incidence] : hypergraph.vertices()) {
    if (v == a) {
      continue;
    }
    vertices_without_a.emplace_back(v);
  }

  BucketList blist(vertices_without_a, hypergraph.edges().size() + 1);

  // Copy vertices and incidence lists because we need to modify them
  std::unordered_map<int, std::vector<int>> vertices(
      std::begin(hypergraph.edges()), std::end(hypergraph.edges()));

  // Tracks how many vertices each edge has that are outside the ordering
  std::unordered_map<int, size_t> num_vertices_outside_ordering;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    num_vertices_outside_ordering[e] = vertices.size();
  }

  // Tracks what vertices have been added to the ordering
  std::unordered_set<int> ordered;

  auto tighten = [&hypergraph, &blist, &num_vertices_outside_ordering,
                  &ordered](const int v) {
    ordered.insert(v);
    tight_ordering_tighten(hypergraph, blist, num_vertices_outside_ordering,
                           ordered, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.vertices().size()) {
    const int v = blist.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}

std::vector<int> queyranne_ordering(const Hypergraph &hypergraph, const int a) {
  std::vector<int> ordering = {a};
  std::vector<int> vertices_without_a;
  for (const auto &[v, incidence] : hypergraph.vertices()) {
    if (v == a) {
      continue;
    }
    vertices_without_a.emplace_back(v);
  }

  BucketList blist(vertices_without_a, 2 * hypergraph.edges().size() + 1);

  // Copy vertices and incidence lists because we need to modify them
  std::unordered_map<int, std::vector<int>> vertices(
      std::begin(hypergraph.edges()), std::end(hypergraph.edges()));

  // Tracks how many vertices each edge has that are outside the ordering
  std::unordered_map<int, size_t> num_vertices_outside_ordering;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    num_vertices_outside_ordering[e] = vertices.size();
  }

  // Tracks what vertices have been added to the ordering
  std::unordered_set<int> ordered;

  // Tracks what edges have been used
  std::unordered_set<int> used;

  auto tighten = [&hypergraph, &blist, &vertices,
                  &num_vertices_outside_ordering, &used,
                  &ordered](const int v) {
    ordered.insert(v);
    maximum_adjacency_ordering_tighten(hypergraph, blist, vertices, used,
                                       ordered, v);
    tight_ordering_tighten(hypergraph, blist, num_vertices_outside_ordering,
                           ordered, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.vertices().size()) {
    const int v = blist.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}

// For a hypergraph with vertices V, returns the value of the cut V - {v}, {v}.
// Takes time linear to the number of vertices across all edges.
size_t one_vertex_cut(const Hypergraph &hypergraph, const int v) {
  size_t cut = 0;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    if (std::find(std::cbegin(vertices), std::cend(vertices), v) !=
        std::cend(vertices)) {
      cut += 1;
    }
  }
  return cut;
}

Hypergraph merge_vertices(const Hypergraph &hypergraph, const int s,
                          const int t) {
  // TODO if we have a non-const contract then this copy is unnecessary. Right
  // now we copy twice (once to avoid modifying the input hypergraph and the
  // second time to contract)
  Hypergraph copy(hypergraph);

  const int new_e =
      std::max_element(
          std::begin(hypergraph.edges()), std::end(hypergraph.edges()),
          [](const auto &a, const auto &b) { return a.first < b.first; })
          ->first +
      1;

  // Add edge (s, t) and contract it
  copy.edges().insert({new_e, {s, t}});
  copy.vertices().at(s).push_back(new_e);
  copy.vertices().at(t).push_back(new_e);

  return copy.contract(new_e);
}
