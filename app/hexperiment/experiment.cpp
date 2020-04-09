//
// Created by Victor on 4/9/20.
//

#include "experiment.hpp"

Experiment experiment_from_config_file(const std::filesystem::path &config_path,
                                       const std::filesystem::path &output_path) {
  YAML::Node node = YAML::LoadFile(config_path);

  using ExperimentGenerator = std::function<Experiment(const std::string &, const YAML::Node &)>;
  const std::map<std::string, ExperimentGenerator> gens = {
      {"planted", planted_experiment_from_yaml},
      {"planted_constant_rank", planted_constant_rank_experiment_from_yaml},
      {"ring", ring_experiment_from_yaml},
      {"disconnected", disconnected_planted_experiment_from_yaml}
  };

  auto experiment_type = node["type"].as<std::string>();
  auto it = gens.find(experiment_type);
  if (it == gens.end()) {
    throw std::runtime_error("Unknown experiment type '" + experiment_type + "'");
  }

  return it->second(output_path.string(), node);
}

Experiment planted_experiment(const std::string &name,
                              const std::vector<size_t> &num_vertices,
                              size_t k,
                              size_t m2_mult,
                              size_t m1_mult) {
  Experiment planted_experiment = {
      .name = name,
      .planted = true
  };

  for (size_t n : num_vertices) {
    size_t m2 = n / m2_mult;
    size_t m1 = m2 * m1_mult;
    double p2 = 0.1;
    double p1 = p2 * k;
    planted_experiment.generators.emplace_back(new PlantedHypergraph(n, m1, p1, m2, p2, k, 777));
  }
  return planted_experiment;
}

Experiment disconnected_planted_experiment(const std::string &name,
                                           const std::vector<size_t> &num_vertices,
                                           size_t k,
                                           size_t m) {
  Experiment planted_experiment = {
      .name = name,
      .planted = true,
  };

  for (size_t n : num_vertices) {
    size_t m2 = 0;
    size_t m1 = n * m;
    planted_experiment.generators.emplace_back(new PlantedHypergraph(n, m1, 0.1, m2, 0.1 * k, k, 777));
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

Experiment planted_uniform_experiment(const std::string &name,
                                      const std::vector<size_t> &num_vertices,
                                      size_t k,
                                      size_t rank,
                                      size_t m2_mult,
                                      size_t m1_mult) {
  Experiment experiment = {
      .name = name,
      .planted = true,
  };

  for (size_t n : num_vertices) {
    size_t m2 = n / m2_mult;
    size_t m1 = m2 * m1_mult;
    experiment.generators.emplace_back(new UniformPlantedHypergraph(n, k, rank, m1, m2, 777));
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
  Experiment experiment = {
      .name = name,
      .planted = false
  };

  for (size_t n : num_vertices) {
    size_t num_edges = n * edge_mult;
    experiment.generators.emplace_back(new RandomRingConstantEdgeHypergraph(n, num_edges, radius, 777));
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

Experiment disconnected_planted_experiment_from_yaml(const std::string &name, const YAML::Node &node) {
  return disconnected_planted_experiment(name,
                                         node["num_vertices"].as<std::vector<size_t>>(),
                                         node["k"].as<size_t>(),
                                         node["m"].as<size_t>());
}
