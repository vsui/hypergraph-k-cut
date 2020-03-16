//
// Created by Victor on 1/27/20.
//

#define CREATE_HYPERGRAPH_K_CUT_TEST_SUITE2(name, ns, unweighted, weighted1, weighted2) \
CREATE_HYPERGRAPH_K_CUT_TEST_FIXTURE(name##Unweighted, ns, Hypergraph, unweighted) \
CREATE_HYPERGRAPH_K_CUT_TEST_FIXTURE(name##WeightedIntegral, ns, WeightedHypergraph<size_t>, weighted1) \
CREATE_HYPERGRAPH_K_CUT_TEST_FIXTURE(name##WeightedFloating, ns, WeightedHypergraph<double>, weighted2) \
CREATE_HYPERGRAPH_K_CUT_VALUE_TEST_FIXTURE(name##ValueUnweighted, ns, Hypergraph, unweighted) \
CREATE_HYPERGRAPH_K_CUT_VALUE_TEST_FIXTURE(name##ValueWeightedIntegral, ns, WeightedHypergraph<size_t>, weighted1) \
CREATE_HYPERGRAPH_K_CUT_VALUE_TEST_FIXTURE(name##ValueWeightedFloating, ns, WeightedHypergraph<double>, weighted2) \
CREATE_HYPERGRAPH_K_CUT_DISCOVERY_TEST_FIXTURE(name##DiscoveryUnweighted, ns, Hypergraph, unweighted) \
CREATE_HYPERGRAPH_K_CUT_DISCOVERY_TEST_FIXTURE(name##DiscoveryWeightedIntegral, ns, WeightedHypergraph<size_t>, weighted1) \
CREATE_HYPERGRAPH_K_CUT_DISCOVERY_TEST_FIXTURE(name##DiscoveryWeightedFloating, ns, WeightedHypergraph<double>, weighted2) \
CREATE_HYPERGRAPH_K_CUT_DISCOVERY_VALUE_TEST_FIXTURE(name##DiscoveryValueUnweighted, ns, Hypergraph, unweighted) \
CREATE_HYPERGRAPH_K_CUT_DISCOVERY_VALUE_TEST_FIXTURE(name##DiscoveryValueWeightedIntegral, ns, WeightedHypergraph<size_t>, weighted1) \
CREATE_HYPERGRAPH_K_CUT_DISCOVERY_VALUE_TEST_FIXTURE(name##DiscoveryValueWeightedFloating, ns, WeightedHypergraph<double>, weighted2)

#define CREATE_HYPERGRAPH_K_CUT_TEST_SUITE(name, ns) \
CREATE_HYPERGRAPH_K_CUT_TEST_SUITE2(name, ns, small_unweighted_tests(), small_weighted_tests<size_t>(), small_weighted_tests<double>())

#define CREATE_HYPERGRAPH_K_CUT_TEST_FIXTURE(name, ns, HypergraphType, values) \
class name##Test : public testing::TestWithParam<TestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  const auto &[hypergraph, cut_pair, filename] = GetParam(); \
  const auto [k, cut_value] = cut_pair; \
  const HypergraphType copy(hypergraph); \
  const auto cut = ns::minimum_cut(copy, k); \
  std::string error; \
  EXPECT_TRUE(cut_is_valid(cut, hypergraph, k, error)) << error; \
  EXPECT_EQ(cut.value, cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { \
    return std::get<2>(info.param) + std::to_string(std::get<0>(std::get<1>(info.param))); \
  } \
);

#define CREATE_HYPERGRAPH_K_CUT_VALUE_TEST_FIXTURE(name, ns, HypergraphType, values) \
class name##Test : public testing::TestWithParam<TestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  const auto &[hypergraph, cut_pair, filename] = GetParam(); \
  const auto [k, cut_value] = cut_pair; \
  const HypergraphType copy(hypergraph); \
  const auto cutvalue = ns::minimum_cut_value(copy, k); \
  EXPECT_EQ(cutvalue, cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { \
    return std::get<2>(info.param) + std::to_string(std::get<0>(std::get<1>(info.param))); \
  } \
);

#define CREATE_HYPERGRAPH_K_CUT_DISCOVERY_TEST_FIXTURE(name, ns, HypergraphType, values) \
class name##Test : public testing::TestWithParam<TestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  const auto &[hypergraph, cut_pair, filename] = GetParam(); \
  const auto [k, cut_value] = cut_pair; \
  const HypergraphType copy(hypergraph); \
  const auto cut = ns::discover(copy, k, cut_value); \
  std::string error; \
  EXPECT_TRUE(cut_is_valid(cut, hypergraph, k, error)) << error; \
  EXPECT_EQ(cut.value, cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { \
    return std::get<2>(info.param) + std::to_string(std::get<0>(std::get<1>(info.param))); \
  } \
);

#define CREATE_HYPERGRAPH_K_CUT_DISCOVERY_VALUE_TEST_FIXTURE(name, ns, HypergraphType, values) \
class name##Test : public testing::TestWithParam<TestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  const auto &[hypergraph, cut_pair, filename] = GetParam(); \
  const auto [k, cut_value] = cut_pair; \
  const HypergraphType copy(hypergraph); \
  const auto cutvalue = ns::discover_value(copy, k, cut_value); \
  EXPECT_EQ(cutvalue, cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { \
    return std::get<2>(info.param) + std::to_string(std::get<0>(std::get<1>(info.param))); \
  } \
);

#define CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE2(name, unweighted, weighted1, weighted2) \
CREATE_HYPERGRAPH_MIN_CUT_TEST_FIXTURE(name##Unweighted, name, Hypergraph, min_cut_instances(unweighted)) \
CREATE_HYPERGRAPH_MIN_CUT_TEST_FIXTURE(name##WeightedIntegral, name, WeightedHypergraph<size_t>, min_cut_instances(weighted1)) \
CREATE_HYPERGRAPH_MIN_CUT_TEST_FIXTURE(name##WeightedFloating, name, WeightedHypergraph<double>, min_cut_instances(weighted2)) \
CREATE_HYPERGRAPH_MIN_CUT_VALUE_TEST_FIXTURE(name##ValueUnweighted, name, Hypergraph, min_cut_instances(unweighted)) \
CREATE_HYPERGRAPH_MIN_CUT_VALUE_TEST_FIXTURE(name##ValueWeightedIntegral, name, WeightedHypergraph<size_t>, min_cut_instances(weighted1)) \
CREATE_HYPERGRAPH_MIN_CUT_VALUE_TEST_FIXTURE(name##ValueWeightedFloating, name, WeightedHypergraph<double>, min_cut_instances(weighted2))

#define CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE(name) \
CREATE_HYPERGRAPH_MIN_CUT_TEST_SUITE2(name, small_unweighted_tests(), small_weighted_tests<size_t>(), small_weighted_tests<double>())

#define CREATE_HYPERGRAPH_MIN_CUT_TEST_FIXTURE(name, funcprefix, HypergraphType, values) \
class name##Test : public testing::TestWithParam<MinCutTestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  auto &[hypergraph, cut_value, filename] = GetParam(); \
  HypergraphType copy(hypergraph); \
  const auto cut = funcprefix##_min_cut(copy); \
  std::string error; \
  EXPECT_TRUE(cut_is_valid(cut, hypergraph, 2, error)) << error; \
  EXPECT_EQ(cut.value, cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { return std::get<2>(info.param); } \
);

#define CREATE_HYPERGRAPH_MIN_CUT_VALUE_TEST_FIXTURE(name, funcprefix, HypergraphType, values) \
class name##Test : public testing::TestWithParam<MinCutTestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  auto &[hypergraph, cut_value, filename] = GetParam(); \
  HypergraphType copy(hypergraph); \
  const auto cutvalue = funcprefix##_min_cut_value(copy); \
  EXPECT_EQ(cutvalue, cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { return std::get<2>(info.param); } \
);

#define CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_SUITE(name, func, epsilon) \
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_FIXTURE( \
  name##Unweighted, \
  func, \
  Hypergraph, \
  min_cut_instances(small_unweighted_tests()), \
  epsilon) \
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_FIXTURE( \
  name##WeightedIntegral, \
  func, \
  WeightedHypergraph<size_t>, \
  min_cut_instances(small_weighted_tests<size_t>()), \
  epsilon) \
CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_FIXTURE( \
  name##WeightedFloating, \
  func, \
  WeightedHypergraph<double>, \
  min_cut_instances(small_weighted_tests<double>()), \
  epsilon)

#define CREATE_HYPERGRAPH_APPROX_MIN_CUT_TEST_FIXTURE(name, func, HypergraphType, values, epsilon) \
class name##Test : public testing::TestWithParam<MinCutTestCaseInstance<HypergraphType>> {}; \
TEST_P(name##Test, Works) { \
  auto &[hypergraph, cut_value, filename] = GetParam(); \
  HypergraphType copy(hypergraph); \
  const auto cut = func(copy, epsilon); \
  std::string error; \
  EXPECT_TRUE(cut_is_valid(cut, hypergraph, 2, error)) << error; \
  EXPECT_LE(cut.value, epsilon * cut_value); \
} \
INSTANTIATE_TEST_SUITE_P( \
  name##Test, name##Test, testing::ValuesIn(values), \
  [](const testing::TestParamInfo<name##Test::ParamType>& info) { \
    std::string eps = std::to_string(static_cast<int>(epsilon * 10)); /* No '.'s allowed in name */ \
    return std::get<2>(info.param) + eps; \
  } \
);
