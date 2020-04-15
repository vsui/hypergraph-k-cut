#include <fstream>
#include <iostream>
#include <vector>
#include <functional>
#include <filesystem>
#include <optional>

#include <tclap/CmdLine.h>

#include <hypergraph/kk.hpp>
#include <hypergraph/approx.hpp>
#include <hypergraph/cxy.hpp>
#include <hypergraph/fpz.hpp>
#include <hypergraph/hypergraph.hpp>
#include <hypergraph/certificate.hpp>
#include <hypergraph/order.hpp>

#include "switch.hpp"

using namespace hypergraphlib;

std::map<std::string, cut_algorithm> string_to_algorithm = {
    {"CXY", cut_algorithm::CXY},
    {"FPZ", cut_algorithm::FPZ},
    {"MW", cut_algorithm::MW},
    {"KW", cut_algorithm::KW},
    {"Q", cut_algorithm::Q},
    {"CX", cut_algorithm::CX},
    {"KK", cut_algorithm::KK}
};

struct Options {
  Options() : algorithm(cut_algorithm::CXY) {}

  cut_algorithm algorithm;
  std::string filename;
  size_t k = 2; // Compute k-cut
  std::optional<size_t> runs; // Number of runs to repeat contraction algo for
  std::optional<double> epsilon; // Epsilon for approximation algorithms
  std::optional<double> discover; // Discovery value
  uint32_t random_seed = 0;
  uint8_t verbosity = 2; // Verbose output
};

/**
 * Parses command line arguments. Returns false on failure, true otherwise.
 *
 * @param argc
 * @param argv
 * @param options
 * @return
 */
