#include <filesystem>
#include <memory>
#include <cstdlib>

#include <yaml-cpp/yaml.h>
#include <tclap/CmdLine.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <hypergraph/order.hpp>

#include "generators.hpp"
#include "store.hpp"
#include "runner.hpp"

using HyGenPtr = std::unique_ptr<HypergraphGenerator>;
using HyGenPtrs = std::vector<HyGenPtr>;
using Experiment = std::tuple<std::string, HyGenPtrs, bool /* compare_kk */, bool /* planted */>;

namespace {

/**
 * Generate an experiment for planted hypergraphs where the size of edges in the crossing and non-crossing hyperedges
 * are equal in expectation.
 *
 * @param name name of experiment
 * @param num_vertices
 * @param k
 * @param m2_mult set m2 to `n / m2_mult`
 * @param m1_mult set m1 to `m2 * m1_mult`
 */
Experiment planted_experiment(const std::string &name,
                              const std::vector<size_t> &num_vertices,
                              size_t k,
                              size_t m2_mult,
                              size_t m1_mult) {
  Experiment planted_experiment;

  std::get<0>(planted_experiment) = name;
  std::get<2>(planted_experiment) = false;
  std::get<3>(planted_experiment) = true;

  for (size_t n : num_vertices) {
    size_t m2 = n / m2_mult;
    size_t m1 = m2 * m1_mult;
    double p2 = 0.1;
    double p1 = p2 * k;
    std::get<1>(planted_experiment).emplace_back(new PlantedHypergraph(n, m1, p1, m2, p2, k, 777));
  }
  return planted_experiment;
}

Experiment planted_experiment_from_yaml(const std::string &name, const YAML::Node &node) {
  return planted_experiment(
      name,
      node["num_vertices"].as<std::vector<size_t>>(),
      node["k"].as<size_t>(),
      node["m2_mult"].as<size_t>(),
      node["m1_mult"].as<size_t>()
  );
}

/**
 * Generate an experiment for uniform planted hypergraphs.
 * @param name
 * @param num_vertices
 * @param k
 * @param rank
 * @param m2_mult set m2 to `n / m2_mult`
 * @param m1_mult set m1 to `m2 * m1_mult`
 * @return
 */
Experiment planted_uniform_experiment(const std::string &name,
                                      const std::vector<size_t> &num_vertices,
                                      size_t k,
                                      size_t rank,
                                      size_t m2_mult,
                                      size_t m1_mult) {
  Experiment experiment;

  std::get<0>(experiment) = name;
  std::get<2>(experiment) = true;
  std::get<3>(experiment) = true;

  for (size_t n : num_vertices) {
    size_t m2 = n / m2_mult;
    size_t m1 = m2 * m1_mult;
    std::get<1>(experiment).emplace_back(new UniformPlantedHypergraph(n, k, rank, m1, m2, 777));
  }
  return experiment;
}

Experiment planted_constant_rank_experiment_from_yaml(const std::string &name, const YAML::Node &node) {
  return planted_uniform_experiment(
      name,
      node["num_vertices"].as<std::vector<size_t>>(),
      node["k"].as<size_t>(),
      node["rank"].as<size_t>(),
      node["m2_mult"].as<size_t>(),
      node["m1_mult"].as<size_t>()
  );
}

Experiment ring_experiment(const std::string &name,
                           const std::vector<size_t> &num_vertices,
                           size_t edge_mult,
                           size_t radius) {
  Experiment experiment;

  std::get<0>(experiment) = name;
  std::get<2>(experiment) = false;
  std::get<3>(experiment) = false;

  for (size_t n : num_vertices) {
    size_t num_edges = n * edge_mult;
    std::get<1>(experiment).emplace_back(new RandomRingConstantEdgeHypergraph(n, num_edges, radius, 777));
  }
  return experiment;
}

Experiment ring_experiment_from_yaml(const std::string &name, const YAML::Node &node) {
  return ring_experiment(
      name,
      node["num_vertices"].as<std::vector<size_t>>(),
      node["edge_mult"].as<size_t>(),
      node["radius"].as<size_t>()
  );
}

}

