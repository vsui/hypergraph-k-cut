#include <iostream>

#include "hypergraph/hypergraph.hpp"

namespace hypergraph_util {

template<typename HypergraphType>
using contract_t = std::add_pointer_t<typename HypergraphType::EdgeWeight(HypergraphType &, size_t, size_t)>;

template<typename HypergraphType>
using default_num_runs_t = std::add_pointer_t<size_t(const HypergraphType &, size_t)>;

/* A utility to repeatedly run Monte Carlo minimum cut algorithms like [CXY'18] and [FPZ'19]
 *
 * Arguments:
 *   hypergraph: the hypergraph to calculate the k-cut of
 *   k: k for k-cut
 *   num_runs: number of runs. Pass in 0 to get the number of runs from DefaultNumRuns
 *   verbose: whether or not to log
 */
template<
    typename HypergraphType,
    contract_t<HypergraphType> Contract,
    default_num_runs_t<HypergraphType> DefaultNumRuns>
typename HypergraphType::EdgeWeight minimum_of_runs(const HypergraphType &hypergraph,
                                                    size_t k,
                                                    size_t num_runs,
                                                    bool verbose) {
  if (num_runs == 0) {
    num_runs = DefaultNumRuns(hypergraph, k);
  }
  if (verbose) {
    std::cout << "Running algorithm " << num_runs << " times..." << std::endl;
  }
  typename HypergraphType::EdgeWeight min_so_far = std::numeric_limits<size_t>::max();
  for (size_t i = 0; i < num_runs; ++i) {
    HypergraphType copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    size_t min_cut = Contract(copy, k, /* unused */ 0);
    auto stop = std::chrono::high_resolution_clock::now();
    min_so_far = std::min(min_so_far, min_cut);
    if (verbose) {
      std::cout << "[" << i + 1 << "/" << num_runs << "] took "
                << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                    start)
                    .count()
                << " milliseconds, got " << min_cut << ", min is " << min_so_far
                << "\n";
    }
  }
  return min_so_far;
}
}