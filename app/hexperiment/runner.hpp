//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP

#include <memory>
#include <vector>
#include <string>

class CutInfoStore;
class HypergraphSource;
class HypergraphGenerator;

class ExperimentRunner {
public:
  ExperimentRunner(const std::string &id,
                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                   std::shared_ptr<CutInfoStore> store,
                   bool planted,
                   bool cutoff,
                   size_t num_runs = 20);
  void run();

  void set_cutoff_percentages(const std::vector<size_t> &cutoffs);

private:
  // The functions to use
  std::vector<std::string> funcnames_;

  std::string id_;
  std::vector<std::unique_ptr<HypergraphGenerator>> src_;
  std::shared_ptr<CutInfoStore> store_;
  size_t num_runs_;
  bool planted_;
  bool cutoff_;
  std::vector<size_t> cutoff_percentages_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