int main(int argc, char **argv) {
  using namespace std::string_literals;

  TCLAP::CmdLine cmd("Hypergraph experiment runner", ' ', "0.1");

  TCLAP::UnlabeledValueArg<std::string> configFileArg(
      "config",
      "A path to the configuration file",
      true,
      "",
      "A path to a valid configuration file",
      cmd);

  TCLAP::UnlabeledValueArg<std::string> destArg(
      "dest",
      "Output directory for experiment artifacts",
      true,
      "",
      "A folder name");

  TCLAP::SwitchArg listSizesArg(
      "s",
      "sizes",
      "List sizes of generated hypergraphs");

  TCLAP::SwitchArg checkCutsArg(
      "c",
      "check-cuts",
      "Check that cuts are not skewed or trivial");

  std::vector<TCLAP::Arg *> xor_list = {&destArg, &listSizesArg, &checkCutsArg};

  cmd.xorAdd(xor_list);

  cmd.parse(argc, argv);

  // Parse config file
  YAML::Node node = YAML::LoadFile(configFileArg.getValue());
  std::filesystem::path config_path(configFileArg.getValue());

  using ExperimentGenerator = std::function<Experiment(const std::string &, const YAML::Node &)>;
  const std::map<std::string, ExperimentGenerator> gens = {
      {"planted", planted_experiment_from_yaml},
      {"planted_constant_rank", planted_constant_rank_experiment_from_yaml},
      {"ring", ring_experiment_from_yaml}
  };

  bool cutoff = node["cutoff"] && node["cutoff"].as<bool>();

  auto experiment_type = node["type"].as<std::string>();
  auto it = gens.find(experiment_type);
  if (it == gens.end()) {
    std::cerr << "Unknown experiment type '" << experiment_type << "'" << std::endl;
    return 1;
  }
  auto num_runs = node["num_runs"].as<size_t>();
  auto experiment = it->second(destArg.getValue(), node);
  auto &[name, generators, compare_kk, planted] = experiment;

  if (listSizesArg.isSet()) {
    for (const auto &gen : generators) {
      auto[hgraph, planted] = gen->generate();
      std::cout << hgraph.num_vertices() << "," << hgraph.size() << std::endl;
    }
    return 0;
  } else if (checkCutsArg.isSet()) {
    if (node["type"].as<std::string>() != "ring" && node["k"].as<size_t>() > 2) {
      std::cerr << "ERROR: cannot check the cuts if k > 2" << std::endl;
      return 1;
    }
    for (const auto &gen : generators) {
      auto[hgraph, planted] = gen->generate();
      std::cout << "Checking " << gen->name() << std::endl;
      auto num_vertices = hgraph.num_vertices(); // Since MW_min_cut modifies hgraph
      auto cut = MW_min_cut(hgraph);
      if (cut.value == 0) {
        std::cout << gen->name() << " is disconnected" << std::endl;
      }
      auto skew = static_cast<double>(cut.partitions[0].size()) / num_vertices;
      if (skew < 0.1 || skew > 0.9) {
        std::cout << gen->name() << " has a skewed min cut (" << cut.partitions.at(0).size() << ", "
                  << cut.partitions.at(1).size() << ")" << std::endl;
      } else {
        std::cout << gen->name() << " has a non-skewed min cut (" << cut.partitions.at(0).size() << ", "
                  << cut.partitions.at(1).size() << ")" << std::endl;
      }
    }
    return 0;
  }

  // Prepare output directory
  if (std::filesystem::exists(destArg.getValue())) {
    std::cerr << "Error: " << destArg.getValue() << " already exists" << std::endl;
    return 1;
  }
  std::filesystem::path dest_path = std::filesystem::path(destArg.getValue());
  if (!std::filesystem::create_directory(dest_path)) {
    std::cerr << "Failed to create output directory '" << dest_path << "'" << std::endl;
    return 1;
  }
  std::filesystem::path db_path = dest_path / "data.db";

  // Prepare logger
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(dest_path / "log.txt", true);
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  std::shared_ptr<spdlog::logger> logger(new spdlog::logger("multi_sink", {file_sink, console_sink}));

  spdlog::set_default_logger(logger);

  std::filesystem::copy_file(config_path, dest_path / "config.yaml");

  // Prepare sqlite database
  auto store = std::make_shared<SqliteStore>();
  if (!store->open(db_path)) {
    std::cerr << "Failed to open store" << std::endl;
    return 1;
  }

  // Run experiment
  ExperimentRunner runner(name, std::move(generators), store, compare_kk, planted, cutoff, num_runs);

  if (cutoff) {
    runner.set_cutoff_percentages(node["percentages"].as<std::vector<size_t>>());
  }

  runner.run();

  std::filesystem::path here = std::filesystem::absolute(__FILE__).remove_filename();

  std::stringstream python_cmd;
  python_cmd << "python3 "s
             << (here / ".." / ".." / (cutoff ? "scripts/sqlplot-cutoff.py" : "scripts/sqlplot.py")) << " "
             << destArg.getValue();

  std::cout << "Done, writing artifacts to " << destArg.getValue() << std::endl;

  std::system(python_cmd.str().c_str());

  return 0;
}