#pragma once

#include <cassert>
#include <unordered_map>
#include <vector>
#include <functional>
#include <numeric>

#include <boost/range/adaptors.hpp>

#include "heap.hpp"
#include "base.hpp"

// To restrict dynamic polymorphism with Hypergraphs and WeightedHypergraphs
class Hypergraph : public HypergraphBase<Hypergraph> {
  using HypergraphBase = HypergraphBase<Hypergraph>;
  friend HypergraphBase;

  Hypergraph(const HypergraphBase &h) : HypergraphBase(h) {}

  // Needs access to private constructor inherited from HypergraphBase
  friend class KTrimmedCertificate;

public:
  static constexpr bool weighted = false;

  using HypergraphBase::HypergraphBase;
};

/* A hypergraph with weighted edges. The time complexities of all operations are the same as with the unweighted
 * hypergraph.
 */
template<typename EdgeWeightType>
class WeightedHypergraph : public HypergraphBase<WeightedHypergraph<EdgeWeightType>> {
  using HypergraphBase = HypergraphBase<WeightedHypergraph<EdgeWeightType>>;
  friend HypergraphBase;

public:
  static constexpr bool weighted = true;

  using Heap = FibonacciHeap<EdgeWeightType>;
  using EdgeWeight = EdgeWeightType;

  WeightedHypergraph() = default;

  template<typename OtherEdgeWeight>
  explicit WeightedHypergraph(const WeightedHypergraph<OtherEdgeWeight> &other): HypergraphBase(other.hypergraph_) {
    for (const auto &[edge_id, incident_on] : other.edges()) {
      edges_to_weights_.insert({edge_id, other.edge_weight(edge_id)});
    }
  }

  explicit WeightedHypergraph(const Hypergraph &hypergraph) :
      WeightedHypergraph(hypergraph.vertices(),
                         edges_weights_added(std::begin(hypergraph.edges()), std::end(hypergraph.edges()))) {}

  WeightedHypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
                     std::unordered_map<int, std::vector<int>> &&edges,
                     const WeightedHypergraph &old) :
      HypergraphBase(std::move(vertices), std::move(edges), static_cast<const HypergraphBase &>(old)),
      edges_to_weights_(old.edges_to_weights_) {}

  bool operator==(const WeightedHypergraph &other) const {
    return HypergraphBase::operator==(other) && edges_to_weights_ == other.edges_to_weights_;
  }

  using Edge = std::vector<int>;
  using WeightedEdge = std::pair<Edge, EdgeWeightType>;

  std::vector<Edge> edges_weights_removed(const std::vector<WeightedEdge> &edges) const {
    std::vector<Edge> edges_without_weights;
    std::transform(std::begin(edges), std::end(edges), std::back_inserter(edges_without_weights), [](const auto &pair) {
      return pair.first;
    });
    return edges_without_weights;
  }

  template<typename InputIt>
  std::vector<WeightedEdge> edges_weights_added(InputIt begin, InputIt end) const {
    std::vector<WeightedEdge> edges_with_weights;
    std::transform(begin, end, std::back_inserter(edges_with_weights), [](const auto &edge) {
      return WeightedEdge{edge, 1};
    });
    return edges_with_weights;
  }

  WeightedHypergraph(const std::vector<int> &vertices,
                     const std::vector<std::pair<std::vector<int>, EdgeWeightType>> edges) :
      HypergraphBase(vertices, edges_weights_removed(edges)) {
    for (size_t i = 0; i < edges.size(); ++i) {
      const auto[it, inserted] = edges_to_weights_.insert({i, edges.at(i).second});
#ifndef NDEBUG
      assert(inserted);
#endif
    }
  }

  using vertex_range = typename HypergraphBase::vertex_range;

  template<bool EdgeMayContainLoops = true>
  [[nodiscard]]
  WeightedHypergraph contract(int edge_id) const {
    HypergraphBase contracted = HypergraphBase::template contract<EdgeMayContainLoops>(edge_id);
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
    auto id = HypergraphBase::add_hyperedge(begin, end);
    const auto[it, inserted] = edges_to_weights_.insert({id, weight});
#ifndef NDEBUG
    assert(inserted);
#endif
    return id;
  }

  void remove_hyperedge(int edge_id) {
    // Edge weights from edges that were subsets of the removed edge are not removed
    HypergraphBase::remove_hyperedge(edge_id);
    edges_to_weights_.erase(edge_id);
  }

  template<typename InputIt>
  WeightedHypergraph contract(InputIt begin, InputIt end) const {
    WeightedHypergraph copy(*this);
    auto new_e = copy.add_hyperedge(begin, end, {}); // Doesn't matter what weight you add
    return copy.contract(new_e);
  }

private:
  WeightedHypergraph(const HypergraphBase &hypergraph, const std::unordered_map<int, EdgeWeightType> edge_weights) :
      HypergraphBase(hypergraph), edges_to_weights_(edge_weights) {}

  std::unordered_map<int, EdgeWeightType> edges_to_weights_;
};

template<typename HypergraphType>
constexpr bool is_weighted = HypergraphType::weighted;

template<typename HypergraphType>
constexpr bool is_unweighted = !HypergraphType::weighted;

template<typename HypergraphType>
inline typename HypergraphType::EdgeWeight edge_weight(const HypergraphType &hypergraph, int edge_id) {
  if constexpr (is_unweighted<HypergraphType>) {
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

// Return true if the header of the output stream appears to be an hmetis file
bool is_unweighted_hmetis_file(std::istream &is);
