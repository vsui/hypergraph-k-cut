#include <fstream>
#include <iostream>
#include <vector>
#include <functional>

#include "hypergraph/approx.hpp"
#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"
#include "hypergraph/hypergraph.hpp"
#include "hypergraph/order.hpp"
#include "hypergraph/registry.hpp"

struct Options {
  std::string algorithm_name;
  std::string filename;
  size_t k;
};

bool hmetis_file_is_unweighted(const std::string &filename) {
  std::ifstream input;
  input.open(filename);
  return is_unweighted_hmetis_file(input);
}

// Attempts to read a hypergraph from a file
template<typename HypergraphType>
bool parse_hypergraph(const std::string &filename, HypergraphType &hypergraph) {
  std::ifstream input;
  input.open(filename);
  input >> hypergraph;
  return input.eof();
}

// Runs algorithm
template<typename HypergraphType>
int dispatch(Options options) {
  HypergraphMinimumCutRegistry<HypergraphType> registry;

  // Read hypergraph
  HypergraphType hypergraph;
  if (!parse_hypergraph(options.filename, hypergraph)) {
    std::cout << "Failed to parse hypergraph in " << options.filename << std::endl;
    return 1;
  }

  if constexpr (std::is_same_v<HypergraphType, Hypergraph>) {
    std::cout << "Read unweighted hypergraph with ";
  } else {
    std::cout << "Read weighted hypergraph with ";
  }
  std::cout << hypergraph.num_vertices() << " vertices and "
            << hypergraph.num_edges() << " edges" << std::endl;

  if (options.k < 2) {
    std::cerr << "k < 2 is not supported" << std::endl;
    return 1;
  }

  std::function<typename HypergraphType::EdgeWeight(const HypergraphType &, size_t)> f;

  if (options.k == 2) {
    const auto it = registry.minimum_cut_functions.find(options.algorithm_name);
    if (it == std::end(registry.minimum_cut_functions)) {
      std::cerr << "Algorithm \"" << options.algorithm_name << "\" not registered" << std::endl;
      return 1;
    }
    f = [it](const HypergraphType &h, int k) {
      return it->second(h);
    };
  } else {
    const auto it = registry.minimum_k_cut_functions.find(options.algorithm_name);
    if (it == std::end(registry.minimum_k_cut_functions)) {
      if (registry.minimum_cut_functions.find(options.algorithm_name) != std::end(registry.minimum_cut_functions)) {
        std::cerr << "Algorithm \"" << options.algorithm_name
                  << "\" is a minimum cut function and is only valid for k = 2" << std::endl;
        return 1;
      } else {
        std::cerr << "Algorithm \"" << options.algorithm_name << "\" not registered" << std::endl;
        return 1;
      }
    }
    f = it->second;
  }

  auto start = std::chrono::high_resolution_clock::now();
  auto cut_size = f(hypergraph, options.k);
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Algorithm took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                start)
                .count()
            << " milliseconds\n";
  std::cout << "The minimum cut size is " << cut_size << std::endl;

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 4 && argc != 5 /* for epsilon */) {
    usage:
    std::cerr << "Usage: " << argv[0]
              << " <input hypergraph filename> <k> <algorithm>\n"
              << "  algorithm: FPZ, CXY, Q, KW or MW\n";
    return 1;
  }

  Options options;
  options.filename = argv[1];
  options.k = std::stoul(argv[2]);
  options.algorithm_name = argv[3];

  if (hmetis_file_is_unweighted(options.filename)) {
    return dispatch<Hypergraph>(options);
  } else {
    return dispatch<WeightedHypergraph<double>>(options);
  }
}
