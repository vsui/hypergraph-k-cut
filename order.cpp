// This file implements vertex orderings for hypergraphs. See CX section 2.1 for
// more details.

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
    std::unordered_set<int> used, std::unordered_set<int> ordered,
    const int v) {
  for (auto &[v, incident_edges] : vertices) {
    for (const int e : incident_edges) {
      if (used.find(e) != std::end(used)) {
        continue;
      }
      for (const int u : hypergraph.edges().at(e)) {
        if (ordered.find(u) == std::end(ordered)) {
          bheap.increment(u);
        }
      }
      used.insert(e);
    }
  }
}

void tight_ordering_tighten(
    const Hypergraph &hypergraph, BucketHeap &bheap,
    std::unordered_map<int, int> num_vertices_outside_ordering,
    std::unordered_set<int> ordered, const int v) {
  for (const int e : hypergraph.vertices().at(v)) {
    num_vertices_outside_ordering[e] -= 1;
    if (num_vertices_outside_ordering[e] == 1) {
      for (const int u : hypergraph.edges().at(e)) {
        if (ordered.find(u) == std::end(ordered)) {
          bheap.increment(v);
          break;
        }
      }
    }
  }
}

// Returns a maximum adjacency ordering of vertices, starting with vertex a.
// Linear in the number of vertices across all hyperedges.
//
// Here, tightness for a vertex v is the number of edges that intersect the
// ordering so far and v.
std::vector<int> maximum_adjacency_ordering(const Hypergraph &hypergraph,
                                            const int a) {
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
  std::unordered_set<int> ordered;

  auto tighten = [&hypergraph, &bheap, &vertices, &used,
                  &ordered](const int v) {
    maximum_adjacency_ordering_tighten(hypergraph, bheap, vertices, used,
                                       ordered, v);
    ordered.insert(v);
  };

  tighten(a);

  while (ordering.size() <= hypergraph.vertices().size()) {
    const int v = bheap.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}

// Return a tight ordering of vertices, starting with vertex a. Linear in the
// number of vertices across all hyperedges.
//
// Here, tightness is the number of edges connecting a vertex v to the
// ordering so far that consist of vertices either in the ordering or v
// itself.
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
    tight_ordering_tighten(hypergraph, bheap, num_vertices_outside_ordering,
                           ordered, v);
    ordered.insert(v);
  };

  tighten(a);

  while (ordering.size() <= hypergraph.vertices().size()) {
    const int v = bheap.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}

// Return a Queyranne ordering of vertices, starting with vertex a. Linear in the
// number of vertices across all hyperedges.
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
                  &num_vertices_outside_ordering, &used, &ordered](const int v) {
    maximum_adjacency_ordering_tighten(hypergraph, bheap, vertices, used, ordered, v);
    tight_ordering_tighten(hypergraph, bheap, num_vertices_outside_ordering,
                           ordered, v);
    ordered.insert(v);
  };

  tighten(a);

  while (ordering.size() <= hypergraph.vertices().size()) {
    const int v = bheap.pop();
    ordering.emplace_back(v);
    tighten(v);
  }

  return ordering;
}
