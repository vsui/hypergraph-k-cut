#pragma once

#include <fstream>
#include <filesystem>

#include "hypergraph/hypergraph.hpp"

inline std::filesystem::path get_directory() {
  return std::filesystem::path(__FILE__).remove_filename();
}

template<typename HypergraphType = Hypergraph>
using CutValue = std::tuple<size_t, typename HypergraphType::EdgeWeight>;

template<typename HypergraphType = Hypergraph>
using CutValues = std::vector<CutValue<HypergraphType>>;

template<typename HypergraphType = Hypergraph>
using TestCaseInstance = std::tuple<HypergraphType, CutValue<HypergraphType>, std::string>;

template<typename HypergraphType>
using MinCutTestCaseInstance = std::tuple<HypergraphType, typename HypergraphType::EdgeWeight, std::string>;

// A collection of hypergraphs with associated cut values for multiple k's, for adding test values
template<typename HypergraphType = Hypergraph>
class TestCase {
public:
  bool from_file(const std::string &filename) {
    std::ifstream input;
    input.open(filename);

    if (!input) {
      return false;
    }

    input >> hypergraph_;

    if (!input) {
      return false;
    }

    std::string line;

    // Throw away junk line
    std::getline(input, line);

    // Accrue cuts
    while (std::getline(input, line)) {
      std::stringstream stream(line);
      size_t k;
      typename HypergraphType::EdgeWeight val;
      stream >> k >> val;
      cuts_.emplace_back(k, val);
    }

    filename_ = std::filesystem::path(filename).stem();

    return !cuts_.empty();
  }

  std::vector<TestCaseInstance<HypergraphType>> split() {
    std::vector<TestCaseInstance<HypergraphType>> instances;
    for (const auto &[k, cut_value] : cuts()) {
      instances.emplace_back(hypergraph(), CutValue<HypergraphType>(k, cut_value), filename());
    }
    return instances;
  }

  typename Hypergraph::EdgeWeight cut(size_t k) {
    auto it = std::find_if(std::begin(cuts()), std::end(cuts()), [k](const auto &cut_value) {
      return std::get<0>(cut_value) == k;
    });
    if (it == std::end(cuts())) {
      // Just return max as tombstone if k is not found
      return std::numeric_limits<typename Hypergraph::EdgeWeight>::max();
    }
    return std::get<1>(*it);
  }

  const HypergraphType &hypergraph() { return hypergraph_; }
  const CutValues<HypergraphType> &cuts() { return cuts_; }
  const std::string &filename() { return filename_; }
private:
  HypergraphType hypergraph_;
  CutValues<HypergraphType> cuts_;
  std::string filename_;
};

template<typename HypergraphType>
inline std::vector<TestCaseInstance<HypergraphType>> tests_in_folder(std::string folder_name) {
  using Instance = TestCaseInstance<HypergraphType>;

  std::vector<Instance> tests;
  std::string directory = get_directory() / folder_name;
  for (const auto &p : std::filesystem::directory_iterator(directory)) {
    TestCase<HypergraphType> tc;
    assert(tc.from_file(p.path()));
    const auto instances = tc.split();
    tests.insert(std::end(tests), std::begin(instances), std::end(instances));
  }

  return tests;
}

template<typename HypergraphType>
inline std::vector<TestCaseInstance<HypergraphType>> tests_in_folders(std::vector<std::string> folder_names) {
  using Instance = TestCaseInstance<HypergraphType>;

  std::vector<Instance> tests;

  for (const auto &folder_name : folder_names) {
    const auto t = tests_in_folder<HypergraphType>(folder_name);
    tests.insert(std::end(tests), std::begin(t), std::end(t));
  }

  return tests;
}

template<typename HypergraphType>
inline std::vector<TestCaseInstance<HypergraphType>> up_to_k(const std::vector<TestCaseInstance<HypergraphType>> &tests,
                                                             size_t k) {
  std::vector<TestCaseInstance<HypergraphType>> ret;
  std::copy_if(tests.begin(), tests.end(), std::back_inserter(ret), [k](auto &test) {
    const auto &[tmp0, cut_value, tmp1] = test;
    const auto &[k_, tmp2] = cut_value;
    return k_ < k;
  });
  return ret;
}

inline std::vector<TestCaseInstance<>> small_unweighted_tests() {
  return tests_in_folders<Hypergraph>({"instances/misc/small/unweighted", "instances/misc/smallrank/unweighted"});
}

template<typename EdgeWeight>
inline std::vector<TestCaseInstance<WeightedHypergraph<EdgeWeight>>> small_weighted_tests() {
  return tests_in_folders<WeightedHypergraph<EdgeWeight>>({"instances/misc/small/weighted",
                                                           "instances/misc/smallrank/weighted"});
}

template<typename HypergraphType>
inline std::vector<MinCutTestCaseInstance<HypergraphType>> min_cut_instances(const std::vector<TestCaseInstance<
    HypergraphType>> &instances) {
  std::vector<MinCutTestCaseInstance<HypergraphType>> mi;
  for (const auto &[hypergraph, cut_pair, filename]: instances) {
    if (std::get<0>(cut_pair) == 2) {
      mi.emplace_back(hypergraph, std::get<1>(cut_pair), filename);
    }
  }
  return mi;
}
