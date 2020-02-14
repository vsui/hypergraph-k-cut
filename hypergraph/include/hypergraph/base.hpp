#pragma once

class HypergraphBase {
public:
  using Heap = BucketHeap;
  using EdgeWeight = size_t;
  using vertex_range = decltype(boost::adaptors::keys(std::unordered_map<int, std::vector<int>>{}));

  HypergraphBase &operator=(const HypergraphBase &other);

  HypergraphBase();

  HypergraphBase(const HypergraphBase &other);

  HypergraphBase(const std::vector<int> &vertices,
                 const std::vector<std::vector<int>> &edges);

  /**
   * Determines whether two hypergraphs have the same vertices and hyperedges
   *
   * @param other
   * @return
   */
  bool operator==(const HypergraphBase &other) const {
    return vertices_ == other.vertices_ && edges_ == other.edges_;
  }

  [[nodiscard]] size_t num_vertices() const;

  [[nodiscard]] size_t num_edges() const;

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
  template<bool EdgeMayContainLoops = true> // TODO check rank and set as necessary
  [[nodiscard]]
  HypergraphBase contract(const int edge_id) const {
    std::vector<int> old_edge = edges_.at(edge_id);

    if (old_edge.empty()) {
      // Pretty much nothing will change, just remove the old edge
      auto new_vertices = vertices_;
      auto new_edges = edges_;
      new_edges.erase(edge_id);
      return HypergraphBase(std::move(new_vertices), std::move(new_edges), *this);
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

    HypergraphBase new_hypergraph(std::move(new_vertices), std::move(new_edges), *this);
    new_hypergraph.next_vertex_id_++;

    // Update vertices_within_ of new hypergraph
    std::list<int> edges_within_new_v;
    for (const auto v : old_edge) {
      edges_within_new_v.splice(std::end(edges_within_new_v), std::move(new_hypergraph.vertices_within_.at(v)));
      new_hypergraph.vertices_within_.erase(v);
    }
    new_hypergraph.vertices_within_.insert({v_e, edges_within_new_v});

    return new_hypergraph;
  }

  template<bool EdgeMayContainLoops = true>
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

    for (const auto v : edge) {
      vertices_within_[new_v].splice(std::end(vertices_within_[new_v]), std::move(vertices_within_[v]));
      vertices_within_.erase(v);
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
  void remove_hyperedge(int edge_id);

  /* Contracts the vertices in the range into one vertex.
   *
   * Time complexity: O(p), where p is the size of the hypergraph.
   */
  template<typename InputIt>
  [[nodiscard]] HypergraphBase contract(InputIt begin, InputIt end) const {
    // TODO if we have a non-const contract then this copy is unnecessary. Right
    // now we copy twice (once to avoid modifying the input hypergraph and the
    // second time to contract)
    HypergraphBase copy(*this);
    auto new_e = copy.add_hyperedge(begin, end);
    return copy.contract(new_e);
  }

  /* When an edge is contracted into a single vertex the original vertices in the edge can be stored and referred to
   * later using this method. This is useful for calculating the actual partitions that make up the cuts.
   */
  [[nodiscard]]
  const std::list<int> &vertices_within(const int v) const;

  /**
   * Compute the rank of the hypergraph by scanning through all hyperedges and returning the size of the hyperedge with
   * maximum rank.
   *
   * @return the rank of the hypergraph
   */
  [[nodiscard]]
  size_t rank() const {
    return std::max_element(edges().begin(), edges().end(), [](const auto a, const auto b) {
      return a.second.size() < b.second.size();
    })->second.size();
  }

private:
  // Constructor that directly sets adjacency lists and next vertex ID (assuming that you just contracted an edge)
  HypergraphBase(std::unordered_map<int, std::vector<int>> &&vertices,
                 std::unordered_map<int, std::vector<int>> &&edges,
                 const HypergraphBase &old);

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
