#pragma once

#include <unordered_map>
#include <vector>

#include <boost/range/adaptors.hpp>

class KTrimmedCertificate;

class Hypergraph {
public:
  virtual ~Hypergraph() = default;

  Hypergraph &operator=(const Hypergraph &other);

  Hypergraph();

  Hypergraph(const Hypergraph &other);

  Hypergraph(const std::vector<int> &vertices,
             const std::vector<std::vector<int>> &edges);

  [[nodiscard]] size_t num_vertices() const;

  [[nodiscard]] size_t num_edges() const;

  using vertex_range = decltype(boost::adaptors::keys(std::unordered_map<int, std::vector<int>>{}));

  [[nodiscard]] vertex_range vertices() const;

  [[nodiscard]] const std::vector<int> &edges_incident_on(int vertex_id) const;

  [[nodiscard]] const std::unordered_map<int, std::vector<int>> &edges() const;

  /* Checks that the internal state of the hypergraph is consistent. Mainly for
   * debugging.
   */
  [[nodiscard]] bool is_valid() const;

  /* Returns a new hypergraph with the edge contracted. Assumes that there is
   * an edge in the hypergraph with the given edge ID.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  [[nodiscard]] Hypergraph contract(const int edge_id) const;

  /* Add hyperedge and return its ID.
   *
   * Time complexity: O(n), where n is the number of vertices in the hyperedge
   */
  template<typename InputIt>
  int add_hyperedge(InputIt begin, InputIt end) {
    std::vector new_edge(begin, end);
    auto new_edge_id = next_edge_id_;

    edges_.insert({new_edge_id, new_edge});

    for (const auto v : new_edge) {
      // TODO IDs should be unsigned
      vertices_.at(v).push_back(new_edge_id);
    }

    ++next_edge_id_;
    return new_edge_id;
  }

  /* Remove a hyperedge.
   *
   * Time complexity: linear with the size of all vertices contained by the hyperedge
   */
  void remove_hyperedge(int edge_id);

  /* Contracts the vertices in the range into one vertex.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  template<typename InputIt>
  [[nodiscard]] Hypergraph contract(InputIt begin, InputIt end) const {
    assert(edges().size() > 0);

    // TODO if we have a non-const contract then this copy is unnecessary. Right
    // now we copy twice (once to avoid modifying the input hypergraph and the
    // second time to contract)
    Hypergraph copy(*this);
    auto new_e = copy.add_hyperedge(begin, end);
    return copy.contract(new_e);
  }

  [[nodiscard]] virtual size_t weight(int e) const {
    if (edge_weights_.find(e) == std::end(edge_weights_)) {
      return 1;
    }
    return edge_weights_.at(e);
  }

private:
  friend class KTrimmedCertificate;

  // Constructor that directly sets adjacency lists and next vertex ID
  Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
             std::unordered_map<int, std::vector<int>> &&edges,
             int next_vertex_id, int next_edge_id);

  std::unordered_map<int, size_t> edge_weights_;

  // Map of vertex IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> vertices_;

  // Map of edge IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> edges_;

  int next_vertex_id_;
  int next_edge_id_;
};

class UnweightedHypergraph : public Hypergraph {
public:
  virtual ~UnweightedHypergraph() = default;

  [[nodiscard]] size_t weight([[maybe_unused]] int e) const { return 1; }
};

std::istream &operator>>(std::istream &is, Hypergraph &hypergraph);

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph);