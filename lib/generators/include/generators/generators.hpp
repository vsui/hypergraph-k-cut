//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP

#include <cstddef>
#include <tuple>
#include <random>
#include <optional>

#include <hypergraph/hypergraph.hpp>
#include <hypergraph/cut.hpp>

class sqlite3;

struct HypergraphGenerator {
  /**
   * Returns a hypergraph and possibly a cut. An instance should always return the same hypergraph.
   * @return
   */
  [[nodiscard]]
  virtual std::tuple<Hypergraph, std::optional<HypergraphCut<size_t>>> generate() const = 0;

  /**
   * Returns a unique identifier for this hypergraph
   * @return
   */
  [[nodiscard]]
  virtual std::string name() const = 0;

  /**
   * Attempts to write to the database. Returns true on success, false on failure.
   * @return
   */
  [[nodiscard]]
  virtual bool write_to_table(sqlite3 *db) const = 0;

  virtual ~HypergraphGenerator() = 0;
};

struct RandomRingHypergraph : public HypergraphGenerator {

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

  bool write_to_table(sqlite3 *db) const override;

  std::tuple<Hypergraph, std::optional<HypergraphCut<size_t>>> generate() const override;

private:
  void sample_hyperedge(const std::vector<int> &vertices,
                        const std::vector<double> &positions,
                        std::vector<int> &edge,
                        std::mt19937_64 &gen) const;

  mutable std::uniform_real_distribution<> dis_;

};

struct RandomRingConstantEdgeHypergraph : public RandomRingHypergraph {
  using ConstructorArgs = std::tuple<size_t, size_t, double, uint64_t>;

  RandomRingConstantEdgeHypergraph(size_t n, size_t m, double r, uint64_t seed);

  std::string name() const override;
};

struct RandomRingVariableEdgeHypergraph : public RandomRingHypergraph {
  using ConstructorArgs = std::tuple<size_t, size_t, double, double, uint64_t>;

  using RandomRingHypergraph::RandomRingHypergraph;

  std::string name() const override;
};

/**
 * Samples m random hypperedges. For each random hyperedge, sample every vertex with probability p
 */
struct MXNHypergraph {
  using ConstructorArgs = std::tuple<size_t, size_t, double, uint64_t>;

  MXNHypergraph(size_t n, size_t m, double p, uint64_t seed);

  size_t n;
  size_t m;
  double p;
  uint64_t seed;

  Hypergraph generate();

  std::string name();
};

/**
 * Divide the hypergraph into k equally-sized clusters.
 *
 * Sample m1 hyperedges from each cluster, sample m2 hyperedges from the entire hypergraph.
 *
 * Hyperedges are sampled by adding each vertex with probability p1 or p2.
 */
struct PlantedHypergraph : public HypergraphGenerator {
  using ConstructorArgs = std::tuple<size_t, size_t, double, size_t, double, size_t, uint64_t>;

  PlantedHypergraph(size_t n, size_t m1, double p1, size_t m2, double p2, size_t k, uint64_t seed);

  size_t n;
  size_t m1;
  double p1;
  size_t m2;
  double p2;
  size_t k;
  uint64_t seed;

  static std::string make_table_sql_command();

  [[nodiscard]]
  std::tuple<Hypergraph, std::optional<HypergraphCut<size_t>>> generate() const override;

  [[nodiscard]]
  std::string name() const override;

  [[nodiscard]]
  bool write_to_table(sqlite3 *db) const override;
};

/**
 * Same as a planted hypergraph, but every hyperedge has the same rank (r).
 */
struct UniformPlantedHypergraph : public HypergraphGenerator {
  using ConstructorArgs = std::tuple<size_t, size_t, size_t, size_t, size_t, uint64_t>;

  UniformPlantedHypergraph(size_t n, size_t k, size_t r, size_t m1, size_t m2, uint64_t seed);

  size_t n;
  size_t k;
  size_t r;
  size_t m1;
  size_t m2;
  uint64_t seed;

  [[nodiscard]]
  std::tuple<Hypergraph, std::optional<HypergraphCut<size_t>>> generate() const override;

  [[nodiscard]]
  std::string name() const override;

  [[nodiscard]]
  bool write_to_table(sqlite3 *db) const override;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_GENERATORS_HPP