bool read_options(int argc, char **argv, Options &options) {
  try {
    TCLAP::CmdLine cmd("Hypergraph cut tool", ' ', "0.1");

    TCLAP::UnlabeledValueArg<std::string>
        filenameArg("filename", "Filename for the input hypergraph", true, "", "A file path", cmd);

    TCLAP::UnlabeledValueArg<size_t> kArg("k", "Compute the k-cut", true, 0, "An integer greater than 1", cmd);

    // Only allow names in the string_to_algorithm map
    std::vector<std::string> allowed;
    for (const auto &[name, _] : string_to_algorithm) {
      allowed.emplace_back(name);
    }
    TCLAP::ValuesConstraint<std::string> allowedAlgorithms(allowed);

    TCLAP::UnlabeledValueArg<std::string>
        algorithmArg("algorithm", "Algorithm to use", true, "", &allowedAlgorithms, cmd);

    TCLAP::ValueArg<size_t>
        numRunsArg("r", "runs", "Number of runs to repeat contraction algorithm", false, 0, "A positive integer", cmd);

    TCLAP::ValueArg<double> epsilonArg("e", "epsilon", "Approximation factor", false, 0.0, "A positive float", cmd);

    TCLAP::ValueArg<double> discoveryArg("d",
                                         "discover",
                                         "Measure time needed to discover a cut with this value",
                                         false,
                                         0.0,
                                         "A non-negative number",
                                         cmd);

    std::vector<size_t> verbosityLevels = {0, 1, 2};
    TCLAP::ValuesConstraint<size_t> allowedVerbosityLevels(verbosityLevels);
    TCLAP::ValueArg<size_t> verbosityArg("v", "verbosity", "Verbose output", false, 2, &allowedVerbosityLevels, cmd);

    TCLAP::ValueArg<uint32_t>
        randomSeedArg("s", "seed", "Random seed", false, 0, "Random seed for randomized algorithms", cmd);

    cmd.parse(argc, argv);

    // Fill in options
    assert(string_to_algorithm.find(algorithmArg.getValue()) != string_to_algorithm.end());
    options.algorithm = string_to_algorithm.at(algorithmArg.getValue());
    options.filename = filenameArg.getValue();
    options.k = kArg.getValue();
    if (epsilonArg.isSet()) {
      options.epsilon = epsilonArg.getValue();
      if (options.algorithm != cut_algorithm::CX) {
        std::cerr << "error: Epsilon argument only valid for \"CX\" algorithm" << std::endl;
        return false;
      }
    } else if (options.algorithm == cut_algorithm::CX) {
      std::cerr << "error: \"CX\" algorithm requires epsilon to be specified" << std::endl;
      return false;
    }
    if (numRunsArg.isSet()) {
      options.runs = numRunsArg.getValue();
      if (!is_contraction_algorithm(options.algorithm)) {
        std::cerr << R"(error: Number of runs only valid for "CXY", "FPZ", and "KK")" << std::endl;
        return false;
      }
    } else {
      if (is_contraction_algorithm(options.algorithm) && !discoveryArg.isSet()) {
        std::cerr << "error: Contraction algorithm requires number of runs to be specified" << std::endl;
        return false;
      }
    }
    if (discoveryArg.isSet()) {
      if (!is_contraction_algorithm(options.algorithm)) {
        std::cerr << "error: --discover flag only valid for contraction algorithms" << std::endl;
        return false;
      }
      options.discover = discoveryArg.getValue();
    }
    if (verbosityArg.isSet()) {
      options.verbosity = verbosityArg.getValue();
    }
    if (randomSeedArg.isSet()) {
      options.random_seed = randomSeedArg.getValue();
    }
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  /* Check that k is valid */
  if (options.k < 2) {
    std::cerr << "k < 2 is not supported" << std::endl;
    return false;
  }
  switch (options.algorithm) {
  case cut_algorithm::CXY:
  case cut_algorithm::FPZ:
  case cut_algorithm::KK: {
    // k is valid
    break;
  }
  case cut_algorithm::MW:
  case cut_algorithm::KW:
  case cut_algorithm::Q:
  case cut_algorithm::CX: {
    if (options.k != 2) {
      std::cout << "Only k=2 is acceptable for this algorithm" << std::endl;
      return false;
    }
    break;
  }
  }
  /* Done checking that k is valid */


  return true;
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
  return true;
}

// Runs algorithm
template<typename HypergraphType>
int dispatch(Options options) {
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

  size_t num_runs{};

  /* Check if need to get default number of runs
   *
   * This is done in dispatch instead of read_options because we need to have access to the hypergraph since the default
   * number of runs is a function of the hypergraph size.
   *
   * In the future we could move this into read_options by potentially just reading the number of vertices in the header
   * of the .hmetis file.
   */
  if (is_contraction_algorithm(options.algorithm)) {
    const std::map<cut_algorithm, size_t> num_runs_map = {
        {cut_algorithm::CXY, cxy::CxyImpl::default_num_runs(hypergraph, options.k)},
        {cut_algorithm::FPZ, fpz::FpzImpl::default_num_runs(hypergraph, options.k)},
        {cut_algorithm::KK, kk::KkImpl::default_num_runs(hypergraph, options.k)}
    };
    size_t recommended_num_runs = num_runs_map.at(options.algorithm);

    if (!options.discover.has_value()) {
      if (!options.runs.has_value()) {
        std::cout << "Input how many times you would like to run the algorithm (recommended is " << recommended_num_runs
                  << " for low error probability)" << std::endl;
        std::cin >> num_runs;
      } else {
        num_runs = options.runs.value();
      }
    }
  }
  /* Done checking if need to get default number of runs */

  // Set f
  MinimumKCutFunction<HypergraphType> f;
  if (is_contraction_algorithm(options.algorithm)) {
    f = [num_runs, &options](HypergraphType &hypergraph, size_t k) {
      return mincut_switch<HypergraphType>(options.algorithm,
                                           options.verbosity,
                                           hypergraph,
                                           k,
                                           num_runs,
                                           options.random_seed);
    };
  }
  switch (options.algorithm) {
  case cut_algorithm::MW: {
    f = [](HypergraphType &hypergraph, size_t k) {
      return MW_min_cut(hypergraph);
    };
    break;
  }
  case cut_algorithm::KW: {
    f = [](HypergraphType &hypergraph, size_t k) {
      return KW_min_cut(hypergraph);
    };
    break;
  }
  case cut_algorithm::Q: {
    f = [](HypergraphType &hypergraph, size_t k) {
      return Q_min_cut(hypergraph);
    };
    break;
  }
  case cut_algorithm::CX: {
    f = [&options](HypergraphType &hypergraph, size_t k) {
      return approximate_minimizer(hypergraph, options.epsilon.value());
    };
    break;
  }
  case cut_algorithm::CXY:
  case cut_algorithm::FPZ:
  case cut_algorithm::KK:
    // Handled earlier
    break;
  }

  // Weighted sparsifier only works on integral cuts
  if constexpr (!is_weighted<HypergraphType>) {
    if (options.k == 2) {
      std::cout << R"(Input "1" if you would like to use a sparsifier, "2" otherwise)" << std::endl;
      size_t i{};
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
  auto cut = HypergraphCut<typename HypergraphType::EdgeWeight>::max();

  auto start = std::chrono::high_resolution_clock::now();
  if (options.discover.has_value()) {
    cut = discovery_switch<HypergraphType>(options.algorithm,
                                           options.verbosity,
                                           hypergraph,
                                           options.k,
                                           options.discover.value(),
                                           options.random_seed);
  } else {
    cut = f(hypergraph, options.k);
  }
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

int main(int argc, char **argv) {
  Options options;

  if (!read_options(argc, argv, options)) {
    return 1;
  }

  if (hmetis_file_is_unweighted(options.filename)) {
    return dispatch<Hypergraph>(options);
  } else {
    return dispatch<WeightedHypergraph<double>>(options);
  }
}
