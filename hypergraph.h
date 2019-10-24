#pragma once

#include <unordered_map>
#include <vector>

class Hypergraph {
public:
  Hypergraph &operator=(const Hypergraph &other);
  Hypergraph();
  Hypergraph(const Hypergraph &other);

  Hypergraph(const int num_edges, const std::vector<std::vector<int>> &edges);

  // Constructor that directly sets adjacency lists and next vertex ID
  Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
             std::unordered_map<int, std::vector<int>> &&edges,
             int next_vertex_id);

  Hypergraph(const std::vector<int> &vertices,
             const std::vector<std::vector<int>> &edges);

  size_t num_vertices() const;
  size_t num_edges() const;

  std::unordered_map<int, std::vector<int>> &vertices();
  std::unordered_map<int, std::vector<int>> &edges();
  const std::unordered_map<int, std::vector<int>> &vertices() const;
  const std::unordered_map<int, std::vector<int>> &edges() const;

  /* Checks that the internal state of the hypergraph is consistent. Mainly for
   * debugging.
   */
  bool is_valid() const;

  /* Returns a new hypergraph with the edge contracted. Assumes that there is
   * an edge in the hypergraph with the given edge ID.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  Hypergraph contract(const int edge_id) const;

  /* Contracts the vertices in the range into one vertex.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  template <typename InputIt>
  Hypergraph contract(InputIt begin, InputIt end) const {
    assert(edges().size() > 0);
    // add_edge and add_vertex to avoid statements like this
    const int new_e = std::max_element(std::begin(edges()), std::end(edges()),
                                       [](const auto &a, const auto &b) {
                                         return a.first < b.first;
                                       })
                          ->first +
                      1;

    std::vector<int> new_edge(begin, end);

    // TODO if we have a non-const contract then this copy is unnecessary. Right
    // now we copy twice (once to avoid modifying the input hypergraph and the
    // second time to contract)
    Hypergraph copy(*this);
    copy.edges().insert({new_e, new_edge});
    for (auto it = begin; it != end; ++it) {
      copy.vertices().at(*it).push_back(new_e);
    }

    return copy.contract(new_e);
  }

private:
  // Map of vertex IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> vertices_;

  // Map of edge IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> edges_;

  mutable int next_vertex_id_;
};

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph);
