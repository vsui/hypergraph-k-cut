#include <tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"
#include "hypergraph/hypergraph.hpp"
#include "hypergraph/order.hpp"
#include "hypergraph/approx.hpp"
#include "hypergraph/registry.hpp"
#include "hypergraph/kk.hpp"

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

TEST(KTrimmedCertificate, Works) {
  for (size_t k = 1; k <= 10; ++k) {
    auto h = factory();
    KTrimmedCertificate certifier(h);
    Hypergraph certificate = certifier.certificate(k);
    const auto f = vertex_ordering_minimum_cut_start_vertex<decltype(h), tight_ordering>;
    ASSERT_EQ(f(certificate, 1).value,
              std::min(k, 3ul));
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

TEST(Hypergraph, InplaceContractSimple) {
  Hypergraph h = {
      {0, 1, 2, 3, 4},
      {
          {0, 1, 2}
      }
  };

  h.contract_in_place(0);

  int new_vertex_id = 5;
  EXPECT_THAT(h.vertices(), testing::UnorderedElementsAre(3, 4, new_vertex_id));
  EXPECT_THAT(h.edges(), testing::IsEmpty());
  EXPECT_EQ(h.num_vertices(), 3);
  EXPECT_EQ(h.num_edges(), 0);
  EXPECT_TRUE(h.is_valid());
}

TEST(Hypergraph, InplaceContractRemovesMultipleEdges) {
  Hypergraph h = {
      {1, 2, 3, 4, 5},
      {
          {1, 2, 3},
          {1, 2},
          {1, 2, 3, 4},
          {4, 5}
      }
  };
  h.contract_in_place(0);
  int new_vertex_id = 6;
  ASSERT_THAT(h.vertices(), testing::UnorderedElementsAre(4, 5, new_vertex_id));
  std::vector<std::pair<int, std::vector<int>>> expected_edges = {
      {2, {4, 6}},
      {3, {4, 5}}
  };
  ASSERT_THAT(h.edges(), testing::UnorderedElementsAreArray(expected_edges));
  ASSERT_EQ(h.num_vertices(), 3);
  ASSERT_EQ(h.num_edges(), 2);
  ASSERT_TRUE(h.is_valid());
}

TEST(Hypergraph, InplaceContractAddsNewVertex) {
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
  h.contract_in_place(0);
  EXPECT_THAT(h.vertices(), testing::UnorderedElementsAre(3, 4, 5, new_vertex_id));
  std::vector<std::pair<int, std::vector<int>>> expected_edges = {
      {1, {3, new_vertex_id}},
      {2, {4, 5, new_vertex_id}},
      {3, {3, new_vertex_id}}
  };
  EXPECT_THAT(h.edges(), testing::UnorderedElementsAreArray(expected_edges));
  EXPECT_EQ(h.num_vertices(), 4);
  EXPECT_EQ(h.num_edges(), 3);
  EXPECT_TRUE(h.is_valid());
}

TEST(Hypergraph, InplaceContractKeepsGraphValid) {
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
  h.contract_in_place(13); // {8, 4}

  EXPECT_TRUE(h.is_valid());
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

namespace {

}

template<typename HypergraphType>
class MinimumCutTest : public ::testing::Test {
  static_assert(
      std::is_same_v<HypergraphType, Hypergraph> || std::is_same_v<HypergraphType, WeightedHypergraph<size_t>>);
public:

  using TestCase = std::pair<const HypergraphType, /* k, cut-value pair */ std::map<size_t, Hypergraph::EdgeWeight>>;

  using MinimumCutFunction = typename HypergraphMinimumCutRegistry<HypergraphType>::MinCutFunction;
  using MinimumCutFunctionWithVertex = typename HypergraphMinimumCutRegistry<HypergraphType>::MinCutFunctionWithVertex;
  using MinimumKCutFunction = typename HypergraphMinimumCutRegistry<HypergraphType>::MinKCutFunction;

  const std::vector<TestCase> test_cases() {
    if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
      return unweighted_test_cases_;
    } else {
      std::vector<TestCase> test_cases_ = weighted_test_cases_;
      // We can reuse unweighted test cases by converting them to unit-weight hypergraphs
      for (const auto &[h, pairs] : unweighted_test_cases_) {
        HypergraphType weighted(h);
        test_cases_.emplace_back(weighted, pairs);
      }
      return test_cases_;
    }
  }

  HypergraphMinimumCutRegistry<HypergraphType> registry;
private:

  using UnweightedTestCase = std::pair<const Hypergraph, std::map<size_t, Hypergraph::EdgeWeight>>;
  using WeightedTestCase = std::pair<const WeightedHypergraph<size_t>, std::map<size_t, size_t>>;
  const std::vector<UnweightedTestCase> unweighted_test_cases_ = {
      {
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2, 9},
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
                  {1, 5}
              }
          ),
          { // k, cut-value tuples
              {2, 3},
              {3, 4},
              {4, 6},
              {5, 7}
          }
      },
      { // Hypergraph with trivial cuts
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2},
                  {3, 4},
                  {5, 6},
                  {7, 8},
                  {9, 10},
              }
          ),
          {
              {2, 0},
              {3, 0},
              {4, 0},
              {5, 0},
              {6, 1},
              {7, 2},
          }
      }
  };
  const std::vector<WeightedTestCase> weighted_test_cases_ = {
      {
          WeightedHypergraph<size_t>(
              {0, 1, 2, 3, 4, 5},
              {
                  {{0, 1, 2}, 3},
                  {{1, 2, 3}, 4},
                  {{3, 4, 5}, 3},
                  {{0, 3, 5}, 7},
                  {{0, 1, 2, 3, 4}, 2}
              }
          ),
          {
              {2, 5},
              {3, 9},
              {4, 12},
              {5, 19}
          }
      },
  };
};

