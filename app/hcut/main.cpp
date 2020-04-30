#include <fstream>
#include <iostream>
#include <vector>
#include <functional>
#include <filesystem>
#include <optional>

#include <tclap/CmdLine.h>

#include <hypergraph/hypergraph.hpp>
#include <hypergraph/certificate.hpp>
#include <hypergraph/order.hpp>

#include "builder.hpp"

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
    for (const auto &builder : cut_funcs<Hypergraph>) {
      allowed.emplace_back(builder->name());
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
    options.algorithm = algorithmArg.getValue();
    options.filename = filenameArg.getValue();
    options.k = kArg.getValue();

    constexpr auto
        flag_to_optional = [](auto &flag) -> std::optional<std::remove_reference_t<decltype(flag.getValue())>> {
      return flag.isSet() ? std::make_optional(flag.getValue()) : std::nullopt;
    };

    options.epsilon = flag_to_optional(epsilonArg);

    options.runs = flag_to_optional(numRunsArg);
    options.discover = flag_to_optional(discoveryArg);
    options.random_seed = randomSeedArg.getValue(); // TODO make optional
    options.verbosity = verbosityArg.getValue(); // TODO make optional;
    return true;
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return false;
  }
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
  // Prepare function
  const auto it = std::find_if(cut_funcs<HypergraphType>.cbegin(),
                               cut_funcs<HypergraphType>.cend(),
                               [&options](const auto &builder) {
                                 return builder->name() == options.algorithm;
                               });
  if (it == cut_funcs<HypergraphType>.cend()) {
    std::cout << "Unknown algorithm '" << options.algorithm << "'" << std::endl;
    return 1;
  }
  (*it)->check(options);
  const auto func = (*it)->build(options);

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

  // To check the results later we need a copy of the hypergraph since the cut function may modify it
  HypergraphType copy(hypergraph);

  // Run function
  const auto start = std::chrono::high_resolution_clock::now();
  auto cut = func(copy);
  const auto stop = std::chrono::high_resolution_clock::now();

  // Report results
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
