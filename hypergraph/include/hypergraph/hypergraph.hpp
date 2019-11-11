#pragma once

#include <cassert>
#include <unordered_map>
#include <vector>
#include <numeric>

#include <boost/range/adaptors.hpp>

#include "heap.hpp"

class KTrimmedCertificate;

template<typename HypergraphType>
struct HypergraphCut {
  explicit HypergraphCut(typename HypergraphType::EdgeWeight value) : value(value) {}

  template<typename It>
  HypergraphCut(It begin, It end, typename HypergraphType::EdgeWeight value): partitions(), value(value) {
    for (auto it = begin; it != end; ++it) {
      partitions.emplace_back(std::begin(*it), std::end(*it));
    }
  }

  bool operator<(const HypergraphCut &other) const {
    return value < other.value;
  }

  std::vector<std::vector<int>> partitions;
  typename HypergraphType::EdgeWeight value;

  // Just returns an invalid cut with maximum value for use as a placeholder value
  static HypergraphCut max() {
    return HypergraphCut(std::numeric_limits<typename HypergraphType::EdgeWeight>::max());
  }
};

template<typename HypergraphType>
bool cut_is_valid(const HypergraphCut<HypergraphType> &cut,
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
    }
  } else {
    if (expected_cut_value != cut.value) {
      error = "Stored value of cut (" + std::to_string(cut.value) + ") does not match actual value of cut ("
          + std::to_string(expected_cut_value) + ")";
    }
  }

  return expected_cut_value == cut.value;
}

template<typename HypergraphType>
std::ostream &operator<<(std::ostream &os, const HypergraphCut<HypergraphType> &cut) {
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

class Hypergraph {
public:
  using Heap = BucketHeap;
  using EdgeWeight = size_t;

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

  /* When an edge is contracted into a single vertex the original vertices in the edge can be stored and referred to
   * later using this method. This is useful for calculating the actual partitions that make up the cuts.
   */
  const std::list<int> &vertices_within(const int v) const;

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
class WeightedHypergraph {
  // The reason this is implemented with composition instead of inheritance is that many algorithms need to be aware of
  // exhibit significantly different behavior in the weighted vs. unweighted case.
public:
  using Heap = FibonacciHeap<EdgeWeightType>;
  using EdgeWeight = EdgeWeightType;

  WeightedHypergraph() = default;

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

  vertex_range vertices() const { return hypergraph_.vertices(); }
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
    const auto[it, inserted] = edges_to_weights_.insert({id, weight});
#ifndef NDEBUG
    assert(inserted);
#endif
    return id;
  }

  void remove_hyperedge(int edge_id) {
    // Edge weights from edges that were subsets of the removed edge are not removed
    hypergraph_.remove_hyperedge(edge_id);
    edges_to_weights_.erase(edge_id);
  }

  template<typename InputIt>
  WeightedHypergraph contract(InputIt begin, InputIt end) const {
    WeightedHypergraph copy(*this);
    auto new_e = copy.add_hyperedge(begin, end, {}); // Doesn't matter what weight you add
    return copy.contract(new_e);
  }

  const std::list<int> &vertices_within(const int v) const {
    return hypergraph_.vertices_within(v);
  }

private:
  WeightedHypergraph(const Hypergraph &hypergraph, const std::unordered_map<int, EdgeWeightType> edge_weights) :
      hypergraph_(hypergraph), edges_to_weights_(edge_weights) {}

  Hypergraph hypergraph_;

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
HypergraphCut<HypergraphType> one_vertex_cut(const HypergraphType &hypergraph, const int v) {
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

  return HypergraphCut<HypergraphType>(std::begin(partitions),
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

  while (std::getline(is, line)) {
    EdgeWeightType edge_weight;
    std::vector<int> edge;
    std::stringstream sstr(line);

    sstr >> edge_weight;

    int i;
    while (sstr >> i) {
      edge.push_back(i);
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

// Return true if the header of the output stream appears to be an hmetis file
bool is_unweighted_hmetis_file(std::istream &is);
