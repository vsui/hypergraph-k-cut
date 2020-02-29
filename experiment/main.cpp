#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  std::vector<MXNHypergraph::ConstructorArgs> mxn_args = {
      {100, 1000, 0.1, 777},
      {100, 2000, 0.1, 777},
      {100, 3000, 0.1, 777},
      {100, 4000, 0.1, 777},
      {1000, 10000, 0.05, 777},
      {1000, 10000, 0.1, 777},
      {1000, 10000, 0.15, 777},
      {1000, 10000, 0.20, 777},
  };

  // Can't use initializer list, std::apply not working
  std::vector<std::unique_ptr<HypergraphGenerator>> planted;

  for (size_t n : {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 10000}) {
    size_t k = 3;
    // Set m1 and m2 so that the number of edges inside the cluster dominates crossing.
    size_t m2 = n / 10;
    size_t m1 = m2 * 100;
    // Set p1 and p2 so that the size of all edges is roughly the same.
    double p2 = 0.1;
    double p1 = p2 * k;
    planted.emplace_back(std::make_unique<PlantedHypergraph>(n, m1, p1, m2, p2, k, 777));
  }

  auto store = std::make_unique<SqliteStore>();
  if (!store->open("my.db")) {
    std::cout << "Failed to open store" << std::endl;
    return 1;
  }

  // MinimumCutFinder finder(std::move(source), std::move(store));

  KDiscoveryRunner runner(std::move(planted), std::move(store));

  runner.run();

  return 0;
}