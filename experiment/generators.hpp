//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP

#include <cstddef>
#include <tuple>
#include <random>

#include <hypergraph/hypergraph.hpp>

struct RandomRingHypergraph {

  RandomRingHypergraph(size_t num_vertices,
                       size_t num_hyperedges,
                       double hyperedge_mean,
                       double hyperedge_variance,
                       uint64_t seed);

  size_t num_vertices;
  size_t num_hyperedges;
  double hyperedge_mean; // Mean angle of the sector each hyperedge is sampled from
  double hyperedge_variance; // Variance of size the sector each hyperedge is sampled from
  uint64_t seed;

  virtual std::string name() = 0;

  Hypergraph generate();

private:
  void sample_hyperedge(const std::vector<int> &vertices, const std::vector<double> &positions, std::vector<int> &edge);

  std::mt19937_64 gen_;
  std::uniform_real_distribution<> dis_;

};

struct RandomRingConstantEdgeHypergraph : public RandomRingHypergraph {
  using ConstructorArgs = std::tuple<size_t, size_t, double, uint64_t>;

  RandomRingConstantEdgeHypergraph(size_t n, size_t m, double r, uint64_t seed);

  std::string name() override;
};

struct RandomRingVariableEdgeHypergraph : public RandomRingHypergraph {
  using ConstructorArgs = std::tuple<size_t, size_t, double, double, uint64_t>;

  using RandomRingHypergraph::RandomRingHypergraph;

  std::string name() override;
};

/**
 * Samples m random hypperedges. For each random hyperedge, sample every vertex with probability p
 */
struct MXNHypergraph {
  using ConstructorArgs = std::tuple<size_t, size_t, double, double>;

  MXNHypergraph(size_t n, size_t m, double p, uint64_t seed);

  size_t n;
  size_t m;
  double p;
  uint64_t seed;

  Hypergraph generate();

  std::string name();
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP
