//
// Created by Victor on 4/9/20.
//

#include <iostream>
#include <random>
#include <algorithm>

#include <generators/generators.hpp>
#include <hypergraph/order.hpp>
#include <hypergraph/approx.hpp>
#include <generators/generators.hpp>
//
//int main(int argc, const char **argv) {
//  size_t edge_mult = 30;
//  double radius = 30;
//  size_t num_seeds = 10000;
//  std::vector<double> epsilons = {0.1, 1., 10.};
//
//  std::vector<uint64_t> seeds(num_seeds, 0);
//  // std::random_device rd;
//  // std::transform(seeds.begin(), seeds.end(), seeds.begin(), [&rd](auto) { return rd(); });
//  std::iota(seeds.begin(), seeds.end(), 0);
//
//  for (size_t n : {100, 112, 125, 137, 150, 162, 175, 187, 200, 212, 225, 237, 250}) {
//    std::mutex mutex;
//    std::atomic_bool done{false};
//    std::atomic_int counter{1};
//    std::for_each(std::execution::par,
//                  seeds.begin(),
//                  seeds.end(),
//                  [&done, &counter, &epsilons, &seeds, n, edge_mult, radius, &mutex](const auto &seed) {
//                    if (done.load()) {
//                      return;
//                    }
//                    {
//                      std::lock_guard lock(mutex);
//                      std::cout << "\r" << std::flush << "[" << counter.fetch_add(1) << "/" << seeds.size() << "]"
//                                << std::flush;
//                    }
//
//                    const auto gen = RandomRingConstantEdgeHypergraph(n, n * edge_mult, radius, seed);
//                    const auto hypergraph = std::get<0>(gen.generate());
//                    Hypergraph copy(hypergraph);
//                    const auto cut = MW_min_cut(copy);
//
//                    const auto skew = static_cast<double>(cut.partitions[0].size()) / hypergraph.num_vertices();
//                    if (skew < 0.1 || skew > 0.9) {
//                      // Skip skewed
//                      return;
//                    }
//
//                    std::vector<double> cut_factors;
//                    std::transform(epsilons.begin(),
//                                   epsilons.end(),
//                                   std::back_inserter(cut_factors),
//                                   [&hypergraph, &cut](const auto epsilon) {
//                                     Hypergraph copy(hypergraph);
//                                     const auto approx_cut = approximate_minimizer(copy, epsilon);
//                                     const auto cut_factor = static_cast<double>(approx_cut.value) / cut.value;
//                                     return cut_factor;
//                                   });
//
//                    if (std::all_of(cut_factors.begin() + 1 /* ignore 1 */,
//cut_factors.
//end(),
//[](
//auto factor
//) {
//return factor > 1.5; })) {
//
//std::lock_guard lock(mutex);
//for (
//int i = 0;
//i<epsilons.
//size();
//++i) {
//if (done.exchange(true)) {
//return;
//}
//std::cout << "\r" << std::flush << n << " " << seed << " " << epsilons[i] << " "
//<< cut_factors[i]
//<<
//std::endl;
//}
//}
//});
//std::cout << "Done with " << n <<
//std::endl;
//}
//}

#include <vector>
#include <string>
#include <fstream>

struct Params {
  size_t num_vertices;
  size_t num_edges;

  std::string name() const {
    using namespace std::string_literals;

    return std::to_string(num_vertices) + "vertices-"s + std::to_string(num_edges) + "edges.csv"s;
  }

  Params(size_t n, size_t m) : num_vertices(n), num_edges(m) {}
};

void find_ring_hypergraphs();

int main() {
  find_ring_hypergraphs();
  return 0;

  std::vector<double> radii(90, 0);
  std::iota(radii.begin(), radii.end(), 5);
  const std::vector<Params> params = {
      // {500, 500},
      {500, 1000}
      // {500, 2500},
      // {500, 5000}
  };

  for (const auto &param : params) {
    std::cout << param.name() << std::endl;
    const auto[n, m] = param;
    std::ofstream out(param.name());
    for (const auto radius : radii) {
      RandomRingConstantEdgeHypergraph gen(n, m, radius, 777);
      auto[h, _] = gen.generate();
      auto cut_val = MW_min_cut_value(h);
      out << radius << "," << cut_val << std::endl;
    }
    out.close();
  }
}

void find_ring_hypergraphs() {
  int num_vertices{100};
  int edge_radius{15};
  int num_edges{200};

  RandomRingConstantEdgeHypergraph gen(num_vertices, num_edges, edge_radius, 777);
  auto[hypergraph, _] = gen.generate();

  /**
   * Get the number of vertices from a cut by taking the sum of the sizes of each partition
   */
  // TODO add to cut.hpp, use namespace
  auto num_vertices_from_cut = [](const auto &cut) {
    return std::accumulate(cut.partitions.begin(),
                           cut.partitions.end(),
                           0,
                           [](auto a, auto b) { return a + b.size(); });
  };

  // TODO put this in cuts.hpp
  /**
   * A measure of how skewed the given cut is.
   *
   * Higher skew factor means the cut is unbalanced, lower skew factor means the cut is balanced.
   */
  auto skew_factor = [num_vertices_from_cut](const auto &cut) {
    auto min_partition = std::min_element(cut.partitions.begin(),
                                          cut.partitions.end(),
                                          [](const auto &a, const auto &b) { return a.size() < b.size(); });
    return static_cast<double>(min_partition->size()) / num_vertices_from_cut(cut);
  };

  const double SKEW_FACTOR = 0.10;

  // Interested in skew and approximate
  auto cut = MW_min_cut(hypergraph);
  if (skew_factor(cut) > 0.30) {
    std::cout << "OK";
  } else {
    std::cout << "MISS ME";
  }
}
