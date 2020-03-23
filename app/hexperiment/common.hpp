//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_COMMON_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_COMMON_HPP

#include <variant>
#include <vector>
#include <iostream>

#include <hypergraph/hypergraph.hpp>
#include <hypergraph/cut.hpp>

/**
 * Return partitions so that they are sorted by size and lexographic order (and each partition is sorted)
 * @param partitions
 * @return
 */
inline std::vector<std::vector<int>> normalize_partitions(const std::vector<std::vector<int>> &partitions) {
  std::vector<std::vector<int>> normalized(std::begin(partitions), std::end(partitions));
  for (auto &p : normalized) {
    std::sort(std::begin(p), std::end(p));
  }
  std::sort(begin(normalized), end(normalized), [](std::vector<int> &a, std::vector<int> &b) {
    if (a.size() == b.size()) {
      return a < b;
    }
    return a.size() < b.size();
  });
  return normalized;
}

struct CutInfo {
  size_t k;
  size_t cut_value;
  std::vector<std::vector<int>> partitions;

  CutInfo(size_t k, size_t cut_value) : k(k), cut_value(cut_value) {}

  CutInfo(size_t k, const HypergraphCut<size_t> cut)
      : k(k), cut_value(cut.value), partitions(normalize_partitions(cut.partitions)) {}

  inline bool operator==(const CutInfo &info) const {
    return k == info.k && cut_value == info.cut_value && partitions == info.partitions;
  };
};

/// hypergraph
/// k
/// cut_value
/// PARTITION1
/// PARTITION2
/// ....

inline std::ostream &operator<<(std::ostream &out, const CutInfo &info) {
  out << info.k << std::endl << info.cut_value << std::endl;
  for (const auto &part : info.partitions) {
    for (const auto v : part) {
      out << v << " ";
    }
    out << std::endl;
  }

  return out;
}

inline std::istream &operator>>(std::istream &in, CutInfo &info) {
  in >> info.k >> info.cut_value;
  std::string line;
  std::getline(in, line);

  while (std::getline(in, line)) {
    std::stringstream stream(line);
    std::vector<int> partition;
    while (stream) {
      int i;
      stream >> i;
      partition.push_back(i);
    }
    info.partitions.emplace_back(std::move(partition));
  }

  return in;
}

struct CutRunInfo {
  CutRunInfo(const std::string &experiment_id, const CutInfo &cut_info)
      : experiment_id(experiment_id), info(cut_info) {}

  std::string experiment_id;
  CutInfo info;
  std::string algorithm; // A unique ID for the algorithm used
  std::string machine; // ID for machine this was run on
  uint64_t time;
  std::string commit; // Commit this was taken on

  inline static std::string csv_header() {
    using namespace std::literals::string_literals;
    return "algorithm,cut_id,k,value,machine,time,commit"s;
  };
};

inline std::ostream &operator<<(std::ostream &out, const CutRunInfo &info) {
  return out << info.algorithm << "," << info.info.k << "," << info.info.cut_value << ","
             << info.machine << "," << info.time << "," << info.commit << std::endl;
}

struct HypergraphWrapper {
  std::string name;
  std::variant<Hypergraph, WeightedHypergraph<size_t>> h;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_COMMON_HPP
