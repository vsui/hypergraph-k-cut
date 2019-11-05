#include <tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"
#include "hypergraph/hypergraph.hpp"
#include "hypergraph/order.hpp"

namespace {

// d1 and d2 are useful for testing but are not actually directly used in
// computing vertex orderings

// Implements the `d` function as described in CX.
//
// `d` takes as input two disjoint sets of vertices and returns the total
// capacity of the hyperedges that are incident on both sets.
template<typename InputIt, typename InputIt2>
int d1(const Hypergraph &hypergraph, InputIt a_begin, InputIt a_end,
       InputIt2 b_begin, InputIt2 b_end) {
  int d = 0;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    const bool e_intersects_a =
        std::any_of(std::begin(vertices), std::end(vertices),
                    [a_begin, a_end](const int v) {
                      return std::find(a_begin, a_end, v) != a_end;
                    });
    const bool e_intersects_b =
        std::any_of(std::begin(vertices), std::end(vertices),
                    [b_begin, b_end](const int v) {
                      return std::find(b_begin, b_end, v) != b_end;
                    });
    if (e_intersects_a && e_intersects_b) {
      ++d;
    }
  }
  return d;
}

// Implements the `d'` function as described in CX.
//
// `d'` takes in two disjoint sets of vertices and returns the total capacity
// of hyperedges e such that e is incident on both sets and only consists of
// vertices that are in one of the two input sets.
template<typename InputIt, typename InputIt2>
int d2(const Hypergraph &hypergraph, InputIt a_begin, InputIt a_end,
       InputIt2 b_begin, InputIt2 b_end) {
  int d = 0;
  for (const auto &[e, vertices] : hypergraph.edges()) {
    const bool e_intersects_a =
        std::any_of(std::begin(vertices), std::end(vertices),
                    [a_begin, a_end](const int v) {
                      return std::find(a_begin, a_end, v) != a_end;
                    });
    const bool e_intersects_b =
        std::any_of(std::begin(vertices), std::end(vertices),
                    [b_begin, b_end](const int v) {
                      return std::find(b_begin, b_end, v) != b_end;
                    });
    const bool e_subset_of_a_and_b =
        std::all_of(std::begin(vertices), std::end(vertices),
                    [a_begin, a_end, b_begin, b_end](const int v) {
                      return std::find(a_begin, a_end, v) != a_end ||
                          std::find(b_begin, b_end, v) != b_end;
                    });
    if (e_intersects_a && e_intersects_b && e_subset_of_a_and_b) {
      ++d;
    }
  }
  return d;
}

template<typename InputIt1, typename InputIt2>
double d3(const Hypergraph &hypergraph, InputIt1 a_begin, InputIt1 a_end,
          InputIt2 b_begin, InputIt2 b_end) {
  return 0.5 * (d1(hypergraph, a_begin, a_end, b_begin, b_end) +
      d2(hypergraph, a_begin, a_end, b_begin, b_end));
}

// Hypergraph cut function
template<typename InputIt>
int cut(const Hypergraph &hypergraph, InputIt a_begin, InputIt a_end) {
  std::unordered_set<int> b;
  for (const auto v : hypergraph.vertices()) {
    if (std::find(a_begin, a_end, v) == a_end) {
      b.emplace(v);
    }
  }

  // Can return either d1 or d2
  return d1(hypergraph, a_begin, a_end, std::begin(b), std::end(b));
}

// Hypergraph cut connectivity function (see [CX'18])
//
// Connectivity function of a function f is d_f(A, B) = 1/2 * (f(A) + f(B) - f(A
// U B)) where A and B are disjoint
template<typename InputIt>
double connectivity(const Hypergraph &hypergraph, InputIt a_begin,
                    InputIt a_end, InputIt b_begin, InputIt b_end) {
  std::unordered_set<int> a_and_b;
  std::for_each(a_begin, a_end, [&a_and_b](const int v) { a_and_b.insert(v); });
  std::for_each(b_begin, b_end, [&a_and_b](const int v) { a_and_b.insert(v); });
  return 0.5 *
      (cut(hypergraph, a_begin, a_end) + cut(hypergraph, b_begin, b_end) -
          cut(hypergraph, std::begin(a_and_b), std::end(a_and_b)));
}

