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

// Taking this out will break all of the macros
using namespace hypergraphlib;

CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(CXY, cxy)
CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(FPZ, fpz)

CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(MW)
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(Q)
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(KW)

template<typename HypergraphType>
hypergraphlib::HypergraphCut<typename HypergraphType::EdgeWeight> KK_min_cut(const HypergraphType &hypergraph) {
  return hypergraphlib::kk::minimum_cut(hypergraph, 2);
}

// KK needs a special suite since it takes very long for high-rank hypergraphs
CREATE_HYPERGRAPH_K_CUT_TEST_SUITE2(KK, hypergraphlib::kk,
                                    up_to_k(tests_in_folder<hypergraphlib::Hypergraph>(
                                        "instances/misc/smallrank/unweighted"), 5),
                                    up_to_k(tests_in_folder<hypergraphlib::WeightedHypergraph<size_t>>(
                                        "instances/misc/smallrank/weighted"), 5),
                                    up_to_k(tests_in_folder<hypergraphlib::WeightedHypergraph<double>>(
                                        "instances/misc/smallrank/weighted"), 5));

// Test approximate tests for several approximation factors
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX1_1, approximate_minimizer, 1.1);
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX1_5, approximate_minimizer, 1.5);
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX2_0, approximate_minimizer, 2.0);
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(CX3_0, approximate_minimizer, 3.0);

template<typename HypergraphType>
hypergraphlib::HypergraphCut<typename HypergraphType::EdgeWeight> MW_certificate_min_cut(const HypergraphType &hypergraph) {
  return hypergraphlib::certificate_minimum_cut<HypergraphType>(hypergraph, hypergraphlib::MW_min_cut<HypergraphType>);
}

// Certificate min cut only works for unweighted as of now
CREATE_HYPERGRAPH_MIN_CUT_TEST_FIXTURE(
    Certificate, MW_certificate, hypergraphlib::Hypergraph, min_cut_instances(small_unweighted_tests()));