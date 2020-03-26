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

class KDiscoveryRunner {
public:
  KDiscoveryRunner(const std::string &id,
                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                   std::shared_ptr<CutInfoStore> store,
                   bool compare_kk,
                   bool planted,
                   size_t num_runs = 20);
  void run();
private:
  std::string id_;
  std::vector<std::unique_ptr<HypergraphGenerator>> src_;
  std::shared_ptr<CutInfoStore> store_;
  bool compare_kk_;
  size_t num_runs_;
  bool planted_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