// Returns a hypergraph to test on
Hypergraph factory() {
  return Hypergraph({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {{1, 2, 9},
                                                      {1, 3, 9},
                                                      {1, 2, 5, 7, 8},
                                                      {3, 5, 8},
                                                      {2, 5, 6},
                                                      {6, 7, 9},
                                                      {2, 3, 10},
                                                      {5, 10},
                                                      {1, 4},
                                                      {4, 8, 10},
                                                      {1, 2, 3},
                                                      {1, 2, 3, 4, 5, 6, 7},
                                                      {1, 5}});
}

WeightedHypergraph<size_t> weighted_factory() {
  const WeightedHypergraph<size_t> h = {
      {0, 1, 2, 3, 4, 5},
      {
          {{0, 1, 2}, 3},
          {{1, 2, 3}, 4},
          {{3, 4, 5}, 3},
          {{0, 3, 5}, 7},
          {{0, 1, 2, 3, 4}, 2}
      }
  };
  return h;
}

template<typename InputIt>
bool verify_maximum_adjacency_ordering(const Hypergraph &hypergraph,
                                       InputIt begin, InputIt end) {
  // d1(V[:i], i) >= d1(V[:i], j) for all j >= i in ordering
  for (auto i = begin + 1; i != end; ++i) {
    for (auto j = i + 1; j != end; ++j) {
      if (d1(hypergraph, begin, i, i, i + 1) <
          d1(hypergraph, begin, i, j, j + 1)) {
        return false;
      }
    }
  }
  return true;
}

template<typename InputIt>
bool verify_tight_ordering(const Hypergraph &hypergraph, InputIt begin,
                           InputIt end) {
  // d2(V[:i], i) >= d2(V[:i], j) for all j >= i in ordering
  for (auto i = begin + 1; i != end; ++i) {
    for (auto j = i + 1; j != end; ++j) {
      if (d2(hypergraph, begin, i, i, i + 1) <
          d2(hypergraph, begin, i, j, j + 1)) {
        return false;
      }
    }
  }
  return true;
}

template<typename InputIt>
bool verify_queyranne_ordering(const Hypergraph &hypergraph, InputIt begin,
                               InputIt end) {
  // d1(V[:i], i) >= d1(V[:i], j) for all j >= i in ordering
  for (auto i = begin + 1; i != end; ++i) {
    for (auto j = i + 1; j != end; ++j) {
      if (d3(hypergraph, begin, i, i, i + 1) <
          d3(hypergraph, begin, i, j, j + 1)) {
        return false;
      }
    }
  }
  return true;
}

template<typename InputIt>
bool verify_queyranne_ordering2(const Hypergraph &hypergraph, InputIt begin,
                                InputIt end) {
  for (auto i = begin + 1; i != end; ++i) {
    for (auto j = i + 1; j != end; ++j) {
      if (connectivity(hypergraph, begin, i, i, i + 1) <
          connectivity(hypergraph, begin, i, j, j + 1)) {
        return false;
      }
    }
  }
  return true;
}

template<typename InputIt>
bool verify_connectivity_is_d3(const Hypergraph &hypergraph, InputIt begin,
                               InputIt end) {
  for (auto i = begin + 1; i != end; ++i) {
    for (auto j = i + 1; j != end; ++j) {
      if (connectivity(hypergraph, begin, i, i, i + 1) !=
          d3(hypergraph, begin, i, i, i + 1)) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

TEST(MaximumAdjacencyOrdering, Works) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = maximum_adjacency_ordering(hypergraph, i);

    ASSERT_EQ(ordering.size(), hypergraph.num_vertices());

    ASSERT_TRUE(verify_maximum_adjacency_ordering(
        hypergraph, std::begin(ordering), std::end(ordering)));
  }
}

TEST(TightOrdering, Works) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = tight_ordering(hypergraph, i);

    ASSERT_EQ(ordering.size(), hypergraph.num_vertices());
    ASSERT_TRUE(verify_tight_ordering(hypergraph, std::begin(ordering),
                                      std::end(ordering)));
  }
}

TEST(QueyranneOrdering, OrderedByD3) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = queyranne_ordering(hypergraph, i);

    ASSERT_EQ(ordering.size(), hypergraph.num_vertices());
    ASSERT_TRUE(verify_queyranne_ordering(hypergraph, std::begin(ordering),
                                          std::end(ordering)));
  }
}

