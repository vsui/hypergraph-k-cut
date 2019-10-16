// This file implements vertex orderings for hypergraphs. See CX section 2.1 for
// more details.

#include <algorithm>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hypergraph.h"

// An important data structure for obtaining the vertex orderings of
// uncapacitated hypergraphs. It is a collection of distinct values, each with
// an associated key that is initially zero. The key of a value can be
// incremented in constant time, and a value with a max key can be popped from
// the data structure in linear time.
class BucketHeap {
public:
  // Create a BucketHeap with the given values and capacity (number of buckets).
  BucketHeap(std::vector<int> values, const int capacity)
      : capacity_(capacity), buckets_(capacity), max_key_(0) {
    for (const int value : values) {
      buckets_.front().emplace_front(value);
      val_to_keys_.insert({value, 0});
      val_to_its_.insert({value, std::begin(buckets_.front())});
    }
  }

  // Increment the key of the value. Takes constant time.
  void increment(const int value) {
    const int old_key = val_to_keys_.at(value);
    const int new_key = ++val_to_keys_.at(value);
    assert(new_key < capacity_);
    buckets_.at(old_key).erase(val_to_its_.at(value));
    if (new_key > max_key_) {
      max_key_ = new_key;
    }
    buckets_.at(new_key).emplace_front(value);
    val_to_its_.at(value) = std::begin(buckets_.at(new_key));
  }

  // Pop one of the values with the maximum key. Takes time linear to the number
  // of buckets.
  int pop() {
    assert(max_key_ > -1);
    while (buckets_.at(max_key_).empty()) {
      --max_key_;
    }
    const int value = buckets_.at(max_key_).front();
    buckets_.at(max_key_).pop_front();
    val_to_keys_.erase(value);
    val_to_its_.erase(value);
    return value;
  }

private:
  const int capacity_;

  // buckets_[i] is a collection of all values with key i
  std::vector<std::list<int>> buckets_;

  // A mapping of values to their keys
  std::unordered_map<int, int> val_to_keys_;

  // A mapping of values to their iterators (for fast deletion)
  std::unordered_map<int, std::list<int>::iterator> val_to_its_;

  // The current maximum key
  int max_key_;
};

void maximum_adjacency_ordering_tighten(
    const Hypergraph &hypergraph, BucketHeap &bheap,
    std::unordered_map<int, std::vector<int>> vertices,
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
        bheap.increment(u);
      }
    }
    used.insert(e);
  }
}

void tight_ordering_tighten(
    const Hypergraph &hypergraph, BucketHeap &bheap,
    std::unordered_map<int, int> &num_vertices_outside_ordering,
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
          bheap.increment(u);
          break;
        }
      }
    }
  }
}

