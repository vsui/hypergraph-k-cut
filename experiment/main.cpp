#include <memory>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

int main() {
  using namespace std::string_literals;

  // std::vector<RandomRingConstantEdgeHypergraph::ConstructorArgs> const_ring_args;
  std::vector<RandomRingVariableEdgeHypergraph::ConstructorArgs> var_ring_args;
  for (size_t num_vertices : {100, 1000, 2000, 3000, 4000, 5000}) {
    for (size_t num_edges : {100, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000}) {
      for (size_t radius = 1; radius <= 30; radius += 2) {
        for (size_t variance = 1; variance <= 10; ++variance) {
          var_ring_args.emplace_back(num_vertices, num_edges, radius, variance, 777);
        }
      }
    }
  }

  std::vector<MXNHypergraph::ConstructorArgs> mxn_args;
  for (size_t m : {100, 1000, 3000, 5000, 7000, 9000, 10000, 12000, 14000, 16000, 18000, 20000}) {
    for (size_t n : {100, 1000, 2500, 5000}) {
      for (size_t p : {0.1, 0.15, 0.2, 0.25, 0.3, 0.35}) {
        mxn_args.emplace_back(n, m, p, 777);
      }
    }
  }

  using Ptr = std::unique_ptr<HypergraphSource>;

  // Why can't I list-initialize this
  std::vector<Ptr> sources;
  sources.emplace_back(std::make_unique<GeneratorSource<RandomRingVariableEdgeHypergraph>>(var_ring_args));
  sources.emplace_back(std::make_unique<GeneratorSource<MXNHypergraph>>(mxn_args));

  std::unique_ptr<AggregateSource> source = std::make_unique<AggregateSource>(std::move(sources));

  auto store = std::make_unique<FilesystemStore>("store"s);

  if (!store->initialize()) {
    std::cerr << "Failed to initialize store" << std::endl;
  }

  MinimumCutFinder finder(std::move(source), std::move(store));

  finder.run();

  return 0;
}