#pragma once

#include <cassert>
#include <unordered_map>
#include <vector>
#include <functional>
#include <numeric>

#include <boost/range/adaptors.hpp>

#include "heap.hpp"

class KTrimmedCertificate;

template<typename EdgeWeightType>
struct HypergraphCut {
  static_assert(std::is_arithmetic_v<EdgeWeightType>);

  explicit HypergraphCut(EdgeWeightType value) : value(value) {}

  template<typename It>
  HypergraphCut(It begin, It end, EdgeWeightType value): partitions(), value(value) {
    for (auto it = begin; it != end; ++it) {
      partitions.emplace_back(std::begin(*it), std::end(*it));
    }
  }

  bool operator<(const HypergraphCut &other) const {
    return value < other.value;
  }

  std::vector<std::vector<int>> partitions;
  EdgeWeightType value;

  // Just returns an invalid cut with maximum value for use as a placeholder value
  static HypergraphCut max() {
    return HypergraphCut(std::numeric_limits<EdgeWeightType>::max());
  }
};

template<typename HypergraphType>
bool cut_is_valid(const HypergraphCut<typename HypergraphType::EdgeWeight> &cut,
                  const HypergraphType &hypergraph,
                  size_t k,
                  std::string &error) {
  // Check number of partitions
  if (cut.partitions.size() != k) {
    error = "Number of partitions does not match k";
    return false;
  }

  // Check all vertices in hypergraph are in one of the partitions
  size_t n_vertices = std::accumulate(std::begin(cut.partitions),
                                      std::end(cut.partitions),
                                      0,
                                      [](const auto accum, const auto partition) {
                                        return accum + partition.size();
                                      });
  if (n_vertices != hypergraph.num_vertices()) {
    error = "Number of vertices in all partitions is not equal to the number of vertices in the hypergraph";
    return false;
  }

  // Check vertices in partitions are the vertices in the hypergraph
  std::set<int> in_partitions;
  for (const auto &partition : cut.partitions) {
    for (const auto v : partition) {
      in_partitions.emplace(v);
    }
  }
  std::set<int> in_hypergraph(std::begin(hypergraph.vertices()), std::end(hypergraph.vertices()));
  if (in_partitions != in_hypergraph) {
    error = "Vertices in partitions do not match vertices in hypergraph";
    return false;
  }

  // Check that no partition is empty
  if (std::any_of(std::begin(cut.partitions), std::end(cut.partitions), [](const auto &c) { return c.empty(); })) {
    error = "One of the partitions was empty";
    return false;
  }

  // Check cut value is as expected
  constexpr auto edge_entirely_inside_partition =
      [](const auto edge_begin, const auto edge_end, const auto partition_begin, const auto partition_end) {
        return std::all_of(edge_begin, edge_end, [partition_begin, partition_end](const auto v) {
          return std::find(partition_begin, partition_end, v) != partition_end;
        });
      };

  typename HypergraphType::EdgeWeight expected_cut_value = 0;
  for (const auto &edge : hypergraph.edges()) {
    // Destructured bindings cannot be captured in lambdas
    const auto &edge_id = edge.first;
    const auto &vertices = edge.second;
    const bool none_of_edges_entirely_inside_partition =
        std::none_of(std::begin(cut.partitions),
                     std::end(cut.partitions),
                     [edge_entirely_inside_partition, vertices](const auto &partition) {
                       return edge_entirely_inside_partition(
                           std::begin(vertices),
                           std::end(vertices),
                           std::begin(partition),
                           std::end(partition)
                       );
                     });
    if (none_of_edges_entirely_inside_partition) {
      expected_cut_value += edge_weight(hypergraph, edge_id);
    }
  }
  if constexpr (std::is_floating_point_v<typename HypergraphType::EdgeWeight>) {
    if (std::abs(expected_cut_value - cut.value) > 0.1) {
      error = "Stored value of cut (" + std::to_string(cut.value) + ") does not match actual value of cut ("
          + std::to_string(expected_cut_value) + ")";
      return false;
    }
    return true;
  } else {
    if (expected_cut_value != cut.value) {
      error = "Stored value of cut (" + std::to_string(cut.value) + ") does not match actual value of cut ("
          + std::to_string(expected_cut_value) + ")";
    }
    return expected_cut_value == cut.value;
  }
}

template<typename EdgeWeight>
std::ostream &operator<<(std::ostream &os, const HypergraphCut<EdgeWeight> &cut) {
  os << "VALUE: " << cut.value << std::endl;
  size_t i = 1;
  for (const auto &partition: cut.partitions) {
    os << "PARTITION " << i << ":";
    for (const auto v : partition) {
      os << " " << v;
    }
    os << std::endl;
    ++i;
  }
  return os;
}

template<typename HypergraphType>
using MinimumCutFunction = std::function<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &)>;
template<typename HypergraphType>
using MinimumCutFunctionWithVertex = std::function<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &,
                                                                                                      int)>;
template<typename HypergraphType>
using MinimumKCutFunction = std::function<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &,
                                                                                             size_t k)>;

