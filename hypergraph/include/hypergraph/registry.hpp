#pragma once

#include <type_traits>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/order.hpp"
#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"

constexpr uint64_t kTestRandomSeed = 408408408408;
constexpr uint64_t kTestNumRuns = 0; // Use default number of runs

/* Registry of cut functions. Adding functions to this will automatically add them to the tests.
 */
template<typename HypergraphType, bool Verbose = false>
struct HypergraphMinimumCutRegistry {
  using MinCutFunction =  MinimumCutFunction<HypergraphType>;
  using MinKCutFunction = MinimumKCutFunction<HypergraphType>;
  using MinCutFunctionWithVertex = MinimumCutFunctionWithVertex<HypergraphType>;

  const std::map<std::string, MinKCutFunction> minimum_k_cut_functions =
      {
          {"CXY", cxy_k_cut_},
          {"FPZ", fpz_k_cut_}
      };

  const std::map<std::string, MinCutFunction> minimum_cut_functions = {
      {"CXY", cxy_minimum_cut_},
      {"FPZ", fpz_minimum_cut_},
      {"KW", vertex_ordering_mincut<HypergraphType, maximum_adjacency_ordering>},
      {"MW", vertex_ordering_mincut<HypergraphType, tight_ordering>},
      {"Q", vertex_ordering_mincut<HypergraphType, queyranne_ordering>},
  };

  const std::map<std::string, MinCutFunctionWithVertex> minimum_cut_functions_with_vertex =
      {
          {"KW", vertex_ordering_minimum_cut_start_vertex<HypergraphType, maximum_adjacency_ordering>},
          {"MW", vertex_ordering_minimum_cut_start_vertex<HypergraphType, tight_ordering>},
          {"Q", vertex_ordering_minimum_cut_start_vertex<HypergraphType, queyranne_ordering>},
      };

private:
  static inline auto cxy_minimum_cut_(const HypergraphType &hypergraph) {
    return cxy::minimum_cut<HypergraphType, Verbose>(hypergraph, 2, kTestNumRuns, kTestRandomSeed);
  };

  static inline auto fpz_minimum_cut_(const HypergraphType &hypergraph) {
    return fpz::minimum_cut<HypergraphType, Verbose>(hypergraph, 2, kTestNumRuns, kTestRandomSeed);
  };

  static inline auto cxy_k_cut_(const HypergraphType &hypergraph, size_t k) {
    return cxy::minimum_cut<HypergraphType, Verbose>(hypergraph, k, kTestNumRuns, kTestRandomSeed);
  }

  static inline auto fpz_k_cut_(const HypergraphType &hypergraph, size_t k) {
    return fpz::minimum_cut<HypergraphType, Verbose>(hypergraph, k, kTestNumRuns, kTestRandomSeed);
  }
};
