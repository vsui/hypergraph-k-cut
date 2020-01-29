#include <gtest/gtest.h>

#include "suite.hpp"
#include "testutil.hpp"

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"
#include "hypergraph/approx.hpp"
#include "hypergraph/order.hpp"
#include "hypergraph/kk.hpp"
#include "hypergraph/certificate.hpp"

CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(CXY, cxy::cxy_contract);
CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(FPZ, fpz::branching_contract);

CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(MW, MW_min_cut);
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(Q, Q_min_cut);
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(KW, KW_min_cut);

template<typename HypergraphType>
HypergraphCut<typename HypergraphType::EdgeWeight> KK_min_cut(const HypergraphType &hypergraph) {
  return kk::contract(hypergraph, 2);
}

// KK needs a special suite since it takes very long for high-rank hypergraphs
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE2(KK, KK_min_cut,
                                      tests_in_folder<Hypergraph>("instances/misc/smallrank/unweighted"),
                                      tests_in_folder<WeightedHypergraph<size_t>>("instances/misc/smallrank/weighted"),
                                      tests_in_folder<WeightedHypergraph<double>>("instances/misc/smallrank/weighted"));

// Test approximate tests for several approximation factors
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX1_1, approximate_minimizer, 1.1);
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX1_5, approximate_minimizer, 1.5);
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX2_0, approximate_minimizer, 2.0);
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX3_0, approximate_minimizer, 3.0);

template<typename HypergraphType>
HypergraphCut<typename HypergraphType::EdgeWeight> MW_certificate_min_cut(const HypergraphType &hypergraph) {
  return certificate_minimum_cut<HypergraphType>(hypergraph, MW_min_cut<HypergraphType>);
}

// Certificate min cut only works for unweighted as of now
CREATE_HYPERGRAPH_MIN_CUT_TEST_FIXTURE(
    Certificate, MW_certificate_min_cut, Hypergraph, min_cut_instances(small_unweighted_tests()));