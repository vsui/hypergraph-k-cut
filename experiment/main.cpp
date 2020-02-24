#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  std::vector<RandomRingHypergraph::ConstructorArgs> args = {
      {10, 10, 10, 777},
      {100, 100, 10, 777},
      {1000, 1000, 10, 777},
      {1000, 1000, 20, 777},
      {1000, 1000, 30, 777},
      {1000, 1000, 40, 777},
      {1000, 1000, 50, 777},
      {1000, 1000, 60, 777},
      {1000, 1000, 70, 777},
      {1000, 5000, 10, 777},
      {1000, 5000, 20, 777},
      {1000, 5000, 30, 777},
      {1000, 5000, 40, 777},
      {1000, 5000, 50, 777},
      {1000, 5000, 60, 777},
      {1000, 5000, 70, 777},
      {1000, 10000, 20, 777},
      {1000, 10000, 30, 777},
      {1000, 10000, 40, 777},
      {1000, 10000, 50, 777},
      {1000, 10000, 60, 777},
      {1000, 10000, 70, 777},
  };

  auto source = std::make_unique<GeneratorSource<RandomRingHypergraph>>(args);
  auto store = std::make_unique<FilesystemStore>("store"s);

  if (!store->initialize()) {
    std::cerr << "Failed to initialize store" << std::endl;
  }

  MinimumCutFinder finder(std::move(source), std::move(store));

  finder.run();

  return 0;
}