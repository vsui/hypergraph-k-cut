#pragma once

#include <random>

#include "hypergraph.hpp"
#include "util.hpp"

namespace hypergraphlib {

struct kk : public ContractionAlgo<kk> {

  static constexpr bool pass_discovery_value = false;

  static constexpr char name[] = "KK";

  template<typename HypergraphType>
  struct Context : public util::BaseContext<HypergraphType> {

    size_t rank;

    Context(const HypergraphType &hypergraph,
            size_t k,
            const std::mt19937_64 &random_generator,
            typename HypergraphType::EdgeWeight discovery_value,
            std::optional<size_t> max_num_runs)
        : util::BaseContext<HypergraphType>(hypergraph, k, random_generator, discovery_value, max_num_runs),
          rank(hypergraph.rank()) {}
  };

/**
 * The min-cut algorithm from [KK'14].
 *
 * Runtime is dependent on the rank of the hypergraph.
 *
 * @tparam HypergraphType
 * @param hypergraph
 * @param k
 * @param random_generator
 * @return
 */
  template<typename HypergraphType, bool ReturnPartitions, uint8_t Verbosity>
  static HypergraphCut<typename HypergraphType::EdgeWeight> contract(Context<HypergraphType> &ctx) {
    HypergraphType h(ctx.hypergraph);

    // This can be tweaked to make this algorithm approximate. We are interested in exact solutions however.
    double alpha = 1.5;

    // TODO abstract sampling an edge with some weight function and use with fpz and cxy
    while (h.num_vertices() > alpha * ctx.k * ctx.rank) { // TODO what should this be when k > 2?
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
      const auto sampled_edge_id = edge_ids.at(distribution(ctx.random_generator));

      h = h.template contract<true, ReturnPartitions>(sampled_edge_id);
      ++ctx.stats.num_contractions;
    }

    const auto contracted_partitions = random_k_partition(h, ctx.k, ctx.random_generator);

    const auto cut_value = cut_value_of_partition(h, contracted_partitions.cbegin(), contracted_partitions.cend());

    if constexpr (ReturnPartitions) {
      // Collect vertices within
      std::vector<std::list<int>> partitions(ctx.k);
      for (size_t i = 0; i < ctx.k; ++i) {
        for (const auto v : contracted_partitions[i]) {
          partitions[i].insert(partitions[i].end(), std::begin(h.vertices_within(v)), std::end(h.vertices_within(v)));
        }
      }
      return {std::begin(partitions), std::end(partitions), cut_value};
    } else {
      return HypergraphCut<typename HypergraphType::EdgeWeight>(cut_value);
    }
  }

  template<typename HypergraphType>
  static size_t default_num_runs(const HypergraphType &hypergraph, size_t k) {
    const auto r = hypergraph.rank();
    const auto n = hypergraph.num_vertices();
    return std::pow(2, r) * std::pow(n, k) * std::log(n);
  }

  /// Returns a random k-way partition of the contracted vertices of the hypergraph
  template<typename HypergraphType>
  static std::vector<std::list<int>> random_k_partition(const HypergraphType &hypergraph,
                                                        size_t k,
                                                        std::mt19937_64 &random_generator) {
    // Shuffle the vertices and choose k-1 indices to split them on
    std::vector<int> vertices(std::begin(hypergraph.vertices()), std::end(hypergraph.vertices()));
    std::shuffle(std::begin(vertices), std::end(vertices), random_generator);

    std::vector<int> indices(vertices.size() - 1);
    std::iota(indices.begin(), indices.end(), 1);

    std::vector<int> sampled(k + 1);
    sampled[0] = 0;
    sampled[k] = vertices.size();

    std::sample(indices.begin(), indices.end(), sampled.begin() + 1, k - 1, random_generator);

    std::vector<std::list<int>> contracted_partitions;
    for (auto it = sampled.begin() + 1; it != sampled.end(); ++it) {
      contracted_partitions.emplace_back(vertices.begin() + *(it - 1), vertices.begin() + *it);
    }

    return contracted_partitions;
  }

  template<typename HypergraphType, typename InputIt>
  static typename HypergraphType::EdgeWeight cut_value_of_partition(const HypergraphType &hypergraph,
                                                                    InputIt begin,
                                                                    InputIt end) {
    // TODO Compute cut value of partition
    // TODO this was copied from cut_is_valid. Could be extracted somehow
    // Given a range of vertices in the edge and a range of vertices in the partition, returns whether
    // any of the vertices in the edge are not in the partition
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
          std::none_of(begin,
                       end,
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

    return cut_value;
  }
};

} // hypergraphlib
