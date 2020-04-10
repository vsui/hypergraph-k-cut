//
// Created by Victor on 4/9/20.
//

#include <iostream>
#include <random>

#include <generators/generators.hpp>
#include <hypergraph/order.hpp>
#include <hypergraph/approx.hpp>

int main(int argc, const char **argv) {
  size_t edge_mult = 30;
  size_t radius = 30;
  size_t num_seeds = 10;
  std::vector<double> epsilons = {1, 10};

  std::vector<uint64_t> seeds(num_seeds, 0);
  // std::random_device rd;
  // std::transform(seeds.begin(), seeds.end(), seeds.begin(), [&rd](auto) { return rd(); });
  std::iota(seeds.begin(), seeds.end(), 0);

  for (size_t n : {100, 112, 125, 137, 150}) {
    for (auto seed : seeds) {
      const auto gen = RandomRingConstantEdgeHypergraph(n, n * edge_mult, radius, seed);
      const auto hypergraph = std::get<0>(gen.generate());
      Hypergraph copy(hypergraph);
      const auto cut = MW_min_cut(copy);

      const auto skew = static_cast<double>(cut.partitions[0].size()) / hypergraph.num_vertices();
      if (skew < 0.1 || skew > 0.9) {
        std::cout << "Skewed cut, skipping..." << std::endl;
      }

      std::vector<double> cut_factors;
      std::transform(epsilons.begin(),
                     epsilons.end(),
                     std::back_inserter(cut_factors),
                     [&hypergraph, &cut](const auto epsilon) {
                       Hypergraph copy(hypergraph);
                       const auto approx_cut = approximate_minimizer(copy, epsilon);
                       const auto cut_factor = static_cast<double>(approx_cut.value) / cut.value;
                       return cut_factor;
                     });

      if (std::all_of(cut_factors.begin(), cut_factors.end(), [](auto factor) { return factor > 1.5; })) {
        std::cout << "Interesting cut factor" << std::endl;
        for (int i = 0; i < epsilons.size(); ++i) {
          std::cout << n << " " << seed << " " << epsilons[i] << " " << cut_factors[i] << std::endl;
        }
      }
    }
  }
}