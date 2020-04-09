//
// Created by Victor on 4/9/20.
//

#ifndef HYPERGRAPHPARTITIONING_APP_HEXPERIMENT_EXPERIMENT_HPP
#define HYPERGRAPHPARTITIONING_APP_HEXPERIMENT_EXPERIMENT_HPP

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>
#include <filesystem>

#include "generators.hpp"

using HyGenPtr = std::unique_ptr<HypergraphGenerator>;
using HyGenPtrs = std::vector<HyGenPtr>;

struct Experiment {
  std::string name;
  HyGenPtrs generators;
  bool planted;
};

// If output_path is null, then the experiment will have a blank name which will cause an error if you try to actually
// run it. So the null option should only be used if not running the experiment (so listing size or checking cuts).
Experiment experiment_from_config_file(const std::filesystem::path &config_path,
                                       const std::filesystem::path &output_path = {});

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
                              size_t m1_mult);

Experiment planted_experiment_from_yaml(const std::string &name, const YAML::Node &node);

Experiment disconnected_planted_experiment(const std::string &name,
                                           const std::vector<size_t> &num_vertices,
                                           size_t k,
                                           size_t m);

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
                                      size_t m1_mult);

Experiment planted_constant_rank_experiment_from_yaml(const std::string &name, const YAML::Node &node);

Experiment ring_experiment(const std::string &name,
                           const std::vector<size_t> &num_vertices,
                           size_t edge_mult,
                           size_t radius);

Experiment ring_experiment_from_yaml(const std::string &name, const YAML::Node &node);

Experiment disconnected_planted_experiment_from_yaml(const std::string &name, const YAML::Node &node);

#endif //HYPERGRAPHPARTITIONING_APP_HEXPERIMENT_EXPERIMENT_HPP