#include <iostream>

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

  BucketHeap bheap(vertices_without_a, hypergraph.edges().size() + 1);

  // Copy vertices and incidence lists because we need to modify them
  std::unordered_map<int, std::vector<int>> vertices(
      std::begin(hypergraph.edges()), std::end(hypergraph.edges()));

  // Tracks what edges have been used
  std::unordered_set<int> used;

  // Tracks what vertices have been added to the ordering
  std::unordered_set<int> ordered = {a};

  auto tighten = [&hypergraph, &bheap, &vertices, &used,
                  &ordered](const int v) {
    ordered.insert(v);
    maximum_adjacency_ordering_tighten(hypergraph, bheap, vertices, used,
                                       ordered, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.vertices().size()) {
    const int v = bheap.pop();
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

  BucketHeap bheap(vertices_without_a, hypergraph.edges().size() + 1);

  // Copy vertices and incidence lists because we need to modify them
  std::unordered_map<int, std::vector<int>> vertices(
      std::begin(hypergraph.edges()), std::end(hypergraph.edges()));

  // Tracks how many vertices each edge has that are outside the ordering
  std::unordered_map<int, int> num_vertices_outside_ordering;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    num_vertices_outside_ordering[e] = vertices.size();
  }

  // Tracks what vertices have been added to the ordering
  std::unordered_set<int> ordered;

  auto tighten = [&hypergraph, &bheap, &vertices,
                  &num_vertices_outside_ordering, &ordered](const int v) {
    ordered.insert(v);
    tight_ordering_tighten(hypergraph, bheap, num_vertices_outside_ordering,
                           ordered, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.vertices().size()) {
    const int v = bheap.pop();
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

  BucketHeap bheap(vertices_without_a, hypergraph.edges().size() + 1);

  // Copy vertices and incidence lists because we need to modify them
  std::unordered_map<int, std::vector<int>> vertices(
      std::begin(hypergraph.edges()), std::end(hypergraph.edges()));

  // Tracks how many vertices each edge has that are outside the ordering
  std::unordered_map<int, int> num_vertices_outside_ordering;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    num_vertices_outside_ordering[e] = vertices.size();
  }

  // Tracks what vertices have been added to the ordering
  std::unordered_set<int> ordered;

  // Tracks what edges have been used
  std::unordered_set<int> used;

  auto tighten = [&hypergraph, &bheap, &vertices,
                  &num_vertices_outside_ordering, &used,
                  &ordered](const int v) {
    ordered.insert(v);
    maximum_adjacency_ordering_tighten(hypergraph, bheap, vertices, used,
                                       ordered, v);
    tight_ordering_tighten(hypergraph, bheap, num_vertices_outside_ordering,
                           ordered, v);
  };

  tighten(a);

  while (ordering.size() < hypergraph.vertices().size()) {
    const int v = bheap.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}

// For a hypergraph with vertices V, returns the value of the cut V - {v}, {v}.
// Takes time linear to the number of vertices across all edges.
int one_vertex_cut(const Hypergraph &hypergraph, const int v) {
  int cut = 0;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    if (std::find(std::cbegin(vertices), std::cend(vertices), v) !=
        std::cend(vertices)) {
      cut += 1;
    }
  }
  return cut;
}

// Return a new hypergraph with vertices s and t merged. Takes time linear to
// the number of certices across all edges.
Hypergraph merge_vertices(const Hypergraph &hypergraph, const int s,
                          const int t) {
  const int new_v =
      std::max_element(
          std::cbegin(hypergraph.vertices()), std::cend(hypergraph.vertices()),
          [](const auto &a, const auto &b) { return a.first < b.first; })
          ->first;

  std::vector<std::vector<int>> new_edges;

  for (const auto &[e, vertices] : hypergraph.edges()) {
    std::vector<int> new_edge;
    bool add_new = false;
    for (const int v : vertices) {
      if (v == s || v == t) {
        add_new = true;
      } else {
        new_edge.push_back(v);
      }
    }
    if (add_new) {
      new_edge.push_back(new_v);
    }
    if (new_edge.size() > 1) {
      new_edges.push_back(std::move(new_edge));
    }
  }

  std::vector<int> new_vertices;
  for (const auto &[v, incident_edges] : hypergraph.vertices()) {
    if (v == s || v == t) {
      continue;
    }
    new_vertices.push_back(v);
  }
  new_vertices.push_back(new_v);
  return Hypergraph(new_vertices, new_edges);
}

// Returns the induced head ordering of the edges
std::vector<int> induced_head_ordering(const Hypergraph &hypergraph,
                                       std::vector<int> vertex_ordering) {
  std::unordered_map<int, int> vertex_to_order;
  for (int i = 0; i < vertex_ordering.size(); ++i) {
    vertex_to_order[vertex_ordering[i]] = i;
  }

  std::vector<std::vector<int>> buckets(vertex_ordering.size() + 1);

  for (const auto &[e, vertices] : hypergraph.edges()) {
    const int head = *std::min_element(
        std::begin(vertices), std::end(vertices),
        [&vertex_to_order](const auto a, const auto b) {
          return vertex_to_order.at(a) < vertex_to_order.at(b);
        });
    buckets.at(head).push_back(e);
  }

  std::vector<int> edge_ordering;
  for (const auto &bucket : buckets) {
    edge_ordering.insert(std::end(edge_ordering), std::begin(bucket),
                         std::end(bucket));
  }

  return edge_ordering;
}

