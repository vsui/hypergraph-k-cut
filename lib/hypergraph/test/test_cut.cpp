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

CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(CXY, cxy)
CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(FPZ, fpz)

CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(MW)
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(Q)
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(KW)

template<typename HypergraphType>
HypergraphCut<typename HypergraphType::EdgeWeight> KK_min_cut(const HypergraphType &hypergraph) {
  return kk::minimum_cut(hypergraph, 2);
}

// KK needs a special suite since it takes very long for high-rank hypergraphs
CREATE_HYPERGRAPH_K_CUT_TEST_SUITE2(KK, kk,
                                    up_to_k(tests_in_folder<Hypergraph>("instances/misc/smallrank/unweighted"), 5),
                                    up_to_k(tests_in_folder<WeightedHypergraph<size_t>>(
                                        "instances/misc/smallrank/weighted"), 5),
                                    up_to_k(tests_in_folder<WeightedHypergraph<double>>(
                                        "instances/misc/smallrank/weighted"), 5));

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
    Certificate, MW_certificate, Hypergraph, min_cut_instances(small_unweighted_tests()));