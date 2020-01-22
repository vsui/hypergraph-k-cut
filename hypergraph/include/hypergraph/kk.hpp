#pragma once

#include <random>

#include "hypergraph/hypergraph.hpp"

namespace kk {

/* The approximate min-cut algorithm from KK'14.
 *
 * The input hypergraph should be r-uniform and epsilon should be greater than 1.
 */
template<typename HypergraphType>
HypergraphCut<HypergraphType> contract(const HypergraphType &hypergraph, double epsilon) {
  size_t r = hypergraph.rank();

  // Static random device for random sampling and generating random numbers
  static std::random_device rd;
  static std::mt19937 gen(rd());

  HypergraphType h(hypergraph);

  // TODO abstract sampling an edge with some weight function and use with fpz and cxy
  while (h.num_vertices() > epsilon * r) {
    if (h.num_edges() == 0) {
      break;
    }
    // Select a hyperedge with probability proportional to its weight
    std::vector<int> edge_ids;
    std::vector<Hypergraph::EdgeWeight> edge_weights;
    for (const auto &[edge_id, vertices] : h.edges()) {
      edge_ids.push_back(edge_id);
      edge_weights.push_back(edge_weight(h, edge_id));
    }
    std::discrete_distribution<size_t> distribution(std::begin(edge_weights), std::end(edge_weights));
    const auto sampled_edge_id = edge_ids.at(distribution(gen));

    h = h.template contract<true>(sampled_edge_id);
  }

  static std::uniform_real_distribution<> dis(0.0, 1.0);

  // Return random bipartition
  std::vector<int> vertices(std::begin(h.vertices()), std::end(h.vertices()));
  std::shuffle(std::begin(vertices), std::end(vertices), gen);

  std::uniform_int_distribution<size_t> split_dist(1, vertices.size() - 1);
  size_t split = split_dist(gen);

  std::array<std::list<int>, 2> partitions;
  for (auto it = std::begin(vertices); it != std::begin(vertices) + split; ++it) {
    partitions.at(0).insert(
        std::end(partitions.at(0)),
        std::begin(h.vertices_within(*it)),
        std::end(h.vertices_within(*it)));
  }
  for (auto it = std::begin(vertices) + split; it != std::end(vertices); ++it) {
    partitions.at(1).insert(
        std::end(partitions.at(1)),
        std::begin(h.vertices_within(*it)),
        std::end(h.vertices_within(*it)));
  }

  // Check cut value is as expected
  // TODO this was copied from cut_is_valid. Could be extracted somehow
  constexpr auto edge_entirely_inside_partition =
      [](const auto edge_begin, const auto edge_end, const auto partition_begin, const auto partition_end) {
        return std::all_of(edge_begin, edge_end, [partition_begin, partition_end](const auto v) {
          return std::find(partition_begin, partition_end, v) != partition_end;
        });
      };

  typename HypergraphType::EdgeWeight cut_value = 0;
  for (const auto &edge : hypergraph.edges()) {
    // Destructured bindings cannot be captured in lambdas
    const auto &edge_id = edge.first;
    const auto &vertices = edge.second;
    const bool none_of_edges_entirely_inside_partition =
        std::none_of(std::begin(partitions),
                     std::end(partitions),
                     [edge_entirely_inside_partition, vertices](const auto &partition) {
                       return edge_entirely_inside_partition(
                           std::begin(vertices),
                           std::end(vertices),
                           std::begin(partition),
                           std::end(partition)
                       );
                     });
    if (none_of_edges_entirely_inside_partition) {
      cut_value += edge_weight(hypergraph, edge_id);
    }
  }

  return {std::begin(partitions), std::end(partitions), cut_value};
}

}