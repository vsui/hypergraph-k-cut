#pragma once

#include "hypergraph.hpp"
#include "cut.hpp"

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

template<typename HypergraphType, bool ReturnPartitions>
struct HypergraphCutRet;

template<typename HypergraphType>
struct HypergraphCutRet<HypergraphType, true> {
  using T = HypergraphCut<typename HypergraphType::EdgeWeight>;

  static T max() {
    return T::max();
  }
};

template<typename HypergraphType>
struct HypergraphCutRet<HypergraphType, false> {
  using T = typename HypergraphType::EdgeWeight; // Just return cut value

  static T max() {
    return std::numeric_limits<T>::max();
  }
};

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

/* For a hypergraph with vertices V, returns the value of the cut (V\{v}, {v}).
 *
 * Time complexity: O(m), where m is the number of edges
 */
template<bool ReturnPartitions, typename HypergraphType>
auto one_vertex_cut(const HypergraphType &hypergraph, const int v) -> typename HypergraphCutRet<HypergraphType,
                                                                                                ReturnPartitions>::T {
  // Get cut-value
  typename HypergraphType::EdgeWeight cut_value = 0;
  if constexpr (is_unweighted<HypergraphType>) {
    cut_value = hypergraph.edges_incident_on(v).size();
  } else {
    for (const auto e : hypergraph.edges_incident_on(v)) {
      cut_value += edge_weight(hypergraph, e);
    }
  }

  if constexpr (ReturnPartitions) {
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
  } else {
    return cut_value;
  }
}

template<typename HypergraphType, bool ReturnsPartitions>
using Cut = typename std::conditional<ReturnsPartitions,
                                      HypergraphCut<typename HypergraphType::EdgeWeight>,
                                      typename HypergraphType::EdgeWeight>::type;

template<typename HypergraphType, bool ReturnsPartitions = true>
using MinimumCutFunction = std::function<Cut<HypergraphType, ReturnsPartitions>(HypergraphType &)>;

template<typename HypergraphType>
using MinimumKCutFunction = std::function<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &,
                                                                                             size_t k)>;

template<typename HypergraphType>
inline typename HypergraphType::EdgeWeight cut_value(const HypergraphCut<typename HypergraphType::EdgeWeight> &cut) {
  return cut.value;
}

template<typename HypergraphType>
inline typename HypergraphType::EdgeWeight cut_value(typename HypergraphType::EdgeWeight cut) {
  return cut;
}
