#include <tuple>

#include "gtest/gtest.h"

#include "hypergraph.h"
#include "order.h"

namespace {

// d1 and d2 are useful for testing but are not actually directly used in
// computing vertex orderings

// Implements the `d` function as described in CX.
//
// `d` takes as input two disjoint sets of vertices and returns the total
// capacity of the hyperedges that are incident on both sets.
template <typename InputIt>
int d1(const Hypergraph &hypergraph, InputIt a_begin, InputIt a_end,
       InputIt b_begin, InputIt b_end) {
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
template <typename InputIt>
int d2(const Hypergraph &hypergraph, InputIt a_begin, InputIt a_end,
       InputIt b_begin, InputIt b_end) {
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

template <typename InputIt>
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

template <typename InputIt>
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

template <typename InputIt>
bool verify_queyranne_ordering(const Hypergraph &hypergraph, InputIt begin,
                               InputIt end) {
  // d1(V[:i], i) >= d1(V[:i], j) for all j >= i in ordering
  for (auto i = begin + 1; i != end; ++i) {
    for (auto j = i + 1; j != end; ++j) {
      if (d1(hypergraph, begin, i, i, i + 1) +
              d2(hypergraph, begin, i, i, i + 1) <
          d1(hypergraph, begin, i, j, j + 1) +
              d2(hypergraph, begin, i, j, j + 1)) {
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

    ASSERT_EQ(ordering.size(), hypergraph.vertices().size());

    ASSERT_TRUE(verify_maximum_adjacency_ordering(
        hypergraph, std::begin(ordering), std::end(ordering)));
  }
}

TEST(TightOrdering, Works) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = tight_ordering(hypergraph, 1);

    ASSERT_EQ(ordering.size(), hypergraph.vertices().size());
    ASSERT_TRUE(verify_tight_ordering(hypergraph, std::begin(ordering),
                                      std::end(ordering)));
  }
}

TEST(QueyranneOrdering, Works) {
  for (int i = 1; i <= 10; ++i) {
    Hypergraph hypergraph = factory();

    std::vector<int> ordering = queyranne_ordering(hypergraph, 1);

    ASSERT_EQ(ordering.size(), hypergraph.vertices().size());
    ASSERT_TRUE(verify_queyranne_ordering(hypergraph, std::begin(ordering),
                                          std::end(ordering)));
  }
}

TEST(MaximumAdjacencyMinCut, Works) {
  for (int i = 1; i <= 10; ++i) {
    auto h = factory();
    ASSERT_EQ(vertex_ordering_mincut(h, i, maximum_adjacency_ordering), 3);
  }
}

TEST(TightMinCut, Works) {
  for (int i = 1; i <= 10; ++i) {
    auto h = factory();
    ASSERT_EQ(vertex_ordering_mincut(h, i, tight_ordering), 3);
  }
}

TEST(QueyranneMinCut, Works) {
  for (int i = 1; i <= 10; ++i) {
    auto h = factory();
    ASSERT_EQ(vertex_ordering_mincut(h, i, queyranne_ordering), 3);
  }
}

TEST(KTrimmedCertificate, Works) {
  for (int k = 1; k <= 10; ++k) {
    auto h = factory();
    KTrimmedCertificate certifier(h);
    Hypergraph certificate = certifier.certificate(k);
    ASSERT_EQ(vertex_ordering_mincut(certificate, 1, tight_ordering),
              std::min(k, 3));
  }
}
