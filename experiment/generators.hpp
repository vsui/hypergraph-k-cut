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
  using ConstructorArgs = std::tuple<size_t, size_t, size_t, uint64_t>;

  size_t num_vertices;
  size_t num_hyperedges;
  size_t hyperedge_radius;
  uint64_t seed;

  RandomRingHypergraph(size_t n, size_t m, size_t r, uint64_t seed);

  std::string name();

  Hypergraph generate();
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP
