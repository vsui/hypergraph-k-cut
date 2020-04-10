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
  double radius = 30;
  size_t num_seeds = 10000;
  std::vector<double> epsilons = {0.1, 1., 10.};

  std::vector<uint64_t> seeds(num_seeds, 0);
  // std::random_device rd;
  // std::transform(seeds.begin(), seeds.end(), seeds.begin(), [&rd](auto) { return rd(); });
  std::iota(seeds.begin(), seeds.end(), 0);

  for (size_t n : {100, 112, 125, 137, 150, 162, 175, 187, 200, 212, 225, 237, 250}) {
    for (auto seed : seeds) {
	    std::cout << "\r" << std::flush << "[" << seed << "/" << seeds.size() << "]" << std::flush;
      const auto gen = RandomRingConstantEdgeHypergraph(n, n * edge_mult, radius, seed);
      const auto hypergraph = std::get<0>(gen.generate());
      Hypergraph copy(hypergraph);
      const auto cut = MW_min_cut(copy);

      const auto skew = static_cast<double>(cut.partitions[0].size()) / hypergraph.num_vertices();
      if (skew < 0.1 || skew > 0.9) {
	// Skip skewed
	continue;
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

      if (std::all_of(cut_factors.begin() + 1 /* ignore 1 */, cut_factors.end(), [](auto factor) { return factor > 1.5; })) {
        for (int i = 0; i < epsilons.size(); ++i) {
	std::cout << "\r" << std::flush;
          std::cout << n << " " << seed << " " << epsilons[i] << " " << cut_factors[i] << std::endl;
        }
	goto outofloop;
      }
      std::cout << "\r" << std::flush;
      std::cout.flush();
    }
outofloop:
    std::cout << "Done with " << n << std::endl;
  }
}