TEST(QueyranneOrdering, OrderedByConnectivity) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = queyranne_ordering(hypergraph, i);

    ASSERT_EQ(ordering.size(), hypergraph.num_vertices());
    ASSERT_TRUE(verify_queyranne_ordering2(hypergraph, std::begin(ordering),
                                           std::end(ordering)));
  }
}

TEST(QueyranneOrdering, ConnectivityIsD3) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = queyranne_ordering(hypergraph, i);

    ASSERT_EQ(ordering.size(), hypergraph.num_vertices());
    ASSERT_TRUE(verify_connectivity_is_d3(hypergraph, std::begin(ordering),
                                          std::end(ordering)));
  }
}

TEST(QueyranneOrdering, TightnessMatchesConnectivity) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    const auto &[ordering, tightness] =
    queyranne_ordering_with_tightness(hypergraph, i);

    ASSERT_EQ(ordering.size(), hypergraph.num_vertices());
    ASSERT_EQ(ordering.size(), tightness.size());

    auto tightness_it = std::begin(tightness);
    for (auto it = std::begin(ordering); it != std::end(ordering); ++it) {
      auto conn = connectivity(hypergraph, std::begin(ordering), it, it, it + 1);
      EXPECT_EQ(conn, *tightness_it);
      tightness_it++;
    }
    ASSERT_EQ(tightness_it, std::end(tightness));
  }
}

TEST(QueyranneOrdering, UnweightedVsWeightedSanityCheck) {
  for (int i = 1; i <= 10; ++i) {
    auto unweighted = factory();
    auto weighted = WeightedHypergraph<size_t>(unweighted);

    const auto unweighted_cut = vertex_ordering_mincut<decltype(unweighted), queyranne_ordering>(unweighted, i);
    const auto weighted_cut = vertex_ordering_mincut<decltype(weighted), queyranne_ordering>(weighted, i);

    ASSERT_EQ(unweighted_cut, weighted_cut);
  }
}

TEST(MaximumAdjacencyMinCut, Works) {
  for (int i = 1; i <= 10; ++i) {
    auto h = factory();
    const auto f = vertex_ordering_mincut<decltype(h), maximum_adjacency_ordering>;
    ASSERT_EQ(f(h, i),
              3);
  }
}

TEST(MaximumAdjacencyMinCut, WeightedUnweightedSanityCheck) {
  for (int i = 1; i <= 10; ++i) {
    auto unweighted = factory();
    auto weighted = WeightedHypergraph<size_t>(unweighted);

    const auto unweighted_cut = vertex_ordering_mincut<decltype(unweighted), maximum_adjacency_ordering>(unweighted, i);
    const auto weighted_cut = vertex_ordering_mincut<decltype(weighted), maximum_adjacency_ordering>(weighted, i);

    ASSERT_EQ(unweighted_cut, weighted_cut);
  }
}

TEST(TightMinCut, Works) {
  for (int i = 1; i <= 10; ++i) {
    auto h = factory();
    const auto f = vertex_ordering_mincut<decltype(h), tight_ordering>;
    ASSERT_EQ(f(h, i), 3);
  }
}

TEST(QueyranneMinCut, Works) {
  for (int i = 1; i <= 10; ++i) {
    auto h = factory();
    const auto f = vertex_ordering_mincut<decltype(h), queyranne_ordering>;
    ASSERT_EQ(f(h, i), 3);
  }
}

TEST(MaximumAdjacencyMinCut, WeightedWorks) {
  const auto factory = weighted_factory();
  for (int i : factory.vertices()) {
    auto h = weighted_factory();
    const auto f = vertex_ordering_mincut<decltype(h), maximum_adjacency_ordering>;
    ASSERT_EQ(f(h, i), 5);
  }
}

TEST(TightOrderingMinCut, WeightedWorks) {
  const auto factory = weighted_factory();
  for (int i : factory.vertices()) {
    auto h = weighted_factory();
    const auto f = vertex_ordering_mincut<decltype(h), tight_ordering>;
    ASSERT_EQ(f(h, i), 5);
  }
}

TEST(QueyranneOrderingMinCut, WeightedWorks) {
  const auto factory = weighted_factory();
  for (int i : factory.vertices()) {
    auto h = weighted_factory();
    const auto f = vertex_ordering_mincut<decltype(h), queyranne_ordering>;
    ASSERT_EQ(f(h, i), 5);
  }
}