TYPED_TEST_SUITE_P(MinimumCutTest);

TYPED_TEST_P(MinimumCutTest, MinimumCutWorks) {
  for (const auto &[name, minimum_cut] : this->registry.minimum_cut_functions) {
    for (const auto &[hypergraph, cut_values] : this->test_cases()) {
      TypeParam copy(hypergraph);
      const auto cut = minimum_cut(copy);
      std::string error;
      EXPECT_TRUE(cut_is_valid(cut, hypergraph, 2, error)) << name << " produced invalid cut: " << error << "\n" << cut;
      EXPECT_EQ(cut.value, cut_values.at(2)) << name << " produced a non-minimal cut";
    }
  }
}

TYPED_TEST_P(MinimumCutTest, VertexOrderingMinimumCutAnyStartVertexWorks) {
  for (const auto &[name, minimum_cut] : this->registry.minimum_cut_functions_with_vertex) {
    for (const auto &[hypergraph, cut_values] : this->test_cases()) {
      for (const auto v : hypergraph.vertices()) {
        TypeParam copy(hypergraph);
        const auto cut = minimum_cut(copy, v);
        std::string error;
        EXPECT_TRUE(cut_is_valid(cut, hypergraph, 2, error))
                << name << " produced invalid cut: " << error << "\n" << cut;
        EXPECT_EQ(cut.value, cut_values.at(2)) << name << " produced a non-minimal cut";
      }
    }
  }
}

TYPED_TEST_P(MinimumCutTest, MinimumKCutWorks) {
  for (const auto &[name, minimum_k_cut] : this->registry.minimum_k_cut_functions) {
    for (const auto &[hypergraph, cut_values] : this->test_cases()) {
      for (const auto &[k, cut_value] : cut_values) {
        // Technically can skip k = 2 since other test does this.
        TypeParam copy(hypergraph);
        const auto cut = minimum_k_cut(copy, k);
        std::string error;
        EXPECT_TRUE(cut_is_valid(cut, hypergraph, k, error))
                << name << " produced invalid cut: " << error << "\n" << cut;
        EXPECT_EQ(cut.value, cut_values.at(k)) << name << " " << k << "-cut produced a non-minimal cut";
      }
    }
  }
}

