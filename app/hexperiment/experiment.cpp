//
// Created by Victor on 4/9/20.
//

#include "experiment.hpp"

namespace {

struct NoSuchHypergraphType : public std::exception {
  [[nodiscard]]
  const char *what() const noexcept override {
    return "No such hypergraph type";
  }
};

struct MissingField : public std::exception {
  [[nodiscard]]
  explicit MissingField(const char *message) : message_(message) {}

  [[nodiscard]]
  const char *what() const noexcept override {
    return message_;
  }

private:
  const char *message_;
};

struct FallbackNode {
  YAML::Node local_config;
  YAML::Node global_config;

  FallbackNode(const YAML::Node &local_config, const YAML::Node &global_config) :
      local_config(local_config), global_config(global_config) {}

  YAML::Node operator[](const std::string &val_name) {
    if (local_config[val_name]) {
      return local_config[val_name];
    }
    return global_config[val_name];
  }
};

HyGenPtr ring_generator_from_config(const YAML::Node &local_config, const YAML::Node &global_config) {
  FallbackNode config(local_config, global_config);
  return std::make_unique<RandomRingConstantEdgeHypergraph>(
      config["num_vertices"].as<size_t>(),
      config["num_vertices"].as<size_t>() * config["edge_mult"].as<size_t>(),
      config["radius"].as<double>(),
      config["seed"].as<size_t>()
  );
}

HyGenPtr generator_from_config(const YAML::Node &local_config, const YAML::Node &global_config) {
  if (FallbackNode(local_config, global_config)["type"].as<std::string>() == "ring") {
    return ring_generator_from_config(local_config, global_config);
  } else {
    throw NoSuchHypergraphType{};
  }
}

}

Experiment experiment_from_config_file(const std::filesystem::path &config_path,
                                       const std::filesystem::path &output_path) {
  YAML::Node node = YAML::LoadFile(config_path);

  if (node["hypergraphs"]) {
    return hypergraphs_experiment_from_file(config_path, output_path);
  }

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

Experiment hypergraphs_experiment_from_file(const std::filesystem::path &config_path,
                                            const std::filesystem::path &output_path) {
  YAML::Node node = YAML::LoadFile(config_path);
  if (!node["name"]) {
    throw MissingField("'name' not defined");
  }
  Experiment experiment = {
      .name = node["name"].as<std::string>(),
      .planted = false, // TODO not necessarily true, but default to false for now
  };
  std::transform(node["hypergraphs"].begin(),
                 node["hypergraphs"].end(),
                 std::back_inserter(experiment.generators),
                 [node](auto &&config) { return generator_from_config(config, node); });
  return experiment;
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
