//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_COMMON_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_COMMON_HPP

#include <variant>
#include <vector>
#include <iostream>

#include <hypergraph/hypergraph.hpp>

struct CutInfo {
  size_t k;
  size_t cut_value;
  std::vector<std::vector<int>> partitions;

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
