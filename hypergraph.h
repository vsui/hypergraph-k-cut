#pragma once

#include <unordered_map>

class Hypergraph {
public:
  Hypergraph &operator=(const Hypergraph &other);
  Hypergraph();
  Hypergraph(const Hypergraph &other);

  Hypergraph(const int num_edges, const std::vector<std::vector<int>> &edges);

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
   */
  Hypergraph contract(const int edge_id) const;

private:
  // Map of vertex IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> vertices_;

  // Map of edge IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> edges_;

  mutable int next_vertex_id_;

  // Constructor that directly sets adjacency lists and next vertex ID
  Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
             std::unordered_map<int, std::vector<int>> &&edges,
             int next_vertex_id);
};

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph);
