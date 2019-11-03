#pragma once

#include <unordered_map>
#include <vector>
#include <numeric>

#include <boost/range/adaptors.hpp>

class KTrimmedCertificate;

class Hypergraph {
public:
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

private:
  friend class KTrimmedCertificate;

  // Constructor that directly sets adjacency lists and next vertex ID
  Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
             std::unordered_map<int, std::vector<int>> &&edges,
             int next_vertex_id, int next_edge_id);

  // Map of vertex IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> vertices_;

  // Map of edge IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> edges_;

  int next_vertex_id_;
  int next_edge_id_;
};

/* A hypergraph with weighted edges. The time complexities of all operations are the same as with the unweighted
 * hypergraph.
 */
template<typename EdgeWeightType>
class WeightedHypergraph {
  // The reason this is implemented with composition instead of inheritance is that many algorithms need to be aware of
  // exhibit significantly different behavior in the weighted vs. unweighted case.
public:
  WeightedHypergraph() = delete;

  explicit WeightedHypergraph(const Hypergraph &hypergraph) : hypergraph_(hypergraph) {
    for (const auto &[edge_id, incident_on] : hypergraph.edges()) {
      edges_to_weights_.insert({edge_id, 1});
    }
  }

  WeightedHypergraph(const std::vector<int> &vertices,
                     const std::vector<std::pair<std::vector<int>, EdgeWeightType>> edges) {
    std::vector<std::vector<int>> edges_without_weights;
    std::transform(std::begin(edges), std::end(edges), std::back_inserter(edges_without_weights), [](const auto &pair) {
      return pair.first;
    });
    hypergraph_ = Hypergraph(vertices, edges_without_weights);
    for (size_t i = 0; i < edges.size(); ++i) {
      const auto[it, inserted] = edges_to_weights_.insert({i, edges.at(i).second});
#ifndef NDEBUG
      assert(inserted);
#endif
    }
  }

  [[nodiscard]] size_t num_vertices() const { return hypergraph_.num_vertices(); }
  [[nodiscard]] size_t num_edges() const { return hypergraph_.num_edges(); }

  using vertex_range = Hypergraph::vertex_range;

  vertex_range vertices() { return hypergraph_.vertices(); }
  [[nodiscard]] const std::vector<int> &edges_incident_on(int vertex_id) const {
    return hypergraph_.edges_incident_on(vertex_id);
  }

  [[nodiscard]] const std::unordered_map<int, std::vector<int>> &edges() const {
    return hypergraph_.edges();
  }

  [[nodiscard]] bool is_valid() const {
    return hypergraph_.is_valid();
  }

  WeightedHypergraph contract(int edge_id) const {
    Hypergraph contracted = hypergraph_.contract(edge_id);
    // TODO edges to weights should still be valid
    return WeightedHypergraph(contracted, edges_to_weights_);
  }

  EdgeWeightType edge_weight(int edge_id) const {
    return edges_to_weights_.at(edge_id);
  }

  template<typename InputIt>
  int add_hyperedge(InputIt begin, InputIt end, EdgeWeightType weight) {
    auto id = hypergraph_.add_hyperedge(begin, end);
    const auto[inserted, it] = edges_to_weights_.insert({id, weight});
#ifndef NDEBUG
    assert(inserted);
#endif
  }

  void remove_hyperedge(int edge_id) {
    // Edge weights from edges that were subsets of the removed edge are not removed
    hypergraph_.remove_hyperedge();
    edges_to_weights_.erase(edge_id);
  }

  template<typename InputIt>
  Hypergraph contract(InputIt begin, InputIt end) const {
    Hypergraph copy(*this);
    auto new_e = copy.add_hyperedge(begin, end, {}); // Doesn't matter what weight you add
    return copy.contract(new_e);
  }

private:
  WeightedHypergraph(const Hypergraph &hypergraph, const std::unordered_map<int, EdgeWeightType> edge_weights) :
      hypergraph_(hypergraph), edges_to_weights_(edge_weights) {}

  Hypergraph hypergraph_;

  std::unordered_map<int, EdgeWeightType> edges_to_weights_;
};

template<typename HypergraphType, typename EdgeWeightType = size_t>
inline EdgeWeightType edge_weight(const HypergraphType &hypergraph, int edge_id) {
  static_assert(
      std::is_same_v<HypergraphType, Hypergraph> || std::is_same_v<HypergraphType, WeightedHypergraph<EdgeWeightType>>);
  if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
    return 1;
  } else {
    return hypergraph.edge_weight(edge_id);
  }
}

/* Return the cost of all edges of a hypergraph combined
 */
template<typename HypergraphType, typename EdgeWeightType = size_t>
inline EdgeWeightType total_edge_weight(const HypergraphType &hypergraph) {
  // TODO could probably optimize for weighted hypergraphs by caching the weight, but then we would have to be a lot
  //      more careful about keeping edge_to_weights_ completely accurate
  static_assert(
      std::is_same_v<HypergraphType, Hypergraph> || std::is_same_v<HypergraphType, WeightedHypergraph<EdgeWeightType>>);
  if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
    return hypergraph.edges().size();
  } else {
    return std::accumulate(hypergraph.edges().begin(), hypergraph.edges().end(), EdgeWeightType(0), [&hypergraph](
        const EdgeWeightType accum,
        const auto &a) {
      return accum + hypergraph.edge_weight(a.first);
    });
  }
}

std::istream &operator>>(std::istream &is, Hypergraph &hypergraph);

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph);