class Hypergraph {
public:
  using Heap = BucketHeap;
  using EdgeWeight = size_t;

  Hypergraph &operator=(const Hypergraph &other);

  Hypergraph();

  Hypergraph(const Hypergraph &other);

  Hypergraph(const std::vector<int> &vertices,
             const std::vector<std::vector<int>> &edges);

  /**
   * Determines whether two hypergraphs have the same vertices and hyperedges
   *
   * @param other
   * @return
   */
  bool operator==(const Hypergraph &other) const {
    return vertices_ == other.vertices_ && edges_ == other.edges_;
  }

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
  template<bool EdgeMayContainLoops = true> // TODO check rank and set as necessary
  [[nodiscard]]
  Hypergraph contract(const int edge_id) const {
    std::vector<int> old_edge = edges_.at(edge_id);

    if (old_edge.empty()) {
      // Pretty much nothing will change, just remove the old edge
      auto new_vertices = vertices_;
      auto new_edges = edges_;
      new_edges.erase(edge_id);
      return Hypergraph(std::move(new_vertices), std::move(new_edges), *this);
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

    Hypergraph new_hypergraph(std::move(new_vertices), std::move(new_edges), *this);
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
  [[nodiscard]] Hypergraph contract(InputIt begin, InputIt end) const {
    // TODO if we have a non-const contract then this copy is unnecessary. Right
    // now we copy twice (once to avoid modifying the input hypergraph and the
    // second time to contract)
    Hypergraph copy(*this);
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
  friend class KTrimmedCertificate;

  // Constructor that directly sets adjacency lists and next vertex ID (assuming that you just contracted an edge)
  Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
             std::unordered_map<int, std::vector<int>> &&edges,
             const Hypergraph &old);

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

/* A hypergraph with weighted edges. The time complexities of all operations are the same as with the unweighted
 * hypergraph.
 */
template<typename EdgeWeightType>
class WeightedHypergraph : public Hypergraph {
public:
  using Heap = FibonacciHeap<EdgeWeightType>;
  using EdgeWeight = EdgeWeightType;

  WeightedHypergraph() = default;

  template<typename OtherEdgeWeight>
  explicit WeightedHypergraph(const WeightedHypergraph<OtherEdgeWeight> &other): Hypergraph(other.hypergraph_) {
    for (const auto &[edge_id, incident_on] : other.edges()) {
      edges_to_weights_.insert({edge_id, other.edge_weight(edge_id)});
    }
  }

  explicit WeightedHypergraph(const Hypergraph &hypergraph) : Hypergraph(hypergraph) {
    for (const auto &[edge_id, incident_on] : hypergraph.edges()) {
      edges_to_weights_.insert({edge_id, 1});
    }
  }

  bool operator==(const WeightedHypergraph &other) const {
    return Hypergraph::operator==(other) && edges_to_weights_ == other.edges_to_weights_;
  }

  std::vector<std::vector<int>> edges_weights_removed(const std::vector<std::pair<std::vector<int>,
                                                                                  EdgeWeightType>> &edges) const {
    std::vector<std::vector<int>> edges_without_weights;
    std::transform(std::begin(edges), std::end(edges), std::back_inserter(edges_without_weights), [](const auto &pair) {
      return pair.first;
    });
    return edges_without_weights;
  }
  WeightedHypergraph(const std::vector<int> &vertices,
                     const std::vector<std::pair<std::vector<int>, EdgeWeightType>> edges) :
      Hypergraph(vertices, edges_weights_removed(edges)) {
    for (size_t i = 0; i < edges.size(); ++i) {
      const auto[it, inserted] = edges_to_weights_.insert({i, edges.at(i).second});
#ifndef NDEBUG
      assert(inserted);
#endif
    }
  }

  using vertex_range = Hypergraph::vertex_range;

  template<bool EdgeMayContainLoops = true>
  [[nodiscard]]
  WeightedHypergraph contract(int edge_id) const {
    Hypergraph contracted = Hypergraph::contract<EdgeMayContainLoops>(edge_id);
    // TODO edges to weights should still be valid
    return WeightedHypergraph(contracted, edges_to_weights_);
  }

  EdgeWeightType edge_weight(int edge_id) const {
    return edges_to_weights_.at(edge_id);
  }

  void resample_edge_weights(std::function<EdgeWeightType()> f) {
    for (auto &[k, v] : edges_to_weights_) {
      v = f();
    }
  }

  template<typename InputIt>
  int add_hyperedge(InputIt begin, InputIt end, EdgeWeightType weight) {
    auto id = Hypergraph::add_hyperedge(begin, end);
    const auto[it, inserted] = edges_to_weights_.insert({id, weight});
#ifndef NDEBUG
    assert(inserted);
#endif
    return id;
  }

  void remove_hyperedge(int edge_id) {
    // Edge weights from edges that were subsets of the removed edge are not removed
    Hypergraph::remove_hyperedge(edge_id);
    edges_to_weights_.erase(edge_id);
  }

  template<typename InputIt>
  WeightedHypergraph contract(InputIt begin, InputIt end) const {
    WeightedHypergraph copy(*this);
    auto new_e = copy.add_hyperedge(begin, end, {}); // Doesn't matter what weight you add
    return copy.contract(new_e);
  }

private:
  WeightedHypergraph(const Hypergraph &hypergraph, const std::unordered_map<int, EdgeWeightType> edge_weights) :
      Hypergraph(hypergraph), edges_to_weights_(edge_weights) {}

  std::unordered_map<int, EdgeWeightType> edges_to_weights_;
};

template<typename HypergraphType>
inline typename HypergraphType::EdgeWeight edge_weight(const HypergraphType &hypergraph, int edge_id) {
  if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
    return 1;
  } else {
    return hypergraph.edge_weight(edge_id);
  }
}

/* Return the cost of all edges of a hypergraph combined
 */
template<typename HypergraphType>
inline typename HypergraphType::EdgeWeight total_edge_weight(const HypergraphType &hypergraph) {
  // TODO could probably optimize for weighted hypergraphs by caching the weight, but then we would have to be a lot
  //      more careful about keeping edge_to_weights_ completely accurate
  if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
    return hypergraph.edges().size();
  } else {
    return std::accumulate(hypergraph.edges().begin(),
                           hypergraph.edges().end(),
                           typename HypergraphType::EdgeWeight(0),
                           [&hypergraph](
                               const typename HypergraphType::EdgeWeight accum,
                               const auto &a) {
                             return accum + hypergraph.edge_weight(a.first);
                           });
  }
}

/* For a hypergraph with vertices V, returns the value of the cut (V\{v}, {v}).
 *
 * Time complexity: O(m), where m is the number of edges
 */
template<typename HypergraphType>
HypergraphCut<typename HypergraphType::EdgeWeight> one_vertex_cut(const HypergraphType &hypergraph, const int v) {
  // Get cut-value
  typename HypergraphType::EdgeWeight cut_value = 0;
  if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
    cut_value = hypergraph.edges_incident_on(v).size();
  } else {
    for (const auto e : hypergraph.edges_incident_on(v)) {
      cut_value += edge_weight(hypergraph, e);
    }
  }

