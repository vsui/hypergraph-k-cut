#pragma once

#include <iostream>
#include <vector>
#include <map>

template<typename T>
class HypergraphBase {
  friend T;
  friend class KTrimmedCertificate;
public:
  using Heap = BucketHeap;
  using EdgeWeight = size_t;
  using vertex_range = decltype(boost::adaptors::keys(std::unordered_map<int, std::vector<int>>{}));

  HypergraphBase() = default;

  HypergraphBase(const HypergraphBase &other) = default;

  HypergraphBase(const std::vector<int> &vertices,
                 const std::vector<std::vector<int>> &edges) :
      next_vertex_id_(*std::max_element(std::begin(vertices), std::end(vertices)) + 1),
      next_edge_id_(static_cast<int>(edges.size())) {
    assert(vertices.size() > 0);
    for (const int v : vertices) {
      vertices_[v] = {};
      vertices_within_[v] = {v};
    }
    int e_i = -1;
    for (const auto &incident_vertices : edges) {
      edges_[++e_i] = incident_vertices;
      for (const int u : incident_vertices) {
        vertices_[u].push_back(e_i);
      }
    }
  }

  HypergraphBase &operator=(const HypergraphBase &other) = default;

  /**
   * Determines whether two hypergraphs have the same vertices and hyperedges with the same labels. Does NOT determine
   * whether they are isomorphic.
   *
   * @param other
   * @return
   */
  bool operator==(const HypergraphBase &other) const {
    return vertices_ == other.vertices_ && edges_ == other.edges_;
  }

  [[nodiscard]]
  size_t num_vertices() const { return vertices_.size(); };

  [[nodiscard]]
  size_t num_edges() const { return edges_.size(); }

  [[nodiscard]]
  vertex_range vertices() const {
    return boost::adaptors::keys(vertices_);
  };

  [[nodiscard]]
  const std::vector<int> &edges_incident_on(int vertex_id) const {
    return vertices_.at(vertex_id);
  }

  [[nodiscard]]
  const std::unordered_map<int, std::vector<int>> &edges() const {
    return edges_;
  }

  [[nodiscard]]
  size_t rank() const {
    return std::max_element(edges().begin(), edges().end(), [](const auto a, const auto b) {
      return a.second.size() < b.second.size();
    })->second.size();
  }

  void remove_singleton_and_empty_hyperedges() {
    while (true) {
      auto it = std::find_if(std::begin(edges()), std::end(edges()), [](auto &&pair) {
        return pair.second.size() < 2;
      });
      if (it == std::end(edges())) {
        return;
      }
      remove_hyperedge(it->first);
    }
  }

  [[nodiscard]]
  size_t degree(const int vertex_id) const {
    auto it = vertices_.find(vertex_id);
    assert(it != vertices_.end());
    return it->second.size();
  }

