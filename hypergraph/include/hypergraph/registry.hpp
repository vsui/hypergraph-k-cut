#pragma once

#include <type_traits>

#include "hypergraph/hypergraph.hpp"

/* Registry of cut functions. Adding functions to this will automatically add them to the CLI/tests.
 */
template<typename HypergraphType>
struct HypergraphMinimumCutRegistry {
  using MinimumCutFunction = std::add_pointer_t<HypergraphCut<HypergraphType>(const HypergraphType &)>;
  using MinimumCutFunctionWithVertex = std::add_pointer_t<HypergraphCut<HypergraphType>(const HypergraphType &, int)>;
  using MinimumKCutFunction = std::add_pointer_t<HypergraphCut<HypergraphType>(const HypergraphType &, size_t k)>;

  const std::map<std::string, MinimumKCutFunction> minimum_k_cut_functions =
      {
          {"CXY", cxy_k_cut_},
          {"FPZ", fpz_k_cut_}
      };

  const std::map<std::string, MinimumCutFunction> minimum_cut_functions = {
      {"CXY", cxy_minimum_cut_},
      {"FPZ", fpz_minimum_cut_},
      {"KW", (MinimumCutFunction) vertex_ordering_mincut<HypergraphType, maximum_adjacency_ordering>},
      {"MW", (MinimumCutFunction) vertex_ordering_mincut<HypergraphType, tight_ordering>},
      {"Q", (MinimumCutFunction) vertex_ordering_mincut<HypergraphType, queyranne_ordering>},
  };

  const std::map<std::string, MinimumCutFunctionWithVertex> minimum_cut_functions_with_vertex =
      {
          {"KW", (MinimumCutFunctionWithVertex) vertex_ordering_minimum_cut_start_vertex<HypergraphType,
                                                                                         maximum_adjacency_ordering>},
          {"MW",
           (MinimumCutFunctionWithVertex) vertex_ordering_minimum_cut_start_vertex<HypergraphType, tight_ordering>},
          {"Q",
           (MinimumCutFunctionWithVertex) vertex_ordering_minimum_cut_start_vertex<HypergraphType, queyranne_ordering>},
      };

private:
  static inline auto cxy_minimum_cut_(const HypergraphType &hypergraph) {
    return cxy::cxy_contract(hypergraph, 2);
  };

  static inline auto fpz_minimum_cut_(const HypergraphType &hypergraph) {
    return fpz::branching_contract(hypergraph, 2);
  };

  static inline auto cxy_k_cut_(const HypergraphType &hypergraph, size_t k) {
    return cxy::cxy_contract(hypergraph, k);
  }

  static inline auto fpz_k_cut_(const HypergraphType &hypergraph, size_t k) {
    return fpz::branching_contract(hypergraph, k);
  }
};
