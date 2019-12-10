#include <fstream>
#include <iostream>
#include <vector>
#include <functional>

#include <hypergraph/kk.hpp>
#include "hypergraph/approx.hpp"
#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"
#include "hypergraph/hypergraph.hpp"
#include "hypergraph/certificate.hpp"
#include "hypergraph/order.hpp"
#include "hypergraph/registry.hpp"

std::string binary_name;

enum class cut_algorithm {
  CXY,
  FPZ,
  MW,
  KW,
  Q,
  CX,
  KK,
};

struct Options {
  cut_algorithm algorithm;
  std::string filename;
  size_t k;
};

static std::map<std::string, cut_algorithm> string_to_algorithm = {
    {"CXY", cut_algorithm::CXY},
    {"FPZ", cut_algorithm::FPZ},
    {"MW", cut_algorithm::MW},
    {"KW", cut_algorithm::KW},
    {"Q", cut_algorithm::Q},
    {"CX", cut_algorithm::CX},
    {"KK", cut_algorithm::KK}
};

int usage() {
  HypergraphMinimumCutRegistry<Hypergraph> registry;
  std::cerr << "Usage: " << binary_name
            << " <input hypergraph filename> <k> <algorithm>\n";
  std::cerr << "Algorithms:\n";
  std::cerr << " min-cut (works for k = 2):\n";
  for (const auto &[name, f] : registry.minimum_cut_functions) {
    std::cerr << "  " << name << std::endl;
  }
  std::cerr << " min-k-cut (works for k > 1):\n";
  for (const auto &[name, f] : registry.minimum_k_cut_functions) {
    std::cerr << "  " << name << std::endl;
  }
  return 1;
}

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
  HypergraphMinimumCutRegistry<HypergraphType, true> registry;

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

  size_t num_runs;
  double epsilon;

  // Special logic to pass in number of runs
  switch (options.algorithm) {
  case cut_algorithm::CXY:
  case cut_algorithm::FPZ: {
    size_t recommended_num_runs;
    if (options.algorithm == cut_algorithm::CXY) {
      recommended_num_runs = cxy::default_num_runs(hypergraph, options.k);
    } else {
      recommended_num_runs = fpz::default_num_runs(hypergraph, options.k);
    }
    std::cout << "Input how many times you would like to run the algorithm (recommended is " << recommended_num_runs
              << " for low error probability)" << std::endl;
    std::cin >> num_runs;
    break;
  }
  default:break;
  }

  // Special logic to pass in epsilon
  switch (options.algorithm) {
  case cut_algorithm::CX:
  case cut_algorithm::KK: {
    std::cout << "Please specify epsilon" << std::endl;
    std::cin >> epsilon;
  }
  default:break;
  }

  // Check k is valid
  switch (options.algorithm) {
  case cut_algorithm::CXY:
  case cut_algorithm::FPZ: {
    // k is valid
    break;
  }
  case cut_algorithm::MW:
  case cut_algorithm::KW:
  case cut_algorithm::Q:
  case cut_algorithm::CX:
  case cut_algorithm::KK: {
    if (options.k != 2) {
      std::cout << "Only k=2 is acceptable for this algorithm" << std::endl;
      return 1;
    }
    break;
  }
  }

  // Set f
  MinimumKCutFunction<HypergraphType> f;
  switch (options.algorithm) {
  case cut_algorithm::CXY: {
    f = [num_runs](HypergraphType &hypergraph, size_t k) {
      return cxy::cxy_contract<HypergraphType, true>(hypergraph, k, num_runs);
    };
    break;
  }
  case cut_algorithm::FPZ: {
    std::cout << "Input 1 if you want to output the values of intermediate cuts, 0 otherwise" << std::endl;
    int i;
    std::cin >> i;
    if (i == 1) {
      f = [num_runs](HypergraphType &hypergraph, size_t k) {
        return fpz::branching_contract<HypergraphType, true, true>(hypergraph, k, num_runs);
      };
    } else if (i == 0) {
      f = [num_runs](HypergraphType &hypergraph, size_t k) {
        return fpz::branching_contract<HypergraphType, true, false>(hypergraph, k, num_runs);
      };
    } else {
      std::cout << "Bad input" << std::endl;
      exit(1);
    }
    break;
  }
  case cut_algorithm::MW: {
    f = [](HypergraphType &hypergraph, size_t k) {
      return vertex_ordering_mincut<HypergraphType, tight_ordering>(hypergraph);
    };
    break;
  }
  case cut_algorithm::KW: {
    f = [](HypergraphType &hypergraph, size_t k) {
      return vertex_ordering_mincut<HypergraphType, maximum_adjacency_ordering>(hypergraph);
    };
    break;
  }
  case cut_algorithm::Q: {
    f = [](HypergraphType &hypergraph, size_t k) {
      return vertex_ordering_mincut<HypergraphType, queyranne_ordering>(hypergraph);
    };
    break;
  }
  case cut_algorithm::CX: {
    f = [epsilon](HypergraphType &hypergraph, size_t k) {
      return approximate_minimizer(hypergraph, epsilon);
    };
    break;
  }
  case cut_algorithm::KK: {
    std::cout << "Please input r" << std::endl;
    size_t r;
    std::cin >> r;
    f = [epsilon, r](HypergraphType &hypergraph, size_t k) {
      return kk::contract<HypergraphType>(hypergraph, r, epsilon);
    };
    break;
  }
  }

  // Weighted sparsifier only works on integral cuts
  if constexpr (!is_weighted<HypergraphType>) {
    if (options.k == 2) {
      std::cout << "Input \"1\" if you would like to use a sparsifier, \"2\" otherwise" << std::endl;
      size_t i;
      std::cin >> i;
      if (i != 1 && i != 2) {
        std::cout << "Invalid input" << std::endl;
        return 1;
      }
      if (i == 1) {
        f = [f](HypergraphType &hypergraph, size_t) {
          const auto min_cut = [f](HypergraphType &hypergraph) {
            return f(hypergraph, 2);
          };
          return certificate_minimum_cut<HypergraphType>(hypergraph, min_cut);
        };
      }
    }
  }

  // To check the results later we need a copy of the hypergraph since the cut function may modify it
  HypergraphType copy(hypergraph);

  auto start = std::chrono::high_resolution_clock::now();
  auto cut = f(hypergraph, options.k);
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Algorithm took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                start)
                .count()
            << " milliseconds\n";

  for (auto &partition : cut.partitions) {
    std::sort(std::begin(partition), std::end(partition));
  }

  std::cout << cut;
  std::string error;
  if (!cut_is_valid(cut, copy, options.k, error)) {
    std::cout << "ERROR: CUT IS NOT VALID " << "(" << error << ")" << std::endl;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  binary_name = argv[0];
  if (argc != 4 && argc != 5 /* for epsilon */) {
    return usage();
  }

  Options options;
  options.filename = argv[1];
  options.k = std::stoul(argv[2]);

  auto algorithm = string_to_algorithm.find(argv[3]);
  if (algorithm == std::end(string_to_algorithm)) {
    return usage();
  }
  options.algorithm = algorithm->second;

  if (hmetis_file_is_unweighted(options.filename)) {
    return dispatch<Hypergraph>(options);
  } else {
    return dispatch<WeightedHypergraph<double>>(options);
  }
}
