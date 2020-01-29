// Test that some of the helper classes used to set up actual tests work

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "testutil.hpp"

class SetupTestFromFile : public ::testing::Test {
protected:
  const std::string filename = get_directory() / "instances/simple.htest";
  const std::string weighted_filename = get_directory() / "instances/weighted_simple.htest";
  const std::string bad_filename = get_directory() / "instances/not_even_a_file.htest";

  const CutValues<> cuts = {{2, 2}, {3, 4}, {4, 5}};

  const Hypergraph simple = {
      {0, 1, 2, 3, 4},
      {
          {0, 1, 2},
          {1, 2, 3},
          {2, 3, 4},
          {3, 4, 0}
      }
  };

  const WeightedHypergraph<size_t> weighted_simple = {
      {0, 1, 2, 3, 4},
      {
          {{0, 1, 2}, 2},
          {{1, 2, 3}, 3},
          {{2, 3, 4}, 4},
          {{3, 4, 0}, 5}
      }
  };
};

TEST_F(SetupTestFromFile, CorrectlyReadsTestCase) {
  TestCase tc;
  ASSERT_TRUE(tc.from_file(filename));
  ASSERT_EQ(simple, tc.hypergraph());
  ASSERT_EQ(cuts, tc.cuts());
}

TEST_F(SetupTestFromFile, CorrectlyReadsWeightedTestCase) {
  TestCase<WeightedHypergraph<size_t>> tc;
  ASSERT_TRUE(tc.from_file(weighted_filename));
  ASSERT_EQ(weighted_simple, tc.hypergraph());
  ASSERT_EQ(cuts, tc.cuts());
}

TEST_F(SetupTestFromFile, CorrectlySplitsTestCase) {
  TestCase tc;
  ASSERT_TRUE(tc.from_file(filename));
  const auto inst = tc.split();
  ASSERT_THAT(inst, ::testing::ElementsAre(
      TestCaseInstance<>(tc.hypergraph(), {2, 2}, "simple"),
      TestCaseInstance<>(tc.hypergraph(), {3, 4}, "simple"),
      TestCaseInstance<>(tc.hypergraph(), {4, 5}, "simple")
  ));
}

TEST_F(SetupTestFromFile, FailsIfNoSuchFile) {
  TestCase tc;
  ASSERT_FALSE(tc.from_file(bad_filename));
}