TEST(KTrimmedCertificate, Works) {
  for (size_t k = 1; k <= 10; ++k) {
    auto h = factory();
    KTrimmedCertificate certifier(h);
    Hypergraph certificate = certifier.certificate(k);
    const auto f = vertex_ordering_mincut<decltype(h), tight_ordering>;
    ASSERT_EQ(f(certificate, 1),
              std::min(k, 3ul));
  }
}

TEST(KCut, SanityCheck) {
  for (size_t k = 3; k <= 5; ++k) {
    Hypergraph h1 = factory();
    Hypergraph h2 = factory();

    ASSERT_EQ(cxy::cxy_contract(h1, k), fpz::branching_contract(h2, k));
  }
}

TEST(KCut, WeightedSanityCheck) {
  const WeightedHypergraph<size_t> h = {
      {0, 1, 2, 3, 4, 5},
      {
          {{0, 1, 2}, 3},
          {{1, 2, 3}, 4},
          {{3, 4, 5}, 3},
          {{0, 3, 5}, 7},
          {{0, 1, 2, 3, 4}, 2}
      }
  };

  for (size_t k = 3; k <= 5; ++k) {
    ASSERT_EQ(cxy::cxy_contract(h, k), fpz::branching_contract(h, k));
  }
}

TEST(KCut, CXY) {
  Hypergraph h = factory();
  size_t ans = cxy::cxy_contract(h, 4);
  ASSERT_EQ(ans, 6);
}

TEST(KCut, WeightedCXY) {
  WeightedHypergraph<size_t> h = {
      {0, 1, 2, 3, 4, 5},
      {
          {{0, 1, 2}, 3},
          {{1, 2, 3}, 4},
          {{3, 4, 5}, 3},
          {{0, 3, 5}, 7},
          {{0, 1, 2, 3, 4}, 2}
      }
  };
  ASSERT_EQ(cxy::cxy_contract(h, 3), 9);
}

TEST(KCut, UnweightedVsWeightedSanityCXY) {
  for (size_t k = 3; k <= 5; ++k) {
    Hypergraph h1 = factory();
    WeightedHypergraph<size_t> h2(h1);

    ASSERT_EQ(cxy::cxy_contract(h1, k), cxy::cxy_contract(h2, k));
  }
}

TEST(KCut, FPZ) {
  Hypergraph h = factory();
  size_t ans = fpz::branching_contract(h, 4);
  ASSERT_EQ(ans, 6);
}

TEST(KCut, WeightedFPZ) {
  WeightedHypergraph<size_t> h = {
      {0, 1, 2, 3, 4, 5},
      {
          {{0, 1, 2}, 3},
          {{1, 2, 3}, 4},
          {{3, 4, 5}, 3},
          {{0, 3, 5}, 7},
          {{0, 1, 2, 3, 4}, 2}
      }
  };
  ASSERT_EQ(fpz::branching_contract(h, 3), 9);
}

TEST(KCut, UnweightedVsWeightedSanityFPZ) {
  for (size_t k = 3; k <= 5; ++k) {
    Hypergraph h1 = factory();
    WeightedHypergraph<size_t> h2(h1);

    ASSERT_EQ(fpz::branching_contract(h1, k), fpz::branching_contract(h2, k));
  }
}

TEST(Hypergraph, ContractSimple) {
  Hypergraph h = {
      {0, 1, 2, 3, 4},
      {
          {0, 1, 2}
      }
  };
  Hypergraph contracted = h.contract(0);

  int new_vertex_id = 5;
  EXPECT_THAT(contracted.vertices(), testing::UnorderedElementsAre(3, 4, new_vertex_id));
  EXPECT_THAT(contracted.edges(), testing::IsEmpty());
  EXPECT_EQ(contracted.num_vertices(), 3);
  EXPECT_EQ(contracted.num_edges(), 0);
  EXPECT_TRUE(contracted.is_valid());
}

