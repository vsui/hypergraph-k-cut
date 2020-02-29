//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP

#include <memory>

class CutInfoStore;
class HypergraphSource;
class PlantedHypergraphSource;

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
  KDiscoveryRunner(std::unique_ptr<PlantedHypergraphSource> &&source,
                   std::unique_ptr<CutInfoStore> &&store);
  void run();
private:
  std::unique_ptr<PlantedHypergraphSource> src_;
  std::unique_ptr<CutInfoStore> store_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
