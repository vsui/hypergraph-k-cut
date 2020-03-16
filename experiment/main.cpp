#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

using HyGenPtr = std::unique_ptr<HypergraphGenerator>;
using HyGenPtrs = std::vector<HyGenPtr>;
using Experiment = std::tuple<std::string, HyGenPtrs, bool /* compare_kk */, bool /* planted */>;

namespace {

/**
 * Generate an experiment for planted hypergraphs where the size of edges in the crossing and non-crossing hyperedges
 * are equal in expectation.
 *
 * @param name name of experiment
 * @param num_vertices
 * @param k
 * @param m2_mult set m2 to `n / m2_mult`
 * @param m1_mult set m1 to `m2 * m1_mult`
 */
Experiment planted_experiment(const std::string &name,
                              const std::vector<size_t> &num_vertices,
                              size_t k,
                              size_t m2_mult,
                              size_t m1_mult) {
  Experiment planted_experiment;

  std::get<0>(planted_experiment) = name;
  std::get<2>(planted_experiment) = false;
  std::get<3>(planted_experiment) = true;

  for (size_t n : num_vertices) {
    size_t m2 = n / m2_mult;
    size_t m1 = m2 * m1_mult;
    double p2 = 0.1;
    double p1 = p2 * k;
    std::get<1>(planted_experiment).emplace_back(new PlantedHypergraph(n, m1, p1, m2, p2, k, 777));
  }
  return planted_experiment;
}

/**
 * Generate an experiment for uniform planted hypergraphs.
 * @param name
 * @param num_vertices
 * @param k
 * @param rank
 * @param m2_mult set m2 to `n / m2_mult`
 * @param m1_mult set m1 to `m2 * m1_mult`
 * @return
 */
Experiment planted_uniform_experiment(const std::string &name,
                                      const std::vector<size_t> &num_vertices,
                                      size_t k,
                                      size_t rank,
                                      size_t m2_mult,
                                      size_t m1_mult) {
  Experiment experiment;

  std::get<0>(experiment) = name;
  std::get<2>(experiment) = true;
  std::get<3>(experiment) = true;

  for (size_t n : num_vertices) {
    size_t m2 = n / m2_mult;
    size_t m1 = m2 * m1_mult;
    std::get<1>(experiment).emplace_back(new UniformPlantedHypergraph(n, k, rank, m1, m2, 777));
  }
  return experiment;
}

Experiment ring_experiment(const std::string &name,
                           const std::vector<size_t> &num_vertices,
                           size_t edge_mult,
                           size_t radius) {
  Experiment experiment;

  std::get<0>(experiment) = name;
  std::get<2>(experiment) = false;
  std::get<3>(experiment) = false;

  for (size_t n : num_vertices) {
    size_t num_edges = n * edge_mult;
    std::get<1>(experiment).emplace_back(new RandomRingConstantEdgeHypergraph(n, num_edges, radius, 777));
  }
  return experiment;
}

}

int main() {
  using namespace std::string_literals;

  // Can't use initializer list, std::apply not working
  // std::vector<size_t> ns = {100, 125, 150, 175, 200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475, 500};
  std::vector<size_t> ns = {110, 120, 130, 140, 150};

  std::vector<Experiment> experiments;
  /*
  // Make planted experiments
  for (size_t k = 2; k < 4; ++k) {
    std::string name = "planted_"s + std::to_string(k) + "_"s + std::to_string(10);
    Experiment experiment = planted_experiment(name, ns, k, 10, 10);
    experiments.emplace_back(std::move(experiment));
  }

  // Make uniform planted experiments
  for (size_t k = 2; k < 4; ++k) {
    size_t rank = 3;
    std::string name =
        "uniformplanted_"s + std::to_string(k) + "_"s + std::to_string(10) + "_"s + std::to_string(rank);
    Experiment experiment = planted_uniform_experiment(name, ns, k, rank, 10, 10);
    experiments.emplace_back(std::move(experiment));
  }
   */

  // Make ring experiments
  {
    size_t hyperedge_radius = 30;
    std::string name = "ring_"s + std::to_string(hyperedge_radius);
    Experiment experiment = ring_experiment(name, ns, 10, 30);
    experiments.emplace_back(std::move(experiment));
  }

  for (auto &experiment : experiments) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("3_16_20.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    auto &[name, generators, compare_kk, planted] = experiment;
    KDiscoveryRunner runner(name, std::move(generators), std::move(store), compare_kk, planted, 1);
    runner.run();
  }

  return 0;
}