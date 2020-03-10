#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  // Can't use initializer list, std::apply not working
  std::vector<size_t> multipliers = {10};
  std::vector<size_t> ns = {100, 200, 300, 400, 500};
  std::vector<size_t> ranks = {3, 4, 5};

  using HypergraphGeneratorPtr = std::unique_ptr<HypergraphGenerator>;

  using Experiment = std::tuple<std::string, std::vector<HypergraphGeneratorPtr>>;

  std::vector<Experiment> planted_experiments;
  std::vector<Experiment> uniformplanted_experiments;

  // Make planted generators
  for (size_t k = 2; k < 5; ++k) {
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

  // Make uniform planted generators
  for (size_t k = 2; k < 5; ++k) {
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

  for (auto &experiment : planted_experiments) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("my.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    auto &[name, generators] = experiment;
    KDiscoveryRunner runner(name, std::move(generators), std::move(store));
    runner.run();
  }

  for (auto &experiment : uniformplanted_experiments) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("my.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    auto &[name, generators] = experiment;
    KDiscoveryRunner runner(name, std::move(generators), std::move(store), false);
    runner.run();
  }

  return 0;
}