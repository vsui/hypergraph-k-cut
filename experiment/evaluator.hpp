//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP

#include <memory>
#include <vector>

class CutInfoStore;
class HypergraphSource;
class HypergraphGenerator;

class CutEvaluator {
public:
  CutEvaluator(std::unique_ptr<HypergraphSource> &&source, std::unique_ptr<CutInfoStore> &&store);

  /**
   * Evaluate cuts.
   */
  virtual void run() = 0;

  /**
   * Evaluate a single cut and send any relevant data to the data store.
   */
  virtual void evaluate() = 0;

  virtual ~CutEvaluator() = default;

protected:
  std::unique_ptr<CutInfoStore> store_;
  std::unique_ptr<HypergraphSource> source_;
};

class MinimumCutFinder : public CutEvaluator {
public:
  using CutEvaluator::CutEvaluator;
  void run() override;
  void evaluate() override;
};

class KDiscoveryRunner {
public:
  KDiscoveryRunner(std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                   std::unique_ptr<CutInfoStore> &&store);
  void run();
private:
  std::vector<std::unique_ptr<HypergraphGenerator>> src_;
  std::unique_ptr<CutInfoStore> store_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
