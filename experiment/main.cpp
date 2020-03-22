#include <filesystem>
#include <memory>
#include <cstdlib>

#include <yaml-cpp/yaml.h>
#include <tclap/CmdLine.h>

#include "generators.hpp"
#include "source.hpp"
#include "store.hpp"
#include "evaluator.hpp"

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
      "A folder name",
      cmd);

  cmd.parse(argc, argv);

  if (std::filesystem::exists(destArg.getValue())) {
    std::cerr << "Error: " << destArg.getValue() << " already exists" << std::endl;
    return 1;
  }

  using ExperimentGenerator = std::function<Experiment(const std::string &, const YAML::Node &)>;
  const std::map<std::string, ExperimentGenerator> gens = {
      {"planted", planted_experiment_from_yaml},
      {"constant_rank_planted", planted_constant_rank_experiment_from_yaml},
      {"ring", ring_experiment_from_yaml}
  };

  YAML::Node node = YAML::LoadFile(configFileArg.getValue());

  auto experiment_type = node["type"].as<std::string>();
  auto it = gens.find(experiment_type);
  if (it == gens.end()) {
    std::cerr << "Unknown experiment type '" << experiment_type << "'" << std::endl;
    return 1;
  }
  size_t num_runs = node["num_runs"].as<size_t>();

  if (!std::filesystem::create_directory(destArg.getValue())) {
    std::cerr << "Failed to create output directory '" << destArg.getValue() << "'" << std::endl;
    return 1;
  }
  std::filesystem::path db_path = std::filesystem::path(destArg.getValue()) / "data.db";

  auto store = std::make_shared<SqliteStore>();
  if (!store->open(db_path)) {
    std::cerr << "Failed to open store" << std::endl;
    return 1;
  }

  auto experiment = it->second(destArg.getValue(), node);
  auto &[name, generators, compare_kk, planted] = experiment;
  KDiscoveryRunner runner(name, std::move(generators), store, compare_kk, planted, num_runs);
  runner.run();

  std::filesystem::path here = std::filesystem::absolute(__FILE__).remove_filename();

  std::stringstream python_cmd;
  python_cmd << "python3 "s
             << (here / ".." / "scripts/sqlplot.py") << " "
             << destArg.getValue();

  std::system(python_cmd.str().c_str());

  return 0;
}