  /* Checks that the internal state of the hypergraph is consistent. Mainly for
   * debugging.
   */
  [[nodiscard]]
  bool is_valid() const {

    for (const auto &[v, incidence] : vertices_) {
      for (const int e : incidence) {
        const auto &vertices_incident_on = edges_.at(e);
        if (std::find(vertices_incident_on.begin(), vertices_incident_on.end(),
                      v) == std::end(vertices_incident_on)) {
          std::cerr << "ERROR: edge " << e << " should contain vertex " << v
                    << std::endl;
          return false;
        }
      }
    }

    for (const auto &[e, incidence] : edges_) {
      for (const int v : incidence) {
        const auto &edges_incident_on = vertices_.at(v);
        if (std::find(edges_incident_on.begin(), edges_incident_on.end(), e) ==
            std::end(edges_incident_on)) {
          std::cerr << "ERROR: vertex " << v << " should contain edge " << e
                    << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  /* Returns a new hypergraph with the edge contracted. Assumes that there is
   * an edge in the hypergraph with the given edge ID.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  template<bool EdgeMayContainLoops = true, bool TrackContractedVertices = true>
  [[nodiscard]]
  T contract(const int edge_id) const {
    // TODO EdgeMayContainLoops default true is a bit aggressive and may lead to performance regressions,
    //      we should try to avoid setting it to true if possible
    std::vector<int> old_edge = edges_.at(edge_id);

    if (old_edge.empty()) {
      // Pretty much nothing will change, just remove the old edge
      auto new_vertices = vertices_;
      auto new_edges = edges_;
      new_edges.erase(edge_id);
      return T(std::move(new_vertices), std::move(new_edges), static_cast<const T &>(*this));
    }

    if constexpr (EdgeMayContainLoops) {
      std::set<int> sorted(std::begin(old_edge), std::end(old_edge));
      old_edge = {std::begin(sorted), std::end(sorted)};
    }

    // TODO verify that inserts actually insert (not overwrite)
    // Set V' := V \ e (do not copy incidence lists)
    std::unordered_map<int, std::vector<int>> new_vertices;
    new_vertices.reserve(vertices_.size());
    for (const auto &[id, incidence] : vertices_) {
      new_vertices[id] = {};
    }
    for (const int id : edges_.at(edge_id)) {
      new_vertices.erase(id);
    }

    // Set E' := E \ { e } (do not copy incidence lists)
    std::unordered_map<int, std::vector<int>> new_edges;
    new_edges.reserve(edges_.size());
    for (const auto &[id, incidence] : edges_) {
      new_edges[id] = {};
    }
    new_edges.erase(edge_id);

    // for v' in V'
    for (auto &[v, v_edges] : new_vertices) {
      // for e' in E' incident to v in H
      for (const int e : vertices_.at(v)) {
        auto &e_vertices = new_edges.at(e);
        // add v' to the incidence list of e'
        e_vertices.push_back(v);
        // add e' to the incidence list of v'
        v_edges.push_back(e);
      }
    }

    // Remove any e' with empty incidence list (it was a subset of the removed
    // hyperedge)
    for (auto it = std::begin(new_edges); it != std::end(new_edges);) {
      if (it->second.empty()) {
        it = new_edges.erase(it);
      } else {
        ++it;
      }
    }

    // Now add a new vertex v_e replacing the removed hyperedge
    int v_e = next_vertex_id_;
    new_vertices[v_e] = {};

    // Add v_e to all hyperedges with less edges and vice versa
    for (auto &[e, new_incident_on] : new_edges) {
      const auto &old_incident_on = edges_.at(e);

      if (new_incident_on.size() < old_incident_on.size()) {
        new_incident_on.push_back(v_e);
        new_vertices[v_e].push_back(e);
      }
    }

    T new_hypergraph(std::move(new_vertices), std::move(new_edges), static_cast<const T &>(*this));
    new_hypergraph.next_vertex_id_++;

    if constexpr (TrackContractedVertices) {
      // Update vertices_within_ of new hypergraph
      std::list<int> edges_within_new_v;
      for (const auto v : old_edge) {
        edges_within_new_v.splice(std::end(edges_within_new_v), std::move(new_hypergraph.vertices_within_.at(v)));
        new_hypergraph.vertices_within_.erase(v);
      }
      new_hypergraph.vertices_within_.insert({v_e, edges_within_new_v});
    }

    return new_hypergraph;
  }

  template<bool EdgeMayContainLoops = true, bool TrackContractedVertices = true>
  void contract_in_place(const int edge_id) {
    assert(is_valid());
    if (edges_.at(edge_id).empty()) {
      edges_.erase(edge_id);
      return;
    }

    std::vector<int> edge = edges_.at(edge_id);
    if constexpr (EdgeMayContainLoops) {
      std::set<int> sorted(std::begin(edge), std::end(edge));
      edge = {std::begin(sorted), std::end(sorted)};
    }

    for (auto v : edge) {
      vertices_.erase(v);
    }
    edges_.erase(edge_id);

    // Remove edge_id from incidence of all v in e
    for (auto &[v, edges] : vertices_) {
      edges.erase(std::remove(std::begin(edges), std::end(edges), edge_id), std::end(edges));
    }

    auto new_v = next_vertex_id_++;
    vertices_[new_v] = {};

    if constexpr (TrackContractedVertices) {
      for (const auto v : edge) {
        vertices_within_[new_v].splice(std::end(vertices_within_[new_v]), std::move(vertices_within_[v]));
        vertices_within_.erase(v);
      }
    }

    // Remove v from all edges such that v in edge
    for (auto it = std::begin(edges_); it != std::end(edges_);) {
      auto e = it->first;
      auto &vertices = it->second;
      auto old_size = vertices.size();
      for (auto v : edge) {
        vertices.erase(std::remove(std::begin(vertices), std::end(vertices), v), std::end(vertices));
      }
      if (vertices.empty()) {
        it = edges_.erase(it);
      } else {
        if (old_size != vertices.size()) {
          vertices.push_back(new_v);
          vertices_[new_v].push_back(e);
        }
        ++it;
      }
    }
  }

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
  void remove_hyperedge(int edge_id) {
    for (const auto v : edges_.at(edge_id)) {
      auto &vertex_incidence_list = vertices_.at(v);
      auto it = std::find(std::begin(vertex_incidence_list),
                          std::end(vertex_incidence_list), edge_id);
      // Swap it with the last element and pop it off to remove in O(1) time
      std::iter_swap(it, std::end(vertex_incidence_list) - 1);
      vertex_incidence_list.pop_back();
    }
    edges_.erase(edge_id);
  }

  void remove_vertex(int vertex_id) {
    std::vector<int> invalid_edges;
    auto remove_vertex_from_edge = [vertex_id, &invalid_edges, this](const int edge_id) {
      auto &edge = edges_.at(edge_id);
      edge.erase(std::remove(std::begin(edge), std::end(edge), vertex_id));
      if (edge.size() < 2) {
        invalid_edges.push_back(edge_id);
      }
    };

    auto it = vertices_.find(vertex_id);
    assert(it != vertices_.end());

    auto &edges = it->second;

    std::for_each(std::begin(edges), std::end(edges), remove_vertex_from_edge);
    std::for_each(std::begin(invalid_edges), std::end(invalid_edges), [this](const int id) { remove_hyperedge(id); });

    vertices_.erase(vertex_id);
  }

  /**
   * Return the combined size of all hyperedges
   */
  size_t size() const {
    size_t ret = 0;
    for (const auto &[id, edge] : edges()) {
      ret += edge.size();
    }
    return ret;
  }

  /* Contracts the vertices in the range into one vertex.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  template<bool EdgesMayContainLoops, bool TrackContractedVertices, typename InputIt>
  [[nodiscard]]
  T contract(InputIt begin, InputIt end) const {
    // TODO if we have a non-const contract then this copy is unnecessary. Right
    // now we copy twice (once to avoid modifying the input hypergraph and the
    // second time to contract)
    HypergraphBase copy(*this);
    auto new_e = copy.add_hyperedge(begin, end);
    return copy.contract<EdgesMayContainLoops, TrackContractedVertices>(new_e);
  }

  /* When an edge is contracted into a single vertex the original vertices in the edge can be stored and referred to
   * later using this method. This is useful for calculating the actual partitions that make up the cuts.
   */
  [[nodiscard]]
  const std::list<int> &vertices_within(const int v) const {
    return vertices_within_.at(v);
  }

private:
  // Constructor that directly sets adjacency lists and next vertex ID (assuming that you just contracted an edge)
  HypergraphBase(std::unordered_map<int, std::vector<int>> &&vertices,
                 std::unordered_map<int, std::vector<int>> &&edges,
                 const HypergraphBase &old)
      : vertices_(vertices), edges_(edges), next_vertex_id_(old.next_vertex_id_), next_edge_id_(old.next_edge_id_),
        vertices_within_(old.vertices_within_) {}

  // Map of vertex IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> vertices_;

  // Map of edge IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> edges_;

  // Map of vertex IDs -> the vertices that have been contracted into this vertex.
  // Note that in the interest of performance, old entries may not be deleted (but each entry is immutable)
  std::unordered_map<int, std::list<int>> vertices_within_;

  int next_vertex_id_;
  int next_edge_id_;
};

