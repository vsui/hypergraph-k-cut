#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  // Can't use initializer list, std::apply not working
  std::vector<size_t> multipliers = {10};
  std::vector<size_t> ns = {100, 125, 150, 175, 200, 225, 250, 275, 300, 325, 350, 375, 400, 425, 450, 475, 500};
  std::vector<size_t> ranks = {3};

  using HypergraphGeneratorPtr = std::unique_ptr<HypergraphGenerator>;

  using Experiment = std::tuple<std::string, std::vector<HypergraphGeneratorPtr>>;

  /*
  std::vector<Experiment> planted_experiments;

  // Make planted generators
  for (size_t k = 2; k < 3; ++k) {
    for (size_t multiplier : multipliers) {
      std::string name = "planted_"s + std::to_string(k) + "_"s + std::to_string(multiplier);
      std::vector<HypergraphGeneratorPtr> generators;
      for (size_t n : ns) {
        size_t m2 = n / 10;
        size_t m1 = m2 * multiplier;
        double p2 = 0.1;
        double p1 = p2 * k;
        generators.emplace_back(std::make_unique<PlantedHypergraph>(n, m1, p1, m2, p2, k, 777));
      }
      planted_experiments.emplace_back(name, std::move(generators));
    }
  }


  for (auto &experiment : planted_experiments) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("mar11.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    auto &[name, generators] = experiment;
    KDiscoveryRunner runner(name, std::move(generators), std::move(store));
    runner.run();
  }

   */

  // Create ring experiments
  std::vector<Experiment> ring_experiments;
  std::vector<Experiment> uniformring_experiments;

  // Make ring generators
  for (double hyperedge_radius : {10, 15, 20}) {
    std::vector<HypergraphGeneratorPtr> generators;
    for (size_t num_vertices : {100, 150, 200, 250, 300, 350, 400, 450, 500}) {
      size_t num_edges = num_vertices * 10;
      generators.emplace_back(std::make_unique<RandomRingConstantEdgeHypergraph>(num_vertices,
                                                                                 num_edges,
                                                                                 hyperedge_radius,
                                                                                 777));
    }
    ring_experiments.emplace_back("ring_"s + std::to_string(hyperedge_radius), std::move(generators));
  }

  /*
  for (auto &experiment : ring_experiments) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("mar11-2.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    auto &[name, generators] = experiment;
    RandomRingRunner runner(name, std::move(generators), std::move(store), false);
    runner.run();
  }
   */

  std::vector<Experiment> uniformplanted_experiments;
  // Make uniform planted generators
  for (size_t k = 2; k < 3; ++k) {
    for (size_t multiplier : multipliers) {
      for (size_t rank : ranks) {
        std::string name =
            "uniformplanted_"s + std::to_string(k) + "_"s + std::to_string(multiplier) + "_"s + std::to_string(rank);
        std::vector<HypergraphGeneratorPtr> generators;
        for (size_t n : ns) {
          size_t m2 = n / 10;
          size_t m1 = m2 * multiplier;
          generators.emplace_back(std::make_unique<UniformPlantedHypergraph>(n, k, m1, m2, rank, 777));
        }
        uniformplanted_experiments.emplace_back(name, std::move(generators));
      }
    }
  }

  for (auto &experiment : uniformplanted_experiments) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("mar11-3.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    auto &[name, generators] = experiment;
    KDiscoveryRunner runner(name, std::move(generators), std::move(store), false);
    runner.run();
  }

  return 0;
}