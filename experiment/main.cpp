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

  std::vector<PlantedHypergraph::ConstructorArgs> planted_args = {
      {100, 100, 0.5, 100, 0.1, 3, 777},
      {100, 300, 0.5, 100, 0.1, 3, 777},
      {100, 500, 0.5, 100, 0.1, 3, 777}
  };

  using Ptr = std::unique_ptr<HypergraphSource>;

  auto source = std::make_unique<PlantedGeneratorSource<PlantedHypergraph>>(planted_args);

  auto store = std::make_unique<SqliteStore>();
  store->open("my.db");

  // MinimumCutFinder finder(std::move(source), std::move(store));

  KDiscoveryRunner runner(std::move(source), std::move(store));

  runner.run();

  return 0;
}