TEST(Hypergraph, ContractRemovesMultipleEdges) {
  Hypergraph h = {
      {1, 2, 3, 4, 5},
      {
          {1, 2, 3},
          {1, 2},
          {1, 2, 3, 4},
          {4, 5}
      }
  };
  Hypergraph contracted = h.contract(0);
  int new_vertex_id = 6;
  ASSERT_THAT(contracted.vertices(), testing::UnorderedElementsAre(4, 5, new_vertex_id));
  std::vector<std::pair<int, std::vector<int>>> expected_edges = {
      {2, {4, 6}},
      {3, {4, 5}}
  };
  ASSERT_THAT(contracted.edges(), testing::UnorderedElementsAreArray(expected_edges));
  ASSERT_EQ(contracted.num_vertices(), 3);
  ASSERT_EQ(contracted.num_edges(), 2);
  ASSERT_TRUE(contracted.is_valid());
}

TEST(Hypergraph, ContractAddsNewVertex) {
  // TODO contract returns new vertex id
  Hypergraph h = {
      {1, 2, 3, 4, 5},
      {
          {1, 2},
          {1, 2, 3},
          {2, 4, 5},
          {1, 3}
      }

  };
  int new_vertex_id = 6;
  Hypergraph contracted = h.contract(0);
  EXPECT_THAT(contracted.vertices(), testing::UnorderedElementsAre(3, 4, 5, new_vertex_id));
  std::vector<std::pair<int, std::vector<int>>> expected_edges = {
      {1, {3, new_vertex_id}},
      {2, {4, 5, new_vertex_id}},
      {3, {3, new_vertex_id}}
  };
  EXPECT_THAT(contracted.edges(), testing::UnorderedElementsAreArray(expected_edges));
  EXPECT_EQ(contracted.num_vertices(), 4);
  EXPECT_EQ(contracted.num_edges(), 3);
  EXPECT_TRUE(contracted.is_valid());
}

TEST(Hypergraph, ContractKeepsGraphValid) {
  Hypergraph h = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {
          {9, 2, 1}, // 0
          {9, 3, 1},
          {8, 5, 2, 1, 0},
          {8, 5, 3},
          {5, 2, 0},
          {9, 0}, // 5
          {10, 3, 2},
          {10, 5},
          {4, 1},
          {10, 8, 4},
          {3, 2, 1}, // 10
          {5, 4, 3, 2, 1, 0},
          {5, 1},
          {8, 4},
      }
  };
  Hypergraph contracted = h.contract(13); // {8, 4}

  EXPECT_TRUE(contracted.is_valid());
}

TEST(Hypergraph, RemoveHyperedgeSimple) {
  Hypergraph h = {
      {2, 4, 5, 6},
      {
          {2, 4, 5},
          {2, 4},
          {5, 6}
      }
  };
  h.remove_hyperedge(0);
  ASSERT_THAT(h.vertices(), testing::UnorderedElementsAre(2, 4, 5, 6));
  std::vector<std::pair<int, std::vector<int>>> expected_edges = {
      {1, {2, 4}}, {2, {5, 6}}
  };
  ASSERT_THAT(h.edges(),
              testing::UnorderedElementsAreArray(expected_edges));
  ASSERT_TRUE(h.is_valid());
}

TEST(Hypergraph, RemoveHyperedgeKeepsGraphValid) {
  Hypergraph h = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {
          {9, 2, 1}, // 0
          {9, 3, 1},
          {8, 5, 2, 1, 0},
          {8, 5, 3},
          {5, 2, 0},
          {9, 0}, // 5
          {10, 3, 2},
          {10, 5},
          {4, 1},
          {10, 8, 4},
          {3, 2, 1}, // 10
          {5, 4, 3, 2, 1, 0},
          {5, 1},
          {8, 4},
      }
  };
  h.remove_hyperedge(11);

  EXPECT_TRUE(h.is_valid());
}

TEST(Hypergraph, RemoveHyperedgeKeepsGraphValidRepeated) {
  Hypergraph h = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {
          {9, 2, 1}, // 0
          {9, 3, 1},
          {8, 5, 2, 1, 0},
          {8, 5, 3},
          {5, 2, 0},
          {9, 0}, // 5
          {10, 3, 2},
          {10, 5},
          {4, 1},
          {10, 8, 4},
          {3, 2, 1}, // 10
          {5, 4, 3, 2, 1, 0},
          {5, 1},
          {8, 4},
      }
  };

  while (h.num_edges() > 0) {
    const auto &[edge_id, vertices] = *std::begin(h.edges());
    h.remove_hyperedge(edge_id);
    ASSERT_TRUE(h.is_valid());
  }
}
