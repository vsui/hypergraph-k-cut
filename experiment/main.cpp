#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  std::vector<RandomRingConstantEdgeHypergraph::ConstructorArgs> args;

  for (size_t num_vertices : {100, 1000}) {
    for (size_t num_edges : {100, 500, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000}) {
      for (size_t radius = 1; radius <= 30; radius += 2) {
        args.emplace_back(num_vertices, num_edges, radius, 777);
      }
    }
  }

  auto source = std::make_unique<GeneratorSource<RandomRingConstantEdgeHypergraph>>(args);
  auto store = std::make_unique<FilesystemStore>("store"s);

  if (!store->initialize()) {
    std::cerr << "Failed to initialize store" << std::endl;
  }

  MinimumCutFinder finder(std::move(source), std::move(store));

  finder.run();

  return 0;
}