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

  using Ptr = std::unique_ptr<HypergraphSource>;

  auto source = std::make_unique<KCoreSource>(std::make_unique<GeneratorSource<MXNHypergraph>>(mxn_args));

  auto store = std::make_unique<FilesystemStore>("store"s);

  if (!store->initialize()) {
    std::cerr << "Failed to initialize store" << std::endl;
  }

  MinimumCutFinder finder(std::move(source), std::move(store));

  finder.run();

  return 0;
}