  // Get partitions
  // TODO can't use splice here because we need to keep the vertices valid
  std::list<int> partitions[2] = {{}, {}};
  partitions[0].insert(std::end(partitions[0]),
                       std::begin(hypergraph.vertices_within(v)), std::end(hypergraph.vertices_within(v)));
  for (const int u : hypergraph.vertices()) {
    if (u == v) {
      continue;
    }
    partitions[1].insert(std::end(partitions[1]),
                         std::begin(hypergraph.vertices_within(u)), std::end(hypergraph.vertices_within(u)));
  }

  return HypergraphCut<typename HypergraphType::EdgeWeight>(std::begin(partitions),
                                                            std::end(partitions),
                                                            cut_value);
}

/* Return a new hypergraph with vertices s and t merged.
 *
 * Time complexity: O(p), where p is the size of the hypergraph
 */
template<typename HypergraphType>
HypergraphType merge_vertices(const HypergraphType &hypergraph, const int s, const int t) {
  const auto vs = {s, t};
  return hypergraph.contract(std::begin(vs), std::end(vs));
}

std::istream &operator>>(std::istream &is, Hypergraph &hypergraph);

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph);

template<typename EdgeWeightType>
std::istream &operator>>(std::istream &is, WeightedHypergraph<EdgeWeightType> &hypergraph) {
  size_t num_edges, num_vertices;
  is >> num_edges >> num_vertices;

  std::vector<std::pair<std::vector<int>, EdgeWeightType>> edges;
  std::string line;
  std::getline(is, line); // Throw away first line

  int i = 0;
  while (i++ < num_edges && std::getline(is, line)) {
    EdgeWeightType edge_weight;
    std::vector<int> edge;
    std::stringstream sstr(line);

    sstr >> edge_weight;

    int v;
    while (sstr >> v) {
      edge.push_back(v);
    }
    edges.emplace_back(edge, edge_weight);
  }

  std::vector<int> vertices(num_vertices);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  hypergraph = WeightedHypergraph<EdgeWeightType>(vertices, edges);
  return is;
}

template<typename EdgeWeightType>
std::ostream &operator<<(std::ostream &os, const WeightedHypergraph<EdgeWeightType> &hypergraph) {
  os << hypergraph.num_edges() << " " << hypergraph.num_vertices() << " 1" << std::endl;
  for (const auto &[edge_id, vertices] : hypergraph.edges()) {
    os << hypergraph.edge_weight(edge_id);
    for (const auto v : vertices) {
      os << " " << v;
    }
    os << std::endl;
  }
  return os;
}

template<typename HypergraphType>
constexpr bool is_weighted = !std::is_same_v<HypergraphType, Hypergraph>;

// Return true if the header of the output stream appears to be an hmetis file
bool is_unweighted_hmetis_file(std::istream &is);
