#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  // Can't use initializer list, std::apply not working
  std::vector<size_t> multipliers = {1, 5, 10, 15, 20, 25};
  std::vector<std::vector<std::unique_ptr<HypergraphGenerator>>> planted_generators;

  for (size_t multiplier : multipliers) {
    planted_generators.emplace_back();
    std::vector<std::unique_ptr<HypergraphGenerator>> &planted = planted_generators.back();
    for (size_t n : {100, 150, 200, 250, 300, 350, 400, 450, 500}) {
      size_t k = 3;
      // Set m1 and m2 so that the number of edges inside the cluster dominates crossing.
      size_t m2 = n / 10;
      size_t m1 = m2 * multiplier;
      // Set p1 and p2 so that the size of all edges is roughly the same.
      double p2 = 0.1;
      double p1 = p2 * k;
      planted.emplace_back(std::make_unique<PlantedHypergraph>(n, m1, p1, m2, p2, k, 777));
    }
  }



  // MinimumCutFinder finder(std::move(source), std::move(store));

  for (int i = 0; i < multipliers.size(); ++i) {
    auto store = std::make_unique<SqliteStore>();
    if (!store->open("my.db")) {
      std::cout << "Failed to open store" << std::endl;
      return 1;
    }
    KDiscoveryRunner
        runner("discovery"s + std::to_string(multipliers[i]), std::move(planted_generators[i]), std::move(store));
    runner.run();
  }

  return 0;
}