TEST(MinimumCut, SparsifierWorks) {
  using UnweightedTestCase = std::pair<const Hypergraph, std::map<size_t, size_t>>;
  std::vector<UnweightedTestCase> test_cases = {
      {
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2, 9},
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
                  {1, 5}
              }
          ),
          { // k, cut-value tuples
              {2, 3},
              {3, 4},
              {4, 6},
              {5, 7}
          }
      },
      { // Hypergraph with trivial cuts
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2},
                  {3, 4},
                  {5, 6},
                  {7, 8},
                  {9, 10},
              }
          ),
          {
              {2, 0},
              {3, 0},
              {4, 0},
              {5, 0},
              {6, 1},
              {7, 2},
          }
      }
  };

  for (auto test_case : test_cases) {
    auto[hypergraph, cuts] = test_case;
    for (auto[k, cut_value] : cuts) {
      if (k == 2) {
        EXPECT_EQ(certificate_minimum_cut<Hypergraph>(hypergraph,
                                                      vertex_ordering_mincut<Hypergraph, queyranne_ordering>).value,
                  cut_value);
      }
    }
  }
}

TEST(ApproxMinCut, Works) {
  using UnweightedTestCase = std::pair<Hypergraph, std::map<size_t, size_t>>;
  std::vector<UnweightedTestCase> test_cases = {
      {
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2, 9},
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
                  {1, 5}
              }
          ),
          { // k, cut-value tuples
              {2, 3},
              {3, 4},
              {4, 6},
              {5, 7}
          }
      },
      { // Hypergraph with trivial cuts
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2},
                  {3, 4},
                  {5, 6},
                  {7, 8},
                  {9, 10},
              }
          ),
          {
              {2, 0},
              {3, 0},
              {4, 0},
              {5, 0},
              {6, 1},
              {7, 2},
          }
      }
  };

  for (auto test_case : test_cases) {
    auto[hypergraph, cuts] = test_case;
    for (auto[k, cut_value] : cuts) {
      if (k == 2) {
        EXPECT_LE(approximate_minimizer(hypergraph, 2).value, 2 * cut_value);
      }
    }
  }
}

TEST(KKApproxMinCut, Works) {
  using UnweightedTestCase = std::pair<Hypergraph, std::map<size_t, size_t>>;
  std::vector<UnweightedTestCase> test_cases = {
      {
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2, 9},
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
                  {1, 5}
              }
          ),
          { // k, cut-value tuples
              {2, 3},
              {3, 4},
              {4, 6},
              {5, 7}
          }
      },
      { // Hypergraph with trivial cuts
          Hypergraph(
              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
              {
                  {1, 2},
                  {3, 4},
                  {5, 6},
                  {7, 8},
                  {9, 10},
              }
          ),
          {
              {2, 0},
              {3, 0},
              {4, 0},
              {5, 0},
              {6, 1},
              {7, 2},
          }
      }
  };

  for (auto test_case : test_cases) {
    auto[hypergraph, cuts] = test_case;
    for (auto[k, cut_value] : cuts) {
      if (k == 2) {
        EXPECT_LE(kk::contract(hypergraph, 7, 2).value, 2 * cut_value);
      }
    }
  }
}

REGISTER_TYPED_TEST_SUITE_P(
    MinimumCutTest,
    MinimumCutWorks,
    VertexOrderingMinimumCutAnyStartVertexWorks,
    MinimumKCutWorks
);

using MinimumCutTestTypes = ::testing::Types<Hypergraph, WeightedHypergraph<size_t>>;

INSTANTIATE_TYPED_TEST_SUITE_P(UnweightedAndWeighted, MinimumCutTest, MinimumCutTestTypes);
